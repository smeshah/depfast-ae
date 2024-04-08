#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../classic/tpc_command.h"
#include "commo.h"
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

class Proposal {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & priority;
        ar & proposerId;
        ar & value;
    }

public:
    Proposal() : priority(0), proposerId(0), value(0) {}
    Proposal(uint64_t p, uint64_t pid, uint64_t v) : priority(p), proposerId(pid), value(v) {}

    bool operator==(const Proposal& other) const {
        return priority == other.priority &&
               proposerId == other.proposerId &&
               value == other.value;
    }

public:
    uint64_t priority;
    uint64_t proposerId;
    uint64_t value;
};

class SlotState {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & currentStep;
        ar & Fc;
        ar & Ac;
        ar & Ap;
    }

public:
    SlotState() : currentStep(0), Fc(0, 0, 0), Ac(0, 0, 0), Ap(0, 0, 0) {}

public:
    uint64_t currentStep;
    Proposal Fc;
    Proposal Ac;
    Proposal Ap;
};



namespace janus {

class QuePaxaServer : public TxLogServer {
 public:
 
 unordered_map<uint64_t, SlotState> slotStates;
 uint64_t curSlot = 0;
 
 
 uint64_t generateRandomPriority();
 void propose(uint64_t &value);
 void intervalSummaryRegister(const uint64_t &step, const string &proposalData,  string *slotStateData);
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
