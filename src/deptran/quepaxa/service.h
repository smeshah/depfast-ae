#pragma once

#include "__dep__.h"
#include "constants.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "deptran/procedure.h"
#include "../command_marshaler.h"
#include "quepaxa_rpc.h"
#include "server.h"
#include "macros.h"


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

};

} // namespace janus
