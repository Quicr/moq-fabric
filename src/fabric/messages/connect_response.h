// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include "fabric/common.h"
#include "fabric/errors.h"
#include <optional>

#include "node_info.h"

namespace moq::fabric {

    /**
     * @brief Fabric connect response message
     *
     * @details Fabric connect response message is sent by the the server side of the peer in response
     *    to a connect message.
     */
    class ConnectResponse : public CommonHeaders
    {
      public:
        ProtocolError error{
            ProtocolError::kNoError
        }; ///< NoError or error value for connect. If error, node_info is not set

        std::optional<NodeInfo> node_info; ///< This node information of server accepting the connection

        /*
         * Parameters
         */

        /**
         * @brief Serialize the message into wire format to be transmitted
         */
        std::vector<uint8_t> Serialize() const;

        ConnectResponse() = default;
        ConnectResponse(std::span<uint8_t const> serialized_data);

        uint32_t SizeBytes() const;

      private:
    };

} // namespace moq::fabric