#include "server.h"
#include "frame.h"


namespace janus {

QuePaxaServer::QuePaxaServer(Frame * frame) {
  frame_ = frame;
  /* Your code here for server initialization. Note that this function is 
     called in a different OS thread. Be careful about thread safety if 
     you want to initialize variables here. */
  // TODO: REMOVE
}

QuePaxaServer::~QuePaxaServer() {
  /* Your code here for server teardown */

}

void QuePaxaServer::Setup() {
  /* Your code here for server setup. Due to the asynchronous nature of the 
     framework, this function could be called after a RPC handler is triggered. 
     Your code should be aware of that. This function is always called in the 
     same OS thread as the RPC handlers. */

  // Process requests
  // Coroutine::CreateRun([this](){
    
  //   for (int i = 0; i < 5; i++){
  //       string res;

  //       auto event = commo()->SendString(0, /* partition id is always 0 for lab1 */
  //                                       0, "hello", &res);
  //       event->Wait(1000000); //timeout after 1000000us=1s
  //       if (event->status_ == Event::TIMEOUT) {
  //         Log_info("timeout happens sample");
  //       } else {
  //         Log_info("rpc response is: %s", res.c_str()); 
  //       }
  //   }
  // });

   Coroutine::CreateRun([this](){
    while(true) {
      mtx_.lock();
      int size = reqs.size();
      mtx_.unlock();
      // Future Work: Can make more efficient by having a pub-sub kind of thing
      if (size == 0) {
        Coroutine::Sleep(1000);
      } else {
        mtx_.lock();
        int req = reqs.front();
        reqs.pop_front();
        mtx_.unlock();
        Log_info("Processing request %d", req);
        Coroutine::CreateRun([this,&req](){
          propose(req);
        });
      }
    }

  });
}

void QuePaxaServer::GetState(uint64_t *result) {
  /* Your code here. This function can be called from another OS thread. */
  Log_info("GetState method called");

}

void QuePaxaServer::Start(shared_ptr<Marshallable> &cmd, uint64_t *index) {
  /* Your code here. This function can be called from another OS thread. */
  // pending_values.push(value);
  // Log_info("Start method called with value %lu", value);
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    shared_ptr<TpcCommitCommand> tpcCmd=std::dynamic_pointer_cast<TpcCommitCommand>(cmd);
    reqs.push_back((int)(tpcCmd->tx_id_));
    Log_info("Start method called with value %lu on loc id %d", tpcCmd->tx_id_, loc_id_);
    *index = curSlot;
    curSlot++;
}


void QuePaxaServer::handleCommit(const uint64_t &slot, shared_ptr<Marshallable> &cmd) {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    curSlot++;
    app_next_(*cmd);
    // committedValues[slot] = cmd;
}

void QuePaxaServer::intervalSummaryRegister(const uint64_t &step, const string &proposalData, string *slotStateData) {
        
    std::stringstream ss(proposalData);
    Proposal proposal;
    boost::archive::text_iarchive ia(ss);
    ia >> proposal;
    
    SlotState state = slotStates[curSlot];
    if (state.currentStep == step){
        // state.Ac.value = max(state.Ac.value, proposal.value);
        if (proposal.priority > state.Ac.priority){
            state.Ac = proposal;
        }
    }
    else if (state.currentStep < step){
        if (step == state.currentStep + 1){
            state.Ap = state.Ac;
        }
        else {
            state.Ap = Proposal(0, 0, 0);
        }
        state.currentStep = step;
        state.Fc = proposal;
        state.Ac = proposal;
    }
    // slotStates[curSlot] = state;

    std::stringstream ss2;
    boost::archive::text_oarchive oa(ss2);
    oa << state;
    *slotStateData = ss2.str();
}

void QuePaxaServer::propose(const uint64_t &value) {

  Log_info("Propose method called with value %lu", value);
  slotStates[curSlot] = SlotState();
  uint64_t s = 4 * 1 + 0;
  uint64_t H = 100;
  Proposal p(H, proposerId, value);
  vector<Proposal> proposals;
  for (int i = 0; i < 5; i++){
    proposals.push_back(p);
  }
  while (true){
    Coroutine::Sleep(20000);
    vector<SlotState> replies;
    for (int i = 0; i < 5; i++){
      replies.push_back(SlotState());
    }
    // Phase 0: If not leader, then propose with random priority
    if (s%4 == 0 && (s>4 || loc_id_!=leader_id_)){
       for (int i = 0; i < 5; i++){
         proposals[i].priority = generateRandomPriority();
       }
    }
    // Send record(step, proposal) to all recorders
    for (int i = 0; i < 5; i++){
      if (i == loc_id_){
        continue;
      }

      std::stringstream ss;
      boost::archive::text_oarchive oa(ss);
      oa << proposals[i];
      string proposalData = ss.str();

      string slotStateData = "";

      auto event = commo()->SendToRecoder(0, i, s, proposalData, &slotStateData);
      event->Wait(100000);
      Log_info("Slot state from %d data is %s", i, slotStateData.c_str());
      if (slotStateData != ""){
        std::stringstream ss2(slotStateData);
        SlotState state;
        boost::archive::text_iarchive ia(ss2);
        ia >> state;
        replies[i] = state;

      }
    }
    // Process replies (step, Fc, Ap)
    // Check if all replies have the same step
    bool allRepliesHaveSameStep = true;
    for (int i = 0; i < 5; i++){
      if ( i!=loc_id_ && replies[i].currentStep != s){
        allRepliesHaveSameStep = false;
        break;
      }
    }
    if (allRepliesHaveSameStep == true){
      // Check if all replies have the same priority
      if (s%4 == 0){
        bool allRepliesHaveSamePriority = true;
        for (int i = 0; i < 5; i++){
          if (i!=loc_id_ && replies[i].Fc.priority != proposals[i].priority){
            allRepliesHaveSamePriority = false;
            break;
          }
        }
        // If all replies have the same priority, then choose the value with the highest priority
        // Commit the value and return
        if (allRepliesHaveSamePriority == true){
            Proposal chosenProposal = proposals[loc_id_];
            uint64_t value = chosenProposal.value;
            Log_info("Value chose is %d", value);
            committedValues[curSlot] = value;
            auto cmdptr_m = convertValueToCommand(value);
            app_next_(*cmdptr_m);
            for (int i = 0; i < 5; i++){
              if (i!=loc_id_){
                commo()->SendCommit(0, i, curSlot, cmdptr_m);
              }
            }
            return;
        }
        // If all replies do not have the same priority, then choose the value with the highest priority
        else {
            p = findBestOfFirstSeenProposals(replies);
        }
      }
      // Phase 1, Spread E existent proposal (best proposal p) to all recorders
      // if (s%4 == 1){
      //   continue;
      // }
      // Phase 2, Gather E existent proposal from all recorders, spread C common proposal to all recorders
      if (s%4 == 2){
        Proposal bestOfAggregateProposal = findBestOfAggregateProposals(replies);
        if (p == bestOfAggregateProposal){
            uint64_t value = p.value;
            Log_info("Value chose is %d", value);
            committedValues[curSlot] = value;
            auto cmdptr_m = convertValueToCommand(value);
            app_next_(*cmdptr_m);
            for (int i = 0; i < 5; i++){
              if (i!=loc_id_){
                commo()->SendCommit(0, i, curSlot, cmdptr_m);
              }
            }
            return; 
        }
        // Spread C common proposal to all recorders ???
      }
      // Phase 3, Gather C common proposal from all recorders
      if (s%4 == 3){
        p = findBestOfAggregateProposals(replies); 
      }
      s += 1;
    }
    else if (allRepliesHaveSameStep == false){
      Proposal maxStepProposal = findMaxStepProposal(replies);
      p = maxStepProposal;
      s = findMaxStep(replies);
    }
  }
}

uint64_t QuePaxaServer::generateRandomPriority() {
  return (rand() % 100);
}

Proposal QuePaxaServer::findBestOfFirstSeenProposals(const vector<SlotState>& replies) {
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

shared_ptr<Marshallable> QuePaxaServer::convertValueToCommand(uint64_t value) {
    auto cmdptr = std::make_shared<TpcCommitCommand>();
    auto vpd_p = std::make_shared<VecPieceData>();
    vpd_p->sp_vec_piece_data_ = std::make_shared<vector<shared_ptr<SimpleCommand>>>();
    cmdptr->tx_id_ = value;
    cmdptr->cmd_ = vpd_p;
    auto cmdptr_m = dynamic_pointer_cast<Marshallable>(cmdptr);
    return cmdptr_m;
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
