#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../classic/tpc_command.h"
#include "commo.h"

namespace janus {

struct Proposal {
  uint64_t priority;
  uint64_t proposerId;
  uint64_t value;
  Proposal() : priority(0), proposerId(0), value(0) {}
  Proposal(uint64_t p, uint64_t pid, uint64_t v) : priority(p), proposerId(pid), value(v) {}
  bool operator==(const Proposal& other) const {
      return priority == other.priority &&
              proposerId == other.proposerId &&
              value == other.value;
  }
};

struct SlotState {
  uint64_t currentStep;
  Proposal Fc;
  Proposal Ac;
  Proposal Ap;
  SlotState() : currentStep(0), Fc(0, 0, 0), Ac(0, 0, 0), Ap(0, 0, 0) {}
};

class QuePaxaServer : public TxLogServer {
 public:
uint64_t generateRandomPriority();
 void propose(uint64_t &value);
 void intervalSummaryRegister(const uint64_t &step, const Proposal &proposal, SlotState *slotState);
 Proposal findBestOfFirstProposals(const vector<SlotState>& replies);
 Proposal findBestOfAggregateProposals(const vector<SlotState>& replies);
 Proposal findMaxStepProposal(const vector<SlotState>& replies);
 uint64_t findMaxStep(const vector<SlotState>& replies);
private:
uint64_t leader_id_ = 1; 
uint64_t proposerId = loc_id_;
 
 public:
  QuePaxaServer(Frame *frame) ;
  ~QuePaxaServer() ;
  


 private:
  bool disconnected_ = false;
  void Setup();

 public:
  void Disconnect(const bool disconnect = true);
  void Reconnect() {
    Disconnect(false);
  }
  bool IsDisconnected();

  virtual bool HandleConflicts(Tx& dtxn,
                               innid_t inn_id,
                               vector<string>& conflicts) {
    verify(0);
  };

  QuePaxaCommo* commo() {
    return (QuePaxaCommo*)commo_;
  }

};
} // namespace janus
