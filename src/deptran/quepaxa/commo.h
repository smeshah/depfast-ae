#pragma once

#include "../__dep__.h"
#include "../constants.h"

#include "../communicator.h"


namespace janus {


class TxData;
class QuePaxaCommo : public Communicator {
 public:
  QuePaxaCommo() = delete;
  QuePaxaCommo(PollMgr*);

  shared_ptr<IntEvent> 
  SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res);

  shared_ptr<IntEvent>
  SendToRecoder(parid_t par_id, siteid_t site_id, const uint64_t& curSlot, const uint64_t& step, const string& proposalData, string* slotStateData);

  shared_ptr<IntEvent>
  SendCommit(parid_t par_id, siteid_t site_id, shared_ptr<Marshallable> cmd);
                    
                     
  /* Do not modify this class below here */

 public:
  #ifdef QUEPAXA_TEST_CORO
  std::recursive_mutex rpc_mtx_ = {};
  uint64_t rpc_count_ = 0;
  #endif
};

} // namespace janus

