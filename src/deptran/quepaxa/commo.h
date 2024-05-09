#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "proposal_slot.h"
#include "../communicator.h"

#define NSERVERS 5
#define cur_leader 3


namespace janus {

class RecorderQuorumEvent : public QuorumEvent {

public:
    vector<SlotState> replies;
    RecorderQuorumEvent(int n_total, int quorum) : QuorumEvent(n_total, quorum) {
        //replies.resize(n_total);
    }

    void VoteYes(SlotState& reply){
        replies.push_back(reply);
        this->QuorumEvent::VoteYes();
    }
    
    void VoteNo(){
        // replies.push_back(reply);
        this->QuorumEvent::VoteNo();
    }
};

class TxData;
class QuePaxaCommo : public Communicator {
 public:
  QuePaxaCommo() = delete;
  QuePaxaCommo(PollMgr*);

  shared_ptr<IntEvent> 
  SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res);

  shared_ptr<RecorderQuorumEvent>
  SendToRecoder(parid_t par_id, siteid_t site_id, const uint64_t& curSlot, const uint64_t& step, Proposal proposal);

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

