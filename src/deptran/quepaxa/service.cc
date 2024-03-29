#include "../marshallable.h"
#include "service.h"
#include "server.h"

namespace Janus {
    QuePaxaServiceImpl :: QuePaxaServiceImpl(QuePaxaServer *server) : server(server) {
        // Empty
    }
    void QuePaxaServiceImpl :: HandleSendToRecoder(int step, const Proposal& proposal, SlotState& slotState){
        // Empty
    }
}