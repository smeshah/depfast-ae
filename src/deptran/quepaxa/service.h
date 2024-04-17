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
  RpcHandler(HelloRpc, 2, const string&, req, string*, res) {
    *res = "error"; 
  };

  RpcHandler(SendToRecoderRpc, 3, const uint64_t&, step, const string&, proposalData, string*, slotStateData) {
    *slotStateData = "error"; 
  };
  RpcHandler(SendCommitRpc, 2, const uint64_t&, slot, const MarshallDeputy&, md_cmd) {
  };
  
};

} // namespace janus
