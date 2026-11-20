// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

// Dials a relay as a stub, subscribes to one track, and prints objects from a producer.

#include "example_util.h"

#include <iostream>
#include <stdexcept>

namespace {

    class SubscriberApp : public moq::fabric::Delegate
    {
      public:
        SubscriberApp(std::shared_ptr<spdlog::logger> logger, moq::fabric::TrackName track)
          : logger_(std::move(logger))
          , track_(std::move(track))
          , track_hash_(moq::fabric::PeerManager::HashTrack(track_))
        {
        }

        void OnPeerUp(std::uint64_t session_id) override { logger_->info("peer up session {}", session_id); }

        void OnPeerDown(std::uint64_t session_id) override { logger_->info("peer down session {}", session_id); }

        void OnPeerTrack(const moq::fabric::TrackName& name, moq::fabric::Encoding, bool withdrawn) override
        {
            if (name.name != track_.name || name.name_space != track_.name_space) {
                return;
            }
            logger_->info(
              "track {} {}", withdrawn ? "withdrawn" : "announced", moq::fabric::examples::FormatTrack(name));
        }

        void OnPeerData(std::uint64_t track_hash,
                        bool,
                        std::optional<std::uint64_t>,
                        std::shared_ptr<const std::vector<uint8_t>> data) override
        {
            if (track_hash != track_hash_ || !data) {
                return;
            }
            const std::string text(data->begin(), data->end());
            logger_->info("received {}", text);
        }

      private:
        std::shared_ptr<spdlog::logger> logger_;
        moq::fabric::TrackName track_;
        std::uint64_t track_hash_;
    };

} // namespace

int
main(int argc, char** argv)
{
    using moq::fabric::examples::ExampleKind;

    try {
        const auto options = moq::fabric::examples::ParseArgs(ExampleKind::kSubscriber, argc, argv);
        if (options.common.help) {
            std::cout << moq::fabric::examples::HelpText(ExampleKind::kSubscriber);
            return 0;
        }

        moq::fabric::examples::CheckCertificate(options.common.cert, options.common.key);
        auto logger = moq::fabric::examples::MakeLogger(options.common.node_id);
        const auto track = moq::fabric::examples::MakeTrack(options.name_space, options.track);
        SubscriberApp app(logger, track);

        moq::fabric::PeerManager peers(moq::fabric::examples::MakeConfig(options.common, logger));
        peers.SetDelegate(&app);

        moq::fabric::SubscribeParams params;
        params.priority = 10;
        params.delivery_timeout_ms = 1000;
        params.forward = 1;
        peers.Subscribe(track, params);
        logger->info("subscribed to {} as stub, listening on {}",
                     moq::fabric::examples::FormatTrack(track),
                     options.common.listen_port);

        moq::fabric::examples::WaitForInterrupt();
        logger->info("stopping");
        peers.SetDelegate(nullptr);
        spdlog::drop(logger->name());
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        std::cerr << moq::fabric::examples::HelpText(ExampleKind::kSubscriber);
        return 1;
    }
    return 0;
}
