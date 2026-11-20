// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include <moq/fabric/peer_manager.h>

#include "fabric/engine.h"
#include "fabric/messages/node_info.h"
#include "fabric/name.h"

namespace moq::fabric {
    std::string FormatNodeId(std::uint64_t id)
    {
        return NodeId().Value(id);
    }

    struct PeerManager::Impl
    {
        explicit Impl(Config config)
          : engine(std::move(config))
        {
        }

        Engine engine;
    };

    PeerManager::PeerManager(const Config& config)
      : impl_(std::make_unique<Impl>(config))
    {
    }

    PeerManager::~PeerManager() = default;

    void PeerManager::SetDelegate(Delegate* delegate)
    {
        impl_->engine.SetDelegate(delegate);
    }

    void PeerManager::Announce(const TrackName& name, Announcement kind, Encoding encoding)
    {
        impl_->engine.Announce(name, kind, encoding, false);
    }

    void PeerManager::Withdraw(const TrackName& name, Announcement kind)
    {
        // The engine sends the encoding stored with the publish. This value is used only when
        // that publish is already gone, so the withdraw message still carries a known draft.
        impl_->engine.Announce(name, kind, Encoding::kMoqt18, true);
    }

    void PeerManager::Subscribe(const TrackName& name, const SubscribeParams& params)
    {
        impl_->engine.Subscribe(name, params);
    }

    void PeerManager::UpdateSubscribe(const TrackName& name, const SubscribeParams& params)
    {
        impl_->engine.UpdateSubscribe(name, params);
    }

    void PeerManager::Unsubscribe(std::uint64_t track_hash)
    {
        impl_->engine.Unsubscribe(track_hash);
    }

    bool PeerManager::HasSubscribers(std::uint64_t track_hash) const
    {
        return impl_->engine.HasSubscribers(track_hash);
    }

    std::vector<PeerSnapshot> PeerManager::Peers()
    {
        return impl_->engine.Peers();
    }

    std::uint64_t PeerManager::LocalNodeId() const
    {
        return impl_->engine.LocalNodeId();
    }

    PeerAdminResult PeerManager::AddPeer(const std::string& host,
                                         std::uint16_t port,
                                         std::optional<std::uint64_t> metric)
    {
        return impl_->engine.AddPeer(host, port, metric);
    }

    PeerAdminResult PeerManager::RemovePeer(const std::string& host, std::uint16_t port)
    {
        return impl_->engine.RemovePeer(host, port);
    }

    PeerAdminResult PeerManager::RestartPeer(const std::string& host, std::uint16_t port)
    {
        return impl_->engine.RestartPeer(host, port);
    }

    PeerAdminResult PeerManager::SetPeerMetric(const std::string& host, std::uint16_t port, std::uint64_t metric)
    {
        return impl_->engine.SetPeerMetric(host, port, metric);
    }

    void PeerManager::SendData(std::uint64_t track_hash,
                               uint8_t priority,
                               uint32_t ttl_ms,
                               DataType type,
                               uint64_t group_id,
                               uint64_t subgroup_id,
                               std::shared_ptr<const std::vector<uint8_t>> data)
    {
        impl_->engine.SendData(track_hash, priority, ttl_ms, type, group_id, subgroup_id, std::move(data));
    }

    void PeerManager::EndSubgroup(std::uint64_t track_hash, uint64_t group_id, uint64_t subgroup_id, bool reset)
    {
        impl_->engine.EndSubgroup(track_hash, group_id, subgroup_id, reset);
    }

    std::uint64_t PeerManager::HashTrack(const TrackName& name)
    {
        return TrackHash(name).track_fullname_hash;
    }

} // namespace moq::fabric
