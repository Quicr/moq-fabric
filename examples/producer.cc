// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

// Dials a relay as a stub, announces one track, and publishes datagrams when a peer subscribes.

#include "example_util.h"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {

    class ProducerApp : public moq::fabric::Delegate
    {
      public:
        ProducerApp(std::shared_ptr<spdlog::logger> logger,
                    moq::fabric::TrackName track,
                    std::string payload,
                    std::chrono::milliseconds interval)
          : logger_(std::move(logger))
          , track_(std::move(track))
          , track_hash_(moq::fabric::PeerManager::HashTrack(track_))
          , payload_(std::move(payload))
          , interval_(interval)
        {
        }

        ~ProducerApp() { Stop(); }

        void Attach(moq::fabric::PeerManager& peers) { peers_ = &peers; }

        void Start()
        {
            worker_ = std::thread([this] { Run(); });
        }

        // Join the publish thread before the PeerManager is destroyed.
        void Stop()
        {
            {
                std::lock_guard lock(mu_);
                stop_ = true;
            }
            cv_.notify_all();
            if (worker_.joinable()) {
                worker_.join();
            }
            peers_ = nullptr;
        }

        void OnPeerSubscribe(const moq::fabric::TrackName&,
                             std::uint64_t track_hash,
                             const moq::fabric::SubscribeParams& params) override
        {
            if (track_hash != track_hash_) {
                return;
            }

            {
                std::lock_guard lock(mu_);
                if (stop_) {
                    return;
                }
                priority_ = params.priority == 0 ? priority_ : params.priority;
                const bool already = publishing_;
                publishing_ = true;
                if (!already) {
                    next_send_ = std::chrono::steady_clock::now() + interval_;
                }
            }
            cv_.notify_all();
            logger_->info("subscriber joined hash {}", track_hash_);
            PublishOne();
        }

        void OnPeerUnsubscribe(std::uint64_t track_hash, bool subscribers_remain) override
        {
            if (track_hash != track_hash_ || subscribers_remain) {
                return;
            }
            {
                std::lock_guard lock(mu_);
                publishing_ = false;
            }
            cv_.notify_all();
            logger_->info("no subscribers left for hash {}", track_hash_);
        }

        void OnPeerUp(std::uint64_t session_id) override { logger_->info("peer up session {}", session_id); }

        void OnPeerDown(std::uint64_t session_id) override { logger_->info("peer down session {}", session_id); }

      private:
        void PublishOne()
        {
            moq::fabric::PeerManager* peers = nullptr;
            uint8_t priority = 10;
            uint64_t group = 0;
            std::shared_ptr<const std::vector<uint8_t>> payload;
            {
                std::lock_guard lock(mu_);
                if (stop_ || !publishing_ || peers_ == nullptr) {
                    return;
                }
                peers = peers_;
                priority = priority_;
                group = ++sequence_;
                const auto text = payload_ + " " + std::to_string(group);
                auto bytes = std::make_shared<std::vector<uint8_t>>(text.begin(), text.end());
                payload = std::move(bytes);
            }

            peers->SendData(
              track_hash_, priority, 1000, moq::fabric::DataType::kDatagram, group, 0, std::move(payload));
            logger_->info("sent datagram group {}", group);
        }

        void Run()
        {
            while (true) {
                std::chrono::steady_clock::time_point when;
                {
                    std::unique_lock lock(mu_);
                    cv_.wait(lock, [&] { return stop_ || publishing_; });
                    if (stop_) {
                        return;
                    }
                    when = next_send_;
                }

                std::unique_lock lock(mu_);
                const bool interrupted = cv_.wait_until(lock, when, [&] { return stop_ || !publishing_; });
                if (stop_) {
                    return;
                }
                if (interrupted) {
                    continue;
                }
                lock.unlock();
                PublishOne();

                std::lock_guard next(mu_);
                next_send_ = std::chrono::steady_clock::now() + interval_;
            }
        }

        std::shared_ptr<spdlog::logger> logger_;
        moq::fabric::TrackName track_;
        std::uint64_t track_hash_;
        std::string payload_;
        std::chrono::milliseconds interval_;

        std::mutex mu_;
        std::condition_variable cv_;
        moq::fabric::PeerManager* peers_{ nullptr };
        bool stop_{ false };
        bool publishing_{ false };
        uint8_t priority_{ 10 };
        uint64_t sequence_{ 0 };
        std::chrono::steady_clock::time_point next_send_{};
        std::thread worker_;
    };

} // namespace

int
main(int argc, char** argv)
{
    using moq::fabric::examples::ExampleKind;

    try {
        const auto options = moq::fabric::examples::ParseArgs(ExampleKind::kProducer, argc, argv);
        if (options.common.help) {
            std::cout << moq::fabric::examples::HelpText(ExampleKind::kProducer);
            return 0;
        }

        moq::fabric::examples::CheckCertificate(options.common.cert, options.common.key);
        auto logger = moq::fabric::examples::MakeLogger(options.common.node_id);
        const auto track = moq::fabric::examples::MakeTrack(options.name_space, options.track);

        ProducerApp app(logger, track, options.payload, std::chrono::milliseconds(options.interval_ms));

        // Declared after the manager so Stop() joins the publish thread while the manager is still alive.
        moq::fabric::PeerManager peers(moq::fabric::examples::MakeConfig(options.common, logger));
        struct StopPublisher
        {
            ProducerApp& app;
            ~StopPublisher() { app.Stop(); }
        } stop_publisher{ app };

        app.Attach(peers);
        app.Start();
        peers.SetDelegate(&app);
        peers.Announce(track, moq::fabric::Announcement::kTrack);
        logger->info("producing {} as stub, listening on {}",
                     moq::fabric::examples::FormatTrack(track),
                     options.common.listen_port);

        moq::fabric::examples::WaitForInterrupt();
        logger->info("stopping");
        peers.SetDelegate(nullptr);
        spdlog::drop(logger->name());
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        std::cerr << moq::fabric::examples::HelpText(ExampleKind::kProducer);
        return 1;
    }
    return 0;
}
