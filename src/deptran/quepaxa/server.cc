#include "server.h"
#include "frame.h"


namespace janus {


static int volatile x1 =
MarshallDeputy::RegInitializer(MarshallDeputy::CMD_QUEPAXA_COMMIT,
                                     [] () -> Marshallable* {
                                       return new QuePaxaCommitMarshallable;
                                     });


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
  //         Log_debug("timeout happens sample");
  //       } else {
  //         Log_debug("rpc response is: %s", res.c_str()); 
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
        std::tuple<int, int, int> req = reqs.front();
        reqs.pop_front();
        mtx_.unlock();

        Coroutine::CreateRun([this, req]() {
            int curSlot = std::get<0>(req);
            int value = std::get<1>(req);
            int proposerId = std::get<2>(req);
            Log_debug("Processing request %d at index %d at loc_id %d", value, curSlot, loc_id_);
            propose(curSlot, value, proposerId);
        });
      }
    }

  });
}

void QuePaxaServer::GetState(uint64_t slot, uint64_t *value) {
  /* Your code here. This function can be called from another OS thread. */
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  *value = committedValues[slot];
}


// Push (curSlot, tx_id) to reqs
// send curslot back to client where command will be committed, increment curslot
void QuePaxaServer::Start(shared_ptr<Marshallable> &cmd, uint64_t *index, const function<void()> &cb) {
  /* Your code here. This function can be called from another OS thread. */
  // pending_values.push(value);
    shared_ptr<TpcCommitCommand> tpcCmd=std::dynamic_pointer_cast<TpcCommitCommand>(cmd);
    #ifdef QUEPAXA_SERVER_METRICS_COLLECTION
    start_times[(int)(tpcCmd->tx_id_)].start();
    #endif
    reqs.push_back(std::make_tuple(curSlot, (int)(tpcCmd->tx_id_), loc_id_));
    Log_debug("Start method called with value %lu on loc id %d", tpcCmd->tx_id_, loc_id_);
    *index = curSlot;
    curSlot++;
    Log_debug("Current slot is %d", curSlot);
    role = PROPOSER;
    callbacks[(int)(tpcCmd->tx_id_)] = cb;
}

#ifdef QUEPAXA_TEST_CORO
void QuePaxaServer::Start(shared_ptr<Marshallable> &cmd, uint64_t *index) {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    shared_ptr<TpcCommitCommand> tpcCmd=std::dynamic_pointer_cast<TpcCommitCommand>(cmd);
    reqs.push_back(std::make_tuple(curSlot, (int)(tpcCmd->tx_id_), loc_id_));
    Log_debug("Start method called with value %lu on loc id %d", tpcCmd->tx_id_, loc_id_);
    *index = curSlot;
    curSlot++;
    Log_debug("Current slot is %d", curSlot);
    role = PROPOSER;
}
#endif

// Propose method
void QuePaxaServer::propose(const uint64_t &slot, const uint64_t &value, const uint64_t &proposerId) {
  Log_debug("Propose method called with value %lu at slot %lu", value, slot);
  role = PROPOSER;
  uint64_t s = 4 * 1 + 0;
  uint64_t H = 100;
  Proposal proposal(H, proposerId, value);
  while (true){
    Coroutine::Sleep(10000);
    // If slot already taken by another proposer return
    if (checkAlreadyCommitted(slot, proposal)){
      return;
    }
    // Artificial delay to make leader performance is better
    if (loc_id_ != cur_leader){
      Log_debug("Not leader");
      Coroutine::Sleep(50000);
    }
 
    // Send record(step, proposal) to all recorders
    auto event = commo()->SendToRecoder(0, loc_id_, slot, s, proposal);
    event->Wait(rpc_timeout);
    // Process replies (step, Fc, Ap)
    // Check if all replies have the same step
    Log_debug("Replies size is %d", event->replies.size());
    if (event->replies.size() < 2){
      continue;
    }
    bool allRepliesHaveSameStep = true;
    for (int i = 0; i < event->replies.size(); i++){
      if (event->replies[i].currentStep!=0 && event->replies[i].currentStep != s){
        allRepliesHaveSameStep = false;
        break;
      }
    }
    if (allRepliesHaveSameStep == true){
      // Check if all replies have the same priority
      if (s%4 == 0){
        Log_debug("In Phase 0");
        bool allRepliesHaveSamePriority = true;
        for (int i = 0; i < event->replies.size(); i++){
          if (event->replies[i].currentStep!=0 && event->replies[i].Fc.priority != proposal.priority){
            allRepliesHaveSamePriority = false;
            break;
          }
        }
        // If all replies have the same priority, then choose the value with the highest priority
        // Commit the value and return
        if (allRepliesHaveSamePriority == true){
            Proposal chosenProposal = proposal;
            uint64_t value = chosenProposal.value;
            if (checkAlreadyCommitted(slot, chosenProposal)){
              role = RECORDER;
              return;
            }
            Log_debug("Value chosen is %d at index %d", value, slot);
            #ifdef QUEPAXA_SERVER_METRICS_COLLECTION
            fast++;
            #endif
            commitChosenValue(slot, chosenProposal);
            role = RECORDER;
            return;
        }
        // If all replies do not have the same priority, then choose the value with the highest priority
        else {
            Log_debug("All replies don't have same priority");
            proposal = findBestOfFirstSeenProposals(event->replies);
        }
      }
      // Phase 1, Spread E existent proposal (best proposal proposal) to all recorders
      // if (s%4 == 1){
      //   
      // }
      // Phase 2, Gather E existent proposal from all recorders, spread C common proposal to all recorders
      if (s%4 == 2){
        Log_debug("In Phase 2");
        Proposal bestOfAggregateProposal = findBestOfAggregateProposals(event->replies);
        if (proposal == bestOfAggregateProposal){
            uint64_t value = proposal.value;
            if (checkAlreadyCommitted(slot, proposal)){
              role = RECORDER;
              return;
            }
            Log_debug("Value chosen is %d at index %d", value, slot);
            #ifdef QUEPAXA_SERVER_METRICS_COLLECTION
            slow++;
            #endif
            commitChosenValue(slot, proposal);
            role = RECORDER;
            return; 
        }
        // Spread C common proposal to all recorders ???
      }
      // Phase 3, Gather C common proposal from all recorders
      if (s%4 == 3){
        Log_info("In Phase 3");
        proposal = findBestOfAggregateProposals(event->replies); 
      }
      s += 1;
    }
    else if (allRepliesHaveSameStep == false){
      Log_debug("All replies don't have same step");
      Proposal maxStepProposal = findMaxStepProposal(event->replies);
      proposal = maxStepProposal;
      s = findMaxStep(event->replies);
    }
  }
}


// receiver/recorder handlers
void QuePaxaServer::intervalSummaryRegister(const uint64_t &index, const uint64_t &step, const string &proposalData, string *slotStateData) {
    
    Log_debug("Interval summary register called with index %lu, step %lu, proposalData %s", index, step, proposalData.c_str());

    if (committedValues.find(index) != committedValues.end()){
        return;
    }
    if (role == PROPOSER){
        return;
    }
    if (slotStates.find(index) == slotStates.end()){
        slotStates[index] = SlotState();
    }

    std::stringstream ss(proposalData);
    Proposal proposal;
    boost::archive::text_iarchive ia(ss);
    ia >> proposal;
    
    SlotState state = slotStates[index];
    if (state.currentStep == step){
        if (proposal.priority > state.Ac.priority){
            state.Ac = proposal;
        }
    }
    else if (state.currentStep < step){
        if (step == state.currentStep + 1){
            state.Ap = state.Ac;
        }
        else {
            state.Ap = Proposal(-1, -1, -1);
        }
        state.currentStep = step;
        state.Fc = proposal;
        state.Ac = proposal;
    }
    slotStates[index] = state;

    std::stringstream ss2;
    boost::archive::text_oarchive oa(ss2);
    oa << state;
    *slotStateData = ss2.str();
}

void QuePaxaServer::handleCommit(shared_ptr<Marshallable> &cmd) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto state = dynamic_pointer_cast<QuePaxaCommitMarshallable>(cmd);
  Log_debug("handleCommit method called with value %lu", state->value);
  committedValues[state->slot] = state->value;
  curSlot = max(curSlot, state->slot + 1);
  #ifdef QUEPAXA_SERVER_METRICS_COLLECTION
  if (state->proposerId == loc_id_){
    commit_times.push_back(start_times[state->value].elapsed());
    callbacks[state->value]();      
  }
  #endif
  #ifdef QUEPAXA_TEST_CORO
  app_next_(*cmd);
  #endif
}

// Helper functions
uint64_t QuePaxaServer::generateRandomPriority() {
  return (rand() % 100);
}

Proposal QuePaxaServer::findBestOfFirstSeenProposals(const vector<SlotState>& replies) {
    int bestIndex = -1;
    uint64_t highestPriority = 0;

    for (int i = 0; i < replies.size(); ++i) {
        if (replies[i].Fc.priority > highestPriority) {
            bestIndex = i;
            highestPriority = replies[i].Fc.priority;
        }
    }

    if (bestIndex != -1) {
        return Proposal(replies[bestIndex].Fc.priority, replies[bestIndex].Fc.proposerId, replies[bestIndex].Fc.value);
    } else {
        Log_info("This should not happen");
        return Proposal(-1,-1,-1);
    }
}


Proposal QuePaxaServer::findBestOfAggregateProposals(const vector<SlotState>& replies) {
    int bestIndex = -1;
    uint64_t highestPriority = 0;

    for (int i = 0; i < replies.size(); ++i) {
        if (replies[i].Ap.priority > highestPriority) {
            bestIndex = i; 
            highestPriority = replies[i].Ap.priority;
        }
    }

    if (bestIndex != -1) {
        return replies[bestIndex].Ap;
    } else {
        return Proposal(-1, -1, -1);
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

shared_ptr<Marshallable> QuePaxaServer::convertValueToCommand(uint64_t slot, uint64_t value, uint64_t proposerId) {
  auto state = make_shared<QuePaxaCommitMarshallable>();
  state->slot = slot;
  state->value = value;
  state->proposerId = proposerId;
  shared_ptr<Marshallable> st(state);
  return st;
}

void QuePaxaServer::commitChosenValue(uint64_t slot, Proposal proposal){
  committedValues[slot] = proposal.value;
  auto cmdptr_m = convertValueToCommand(slot, proposal.value, proposal.proposerId);
  #ifdef QUEPAXA_TEST_CORO
  app_next_(*cmdptr_m);
  #endif
  #ifdef QUEPAXA_SERVER_METRICS_COLLECTION
  if (proposal.proposerId == loc_id_){
    commit_times.push_back(start_times[proposal.value].elapsed());
    callbacks[proposal.value]();      
  }
  #endif
  for (int i = 0; i < 5; i++){
    if (i!=loc_id_){
      auto event = commo()->SendCommit(0, i, cmdptr_m);
      event->Wait(commit_timeout);
    }
  }
}

bool QuePaxaServer::checkAlreadyCommitted(uint64_t slot, Proposal proposal){
  if (committedValues.find(slot) != committedValues.end()){
    role = RECORDER;
    // should we commit on next free slot or ignore?
    #ifdef QUEPAXA_SERVER_METRICS_COLLECTION
    Log_debug("Slot %lu already committed with value %lu", slot, committedValues[slot]);
    reqs.push_back(std::make_tuple(curSlot, proposal.value, proposal.proposerId));
    curSlot++;
    #endif
    return true;
  }
  return false;
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
