
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
shared_ptr<RecorderQuorumEvent>
QuePaxaCommo::SendToRecoder(parid_t par_id, siteid_t site_id, const uint64_t& curSlot, const uint64_t& step, Proposal proposal){
  auto proxies = rpc_par_proxies_[par_id];
  auto ev = Reactor::CreateSpEvent<RecorderQuorumEvent>(NSERVERS, NSERVERS/2);
  for (auto& p : proxies) {
    if (p.first != site_id) {
      QuePaxaProxy *proxy = (QuePaxaProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [ev](Future* fu) {
        string slotStateData = "";
        fu->get_reply() >> slotStateData;
        if (slotStateData != ""){  
          std::stringstream ss(slotStateData);
          SlotState state;
          boost::archive::text_iarchive ia(ss);
          ia >> state;
          ev->VoteYes(state);
        }
        else{
          ev->VoteNo();
        }
      };
      if (step%4 == 0 && (step>4 || site_id!=cur_leader)){
        proposal.priority = (rand() % 100) - 1;
      }
      std::stringstream ss;
      boost::archive::text_oarchive oa(ss);
      oa << proposal;
      string proposalData = ss.str();

      /* wrap Marshallable in a MarshallDeputy to send over RPC */
      Call_Async(proxy, SendToRecoderRpc, curSlot, step, proposalData, fuattr);
      }
  }
  return ev;
}

shared_ptr<IntEvent>
QuePaxaCommo::SendCommit(parid_t par_id, siteid_t site_id,shared_ptr<Marshallable> cmd){
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
      Call_Async(proxy, SendCommitRpc, md, fuattr);
    }
  }
  return ev;
}
} // namespace janus
