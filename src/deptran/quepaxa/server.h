#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../scheduler.h"
#include "commo.h"
#include "../classic/tpc_command.h"

namespace janus {


class QuePaxaCommitMarshallable : public Marshallable {
 public:
  QuePaxaCommitMarshallable() : Marshallable(MarshallDeputy::CMD_QUEPAXA_COMMIT) {}  
  uint64_t slot;
  uint64_t value;
  Marshal& ToMarshal(Marshal& m) const override {
    m << slot;
    m << value;
    return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
    m >> slot;
    m >> value;
    return m;
  }
};

class QuePaxaServer : public TxLogServer {
 public:
  /* do not modify this class below here */
 unordered_map<uint64_t, SlotState> slotStates;
 unordered_map<uint64_t, uint64_t> committedValues;
 std::list<std::pair<int, int>> reqs;
 uint64_t curSlot = 0;
 map<uint64_t, Timer> start_times;
 vector<double> commit_times;
 map<uint64_t, function<void()>> callbacks;
 int fast = 0;
 int slow = 0;
#ifdef QUEPAXA_TEST_CORO
int commit_timeout = 300000;
int rpc_timeout = 2000000;
#else
int commit_timeout = 10000000; // 10 seconds
int rpc_timeout = 5000000; // 5 seconds
#endif
 uint64_t generateRandomPriority();
 void propose(const uint64_t &slot, const uint64_t &value);
 void intervalSummaryRegister(const uint64_t& curSlot, const uint64_t &step, const string &proposalData,  string *slotStateData);
 Proposal findBestOfFirstSeenProposals(const vector<SlotState>& replies);
 Proposal findBestOfAggregateProposals(const vector<SlotState>& replies);
 Proposal findMaxStepProposal(const vector<SlotState>& replies);
 uint64_t findMaxStep(const vector<SlotState>& replies);
 void handleCommit(shared_ptr<Marshallable> &cmd);
 shared_ptr<Marshallable> convertValueToCommand(uint64_t slot, uint64_t value);
 void commitChosenValue(uint64_t slot, uint64_t value);
 bool checkAlreadyCommitted(uint64_t slot, uint64_t value);

 private:
  uint64_t proposerId = loc_id_;
  ROLE role = RECORDER;

 public:
  QuePaxaServer(Frame *frame) ;
  ~QuePaxaServer() ;

  void Start(shared_ptr<Marshallable>& cmd, uint64_t *index, const function<void()> &cb);
  void Start(shared_ptr<Marshallable>& cmd, uint64_t *index);
  void GetState(uint64_t slot, uint64_t *result);
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
