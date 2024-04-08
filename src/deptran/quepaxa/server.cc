#include "server.h"
#include "frame.h"
#if defined(QUEPAXA_TEST_CORO)
#include "../classic/tpc_command.h"
#endif


namespace janus {

QuePaxaServer::QuePaxaServer(Frame * frame) {
  frame_ = frame;
}

QuePaxaServer::~QuePaxaServer() {}

/***********************************
   Main event processing loop      *
************************************/

void QuePaxaServer::Setup() {
}

void QuePaxaServer::intervalSummaryRegister(const uint64_t &step, const string &proposalData, string *slotStateData) {
        

    std::stringstream ss(proposalData);
    Proposal proposal;
    boost::archive::text_iarchive ia(ss);
    ia >> proposal;
    
    
    SlotState state = slotStates[curSlot];
    if (state.currentStep == step){
        state.Ac.value = max(state.Ac.value, proposal.value);
    }
    else if (state.currentStep < step){
        if (step == state.currentStep + 1){
            state.Ap = state.Ac;
        }
        else {
            state.Ap = Proposal(0, 0, 0);
        }
        state.currentStep = step;
        state.Fc.value = proposal.value;
        state.Ac.value = proposal.value;
    }
    slotStates[curSlot] = state;

    std::stringstream ss2;
    boost::archive::text_oarchive oa(ss2);
    oa << state;
    *slotStateData = ss2.str();
}

void QuePaxaServer::propose(uint64_t &value) {
  uint64_t s = 4 * 1 + 0;
  uint64_t H = 0;
  Proposal p(H, proposerId, value);
  vector<Proposal> proposals;
  for (int i = 0; i < 5; i++){
    proposals.push_back(p);
  }
  while (true){
    vector<SlotState> replies;
    for (int i = 0; i < 5; i++){
      replies.push_back(SlotState());
    }
    for (int i = 0; i < 5; i++){
      if (i == loc_id_){
        continue;
      }
      if (s%4 == 0 && (s>4 || i!=leader_id_)){
        proposals[i].priority = generateRandomPriority(); 
      }
      std::stringstream ss;
      boost::archive::text_oarchive oa(ss);
      oa << proposals[i];
      string proposalData = ss.str();

      string slotStateData;


      intervalSummaryRegister(s, proposalData, &slotStateData);
      std::stringstream ss2(slotStateData);
      SlotState state;
      boost::archive::text_iarchive ia(ss2);
      ia >> state;
      replies[i] = state;

    }
    bool allRepliesHaveSameStep = true;
    for (int i = 0; i < 5; i++){
      if ( i!=loc_id_ && replies[i].currentStep != s){
        allRepliesHaveSameStep = false;
        break;
      }
    }
    if (allRepliesHaveSameStep == true){
      if (s%4 == 0){
        bool allRepliesHaveSamePriority = true;
        for (int i = 0; i < 5; i++){
          if (i!=loc_id_ && replies[i].Fc.priority != proposals[i].priority){
            allRepliesHaveSamePriority = false;
            break;
          }
        }
        if (allRepliesHaveSamePriority == true){
            Proposal chosenProposal = proposals[loc_id_];
            uint64_t value = chosenProposal.value;
            Log_info("Value chose is %d", value);
            return;
        }
        else {
            p = findBestOfFirstProposals(replies);
        }
      }
      if (s%4 == 1){
        continue;
      }
      if (s%4 == 2){
        Proposal bestOfAggregateProposal = findBestOfAggregateProposals(replies);
        if (p == bestOfAggregateProposal){
            uint64_t value = p.value;
            Log_info("Value chose is %d", value);
            return; 
        }
      }
      if (s%4 == 3){
        p = findBestOfAggregateProposals(replies); 
      }
    }
    else if (allRepliesHaveSameStep == false){
      Proposal maxStepProposal = findMaxStepProposal(replies);
      p = maxStepProposal;
      s = findMaxStep(replies);
    }
  }
}

uint64_t QuePaxaServer::generateRandomPriority() {
  return 1 + (rand() % 100);
}

Proposal QuePaxaServer::findBestOfFirstProposals(const vector<SlotState>& replies) {
    int bestIndex = -1;
    uint64_t highestPriority = 0;

    for (int i = 0; i < replies.size(); ++i) {
        if (i != loc_id_ && replies[i].Fc.priority > highestPriority) {
            bestIndex = i;
            highestPriority = replies[i].Fc.priority;
        }
    }

    if (bestIndex != -1) {
        return Proposal(replies[bestIndex].Fc.priority, proposerId, replies[bestIndex].Fc.value);
    } else {
        return Proposal();
    }
}


Proposal QuePaxaServer::findBestOfAggregateProposals(const vector<SlotState>& replies) {
    int bestIndex = -1;
    uint64_t highestPriority = 0;

    for (int i = 0; i < replies.size(); ++i) {
        if (replies[i].Ac.priority > highestPriority) {
            bestIndex = i; 
            highestPriority = replies[i].Ac.priority;
        }
    }

    if (bestIndex != -1) {
        return replies[bestIndex].Ac;
    } else {
        return Proposal(0, 0, 0);
    }
}

Proposal QuePaxaServer::findMaxStepProposal(const vector<SlotState>& replies) {
    uint64_t maxStep = 0;
    Proposal maxStepProposal;

    for (const SlotState& reply : replies) {
        if (reply.currentStep > maxStep) {
            maxStep = reply.currentStep;
            maxStepProposal = reply.Fc;
        }
    }

    return maxStepProposal;
}

uint64_t QuePaxaServer::findMaxStep(const vector<SlotState>& replies) {
    uint64_t maxStep = 0;

    for (const SlotState& reply : replies) {
        if (reply.currentStep > maxStep) {
            maxStep = reply.currentStep;
        }
    }

    return maxStep;
}

/* Do not modify any code below here */

void QuePaxaServer::Disconnect(const bool disconnect) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(disconnected_ != disconnect);
  // global map of rpc_par_proxies_ values accessed by partition then by site
  static map<parid_t, map<siteid_t, map<siteid_t, vector<SiteProxyPair>>>> _proxies{};
  if (_proxies.find(partition_id_) == _proxies.end()) {
    _proxies[partition_id_] = {};
  }
  QuePaxaCommo *c = (QuePaxaCommo*) commo();
  if (disconnect) {
    verify(_proxies[partition_id_][loc_id_].size() == 0);
    verify(c->rpc_par_proxies_.size() > 0);
    auto sz = c->rpc_par_proxies_.size();
    _proxies[partition_id_][loc_id_].insert(c->rpc_par_proxies_.begin(), c->rpc_par_proxies_.end());
    c->rpc_par_proxies_ = {};
    verify(_proxies[partition_id_][loc_id_].size() == sz);
    verify(c->rpc_par_proxies_.size() == 0);
  } else {
    verify(_proxies[partition_id_][loc_id_].size() > 0);
    auto sz = _proxies[partition_id_][loc_id_].size();
    c->rpc_par_proxies_ = {};
    c->rpc_par_proxies_.insert(_proxies[partition_id_][loc_id_].begin(), _proxies[partition_id_][loc_id_].end());
    _proxies[partition_id_][loc_id_] = {};
    verify(_proxies[partition_id_][loc_id_].size() == 0);
    verify(c->rpc_par_proxies_.size() == sz);
  }
  disconnected_ = disconnect;
}

bool QuePaxaServer::IsDisconnected() {
  return disconnected_;
}

} // namespace janus
