#pragma once

#include "__dep__.h"
#include "quepaxa_rpc.h"
#include "server.h"
#include "macros.h"
// #include "constants.h"
// #include "../rcc/graph.h"
// #include "../rcc/graph_marshaler.h"
// #include "../command.h"
// #include "../procedure.h"
// #include "../command_marshaler.h"


namespace janus {

class TxLogServer;
class QuePaxaServer;
class QuePaxaServiceImpl : public QuePaxaService {
 public:
  QuePaxaServer* svr_;
  QuePaxaServiceImpl(TxLogServer* sched);
  RpcHandler(Start, 1,
             const MarshallDeputy&, md_cmd) {};
  RpcHandler(HelloRpc, 2, const string&, req, string*, res) {
    *res = ""; 
  };

  RpcHandler(SendToRecoderRpc, 4,const uint64_t&, curSlot, const uint64_t&, step,  const string&, proposalData, string*, slotStateData) {
    *slotStateData = ""; 
  };
  RpcHandler(SendCommitRpc, 1, const MarshallDeputy&, md_cmd) {
  };
  RpcHandler(CollectMetrics, 3,
             uint64_t*, fast_path_count,
             vector<double>*, commit_times,
             vector<double>*, exec_times) {
    *fast_path_count = 0;
    *commit_times = vector<double>();
    *exec_times = vector<double>();
  }
};

} // namespace janus
