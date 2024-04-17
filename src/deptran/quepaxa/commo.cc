
#include "commo.h"
#include "quepaxa_rpc.h"
#include "macros.h"
// #include "../rcc/graph.h"
// #include "../rcc/graph_marshaler.h"
// #include "../command.h"
// #include "../procedure.h"
// #include "../command_marshaler.h"


namespace janus {

QuePaxaCommo::QuePaxaCommo(PollMgr* poll) : Communicator(poll) {
}

shared_ptr<IntEvent> 
QuePaxaCommo::SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res) {
  auto proxies = rpc_par_proxies_[par_id];
  auto ev = Reactor::CreateSpEvent<IntEvent>();
  for (auto& p : proxies) {
    if (p.first == site_id) {
      QuePaxaProxy *proxy = (QuePaxaProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [res,ev](Future* fu) {
        fu->get_reply() >> *res;
        ev->Set(1);
      };
      /* wrap Marshallable in a MarshallDeputy to send over RPC */
      Call_Async(proxy, HelloRpc, msg, fuattr);
    }
  }
  return ev;
}
shared_ptr<IntEvent>
QuePaxaCommo::SendToRecoder(parid_t par_id, siteid_t site_id, const uint64_t& step, const string& proposalData, string* slotStateData){
  auto proxies = rpc_par_proxies_[par_id];
  auto ev = Reactor::CreateSpEvent<IntEvent>();
  for (auto& p : proxies) {
    if (p.first == site_id) {
      QuePaxaProxy *proxy = (QuePaxaProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [slotStateData,ev](Future* fu) {
        fu->get_reply() >> *slotStateData;
        ev->Set(1);
      };
      /* wrap Marshallable in a MarshallDeputy to send over RPC */
      Call_Async(proxy, SendToRecoderRpc, step, proposalData, fuattr);
    }
  }
  return ev;
}

shared_ptr<IntEvent>
QuePaxaCommo::SendCommit(parid_t par_id, siteid_t site_id, const uint64_t& slot, shared_ptr<Marshallable> cmd){
  auto proxies = rpc_par_proxies_[par_id];
  auto ev = Reactor::CreateSpEvent<IntEvent>();
  for (auto& p : proxies) {
    if (p.first == site_id) {
      QuePaxaProxy *proxy = (QuePaxaProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [ev](Future* fu) {
        ev->Set(1);
      };
      MarshallDeputy md(cmd);
      /* wrap Marshallable in a MarshallDeputy to send over RPC */
      Call_Async(proxy, SendCommitRpc, slot, md, fuattr);
    }
  }
  return ev;
}
} // namespace janus
