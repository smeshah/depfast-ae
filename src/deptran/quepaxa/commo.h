#pragma once

#include "../__dep__.h"
#include "../communicator.h"
#include "quepaxa_rpc.h"


namespace janus {

#define NSERVERS 5

class TxData;
class QuePaxaCommo : public Communicator {
 public:
  QuePaxaCommo() = delete;
  QuePaxaCommo(PollMgr*);

  shared_ptr<IntEvent> 
  SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res);

                                  
  /* Do not modify this class below here */

 public:
  #if defined(QUEPAXA_TEST_CORO)
  std::recursive_mutex rpc_mtx_ = {};
  uint64_t rpc_count_ = 0;
  #endif
};

} // namespace janus
