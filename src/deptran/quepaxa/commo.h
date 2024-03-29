#include "commo.h"
#include "macros.h"
#include "server.h"

namespace Janus {
    QuePaxaCommo :: QuePaxaCommo(PollMgr* poll) : Communicator(poll) {
        // Empty
    }
    SlotState QuePaxaCommo :: SendToRecorders(int step, const Proposal& proposal){
        // Empty
    }
    // commit?
}