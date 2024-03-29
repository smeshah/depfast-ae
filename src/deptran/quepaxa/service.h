#pragma once

#include "__dep__.h"
#include "quepaxa_rpc.h"
#include "server.h"
#include "macros.h"

namespace Janus{
    class QuePaxaServer;

    class QuePaxaServiceImpl : public QuePaxaService {
        public:
        QuePaxaServer *server;

        RpcHandler(SendToRecoder, 3, int step, const Proposal& proposal, SlotState& slotState){
        }

    };
}