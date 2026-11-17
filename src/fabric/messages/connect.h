// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "fabric/common.h"

#include "node_info.h"

namespace moq::fabric {

    /**
     * @brief Fabric connect message
     *
     * @details Fabric connect message is sent by the client side of the peer
     */
    class Connect
    {
      public:
        PeerMode mode;      ///< Relay fabric mode
        NodeInfo node_info; ///< node information of client making the connection

        /*
         * Parameters
         */

        /**
         * @brief Encode node object into bytes that can be written on the wire
         */
        std::vector<uint8_t> Serialize() const;

        Connect() = default;
        Connect(PeerMode mode, const NodeInfo& node_info);
        Connect(std::span<uint8_t const> serialized_data);

        uint32_t SizeBytes() const;

      private:
    };

} // namespace moq::fabric