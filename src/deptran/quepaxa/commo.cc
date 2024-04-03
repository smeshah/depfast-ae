
#include "commo.h"
#include "macros.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "../procedure.h"
#include "quepaxa_rpc.h"
#include "../command_marshaler.h"

namespace janus {
QuePaxaCommo::QuePaxaCommo(PollMgr* poll) : Communicator(poll) {
}

shared_ptr<IntEvent> 
QuePaxaCommo::SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res) {
  auto ev = Reactor::CreateSpEvent<IntEvent>();
  return ev;
}

} // namespace janus
