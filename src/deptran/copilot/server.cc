#include "server.h"
#include "frame.h"
#include "coordinator.h"

#define DEBUG

namespace janus {

const char* CopilotServer::toString(uint8_t is_pilot) {
  if (is_pilot)
    return "PILOT";
  else
    return "COPILOT";
}

CopilotServer::CopilotServer(Frame* frame) : log_infos_(2) {
  frame_ = frame;
  id_ = frame->site_info_->id;
  setIsPilot(frame_->site_info_->locale_id == 0);
  setIsCopilot(frame_->site_info_->locale_id == 1);
}

shared_ptr<CopilotData> CopilotServer::GetInstance(slotid_t slot, uint8_t is_pilot) {
  auto& sp_instance = log_infos_[is_pilot].logs[slot];
  if (!sp_instance)
    sp_instance = std::make_shared<CopilotData>(
      CopilotData{nullptr,
                  0,
                  is_pilot, slot,
                  0,
                  Status::NOT_ACCEPTED,
                  0, 0});
  return sp_instance;
}

std::pair<slotid_t, uint64_t> CopilotServer::PickInitSlotAndDep() {
  
  uint64_t init_dep;
  slotid_t assigned_slot;
  /**
   * It also assigns the initial dependency for this entry,
   * which is the most recent entry from the other pilot it has seen.
   * 
   * initial slot id is 1, slot 0 is always empty
   */
  if (isPilot_) {
    init_dep = log_infos_[NO].max_committed_slot;
    assigned_slot = ++log_infos_[YES].current_slot;
  } else if (isCopilot_) {
    init_dep = log_infos_[YES].max_committed_slot;
    assigned_slot = ++log_infos_[NO].current_slot;
  } else {
    init_dep = 0;
    assigned_slot = 0;
  }

  Log_debug("server %d %s slot %lu assigned dep %lu", id_, toString(isPilot_),
            assigned_slot, init_dep);

  return { assigned_slot, init_dep };
}

void CopilotServer::Setup() {

  log_infos_[NO] = {};
  log_infos_[YES] = {};
}

void CopilotServer::OnForward(shared_ptr<Marshallable>& cmd,
                              const function<void()>& cb) {
  verify(isPilot_ || isCopilot_);
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_info("This Copilot server is: %d", id_);
  rep_frame_ = frame_;

  if (!isPilot_ && !isCopilot_) {
    verify(0);
    // TODO: forward to pilot server
  }
  auto coord = (CoordinatorCopilot *)CreateRepCoord();
  coord->Submit(cmd);

  cb();
}

void CopilotServer::OnPrepare(const uint8_t& is_pilot,
                              const uint64_t& slot,
                              const ballot_t& ballot,
                              shared_ptr<Marshallable> ret_cmd,
                              ballot_t* max_ballot,
                              uint64_t* dep,
                              status_t* status,
                              const function<void()>& cb) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto ins = GetInstance(slot, is_pilot);

  if (ins->ballot < ballot) {
    ins->ballot = ballot;
  }
  /**
   * The PrepareOk messages include the highest ballot number
   * for which a replica has fast or regular accepted an entry,
   * the command and dependency associated with that entry, and
   * an id of the dependency's proposing pilot.
   */
  *max_ballot = ins->ballot;
  ret_cmd = ins->cmd; // TODO: how to correctly return a marshallable?
  *dep = ins->dep_id;
  *status = ins->status;

  cb();
}

void CopilotServer::OnFastAccept(const uint8_t& is_pilot,
                                 const uint64_t& slot,
                                 const ballot_t& ballot,
                                 const uint64_t& dep,
                                 shared_ptr<Marshallable>& cmd,
                                 ballot_t* max_ballot,
                                 uint64_t* ret_dep,
                                 const function<void()> &cb) {
  // TODO: deal with ballot
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("copilot server %d fast accept for %s slot: %lu, dep %lu", id_,
            toString(is_pilot), slot, dep);

  auto ins = GetInstance(slot, is_pilot);
  auto& log_info = log_infos_[1 - is_pilot];
  auto& logs = log_info.logs;
  uint64_t suggest_dep = dep;

  /**
   * Thus, the check only needs to look at later entries in the other
   * pilot’s log. The compatibility check passes unless the replica
   * has already accepted a later entry P'.k (k > j) from the other
   * pilot P0 with a dependency earlier than P.i, i.e., P'.k’s dependency
   * is < P.i.
   */
  if (dep != 0) {
    for (slotid_t j = dep + 1; j <= log_info.max_accepted_slot; j++) {
      auto dep_id = logs[j]->dep_id;
      if (dep_id != 0 && dep_id < slot) {
        /**
         * Otherwise, it sends a FastAcceptReply message to the pilot
         * with its latest entry for the other pilot, P'.k,
         * as its suggested dependency.
         * TODO: definition on "latest"
         */
        suggest_dep = log_info.max_accepted_slot;
        Log_debug(
            "copilot server %d find imcompatiable dependence for %s slot: %lu, "
            "dep: %lu. suggest dep: %lu",
            id_, toString(is_pilot), slot, dep, suggest_dep);
        break;
      }
    }
  }

  if (ins->ballot <= ballot) {
    ins->ballot = ballot;
    ins->dep_id = dep;
    ins->cmd = cmd;
    ins->status = Status::FAST_ACCEPTED;
    updateMaxAcptSlot(log_info, slot); 
  } else {
    // TODO
  }
  *max_ballot = ins->ballot;
  *ret_dep = suggest_dep;

  cb();
}

void CopilotServer::OnAccept(const uint8_t& is_pilot,
                             const uint64_t& slot,
                             const ballot_t& ballot,
                             const uint64_t& dep,
                             shared_ptr<Marshallable>& cmd,
                             ballot_t* max_ballot,
                             const function<void()> &cb) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("copilot server %d fast accept for %s slot: %lu, dep %lu", id_, toString(is_pilot), slot, dep);

  auto ins = GetInstance(slot, is_pilot);
  auto& log_info = log_infos_[is_pilot];

  if (ins->ballot <= ballot) {
    ins->ballot = ballot;
    ins->dep_id = dep;
    ins->cmd = cmd;
    ins->status = Status::ACCEPTED;
    updateMaxAcptSlot(log_info, slot);
  } else {
    //TODO
  }

  *max_ballot = ins->ballot;
  cb();
}

void CopilotServer::OnCommit(const uint8_t& is_pilot,
                             const uint64_t& slot,
                             const uint64_t& dep,
                             shared_ptr<Marshallable>& cmd) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("Copilot server %d commit %s slot: %ld, dep %ld", id_, toString(is_pilot), slot, dep);
  auto ins = GetInstance(slot, is_pilot);
  ins->cmd = cmd;
  ins->status = Status::COMMITED;

  auto& log_info = log_infos_[is_pilot];
  updateMaxCmtdSlot(log_info, slot);
  verify(slot > log_info.max_executed_slot);

  /**
   * Q: should we execute commands here?
   * A: We may use another threads to execute the commands,
   * but for better programbility and understandability,
   * we should execute cmds here
   */
  if (executeCmd(ins)) {
    // log_info.max_executed_slot = slot;
    Log_debug("server %d %s slot id advance to %lu", id_, toString(is_pilot), ins->slot_id);
  }

  // TODO: should support snapshot for freeing memory.
  // for now just free anything 1000 slots before.
  int i = log_info.min_active_slot;
  while (i + 1000 < log_info.max_executed_slot) {
    log_info.logs.erase(i++);
  }
  log_info.min_active_slot = i;


}

void CopilotServer::setIsPilot(bool isPilot) {
  verify(!isPilot || !isCopilot_);
  isPilot_ = isPilot;
}

void CopilotServer::setIsCopilot(bool isCopilot) {
  verify(!isCopilot || !isPilot_);
  isCopilot_ = isCopilot;
}

inline void CopilotServer::updateMaxExecSlot(shared_ptr<CopilotData>& ins) {
  auto& log_info = log_infos_[ins->is_pilot];
  if (ins->slot_id == log_info.max_executed_slot + 1)
    log_info.max_executed_slot = ins->slot_id;
}

void CopilotServer::updateMaxAcptSlot(CopilotLogInfo& log_info, slotid_t slot) {
  slotid_t i;
  for (i = log_info.max_accepted_slot + 1; i <= slot; i++) {
    auto& log_entry = log_info.logs[i];
    if (log_entry && log_entry->status < Status::FAST_ACCEPTED)
      break;
  }
  log_info.max_accepted_slot = i - 1;
}

void CopilotServer::updateMaxCmtdSlot(CopilotLogInfo& log_info, slotid_t slot) {
  slotid_t i;
  for (i = log_info.max_committed_slot + 1; i <= slot; i++) {
    auto& log_entry = log_info.logs[i];
    if (log_entry && log_entry->status < Status::COMMITED)
      break;
  }
  log_info.max_committed_slot = i - 1;
}

bool CopilotServer::executeCmd(shared_ptr<CopilotData>& ins) {
  // if (!(ins->cmd))
  //   return true;  // no-op
  
  if (ins->dep_id == 0) {
    app_next_(*ins->cmd);
    updateMaxExecSlot(ins);
    ins->status = Status::EXECUTED;
    return true;
  } else {
#ifdef DEBUG
    app_next_(*ins->cmd);
    ins->status = Status::EXECUTED;
    return true;
#else
    return findSCC(ins);
#endif
  }
}

bool CopilotServer::findSCC(shared_ptr<CopilotData>& root) {
  int index = 1;
  while (!stack_.empty()) {
    stack_.pop();
  }
  
  return strongConnect(root, &index);
}

bool CopilotServer::strongConnect(shared_ptr<CopilotData>& ins, int* index) {
  ins->dfn = *index;
  ins->low = *index;
  *index = *index + 1;
  stack_.push(ins);

  std::vector<uint8_t> order = ins->is_pilot ? std::vector<uint8_t>{YES, NO}
                                             : std::vector<uint8_t>{NO, YES};

  for (auto& p : order) {
    // first handle own side, then handle another side
    auto end = (p == ins->is_pilot) ? ins->slot_id : ins->dep_id;
    for (auto i = log_infos_[p].max_executed_slot + 1; i <= end; i++) {
      auto w = GetInstance(i, ins->is_pilot);
      if (!w->cmd) {
        // TODO: (unlikely) this cmd has not been received, wait or return?
        // either synchronously wait or return, otherwise the stack_ will be
        // in inconsistent state
        return false;
      }

      if (w->status == Status::EXECUTED) continue;

      if (w->status != Status::COMMITED) {
        // TODO: this cmd has not been committed, wait or return?
        return false;
      }

      if (w->dfn == 0) {
        strongConnect(w, index);
        ins->low = std::min(ins->low, w->low);
      } else {
        ins->low = std::min(w->dfn, ins->low);
      }
    }
  }

  if (ins->low == ins->dfn) {
    std::vector<shared_ptr<CopilotData> > list;
    shared_ptr<CopilotData> w;
    
    do {
      w = stack_.top();
      stack_.pop();
      list.push_back(w);
    } while (w != ins);

    std::sort(list.begin(), list.end(),
              [](const shared_ptr<CopilotData>& i1,
                 const shared_ptr<CopilotData>& i2) -> bool {
                if (i1->is_pilot == i2->is_pilot)
                  return i1->slot_id < i2->slot_id;
                else
                  return i1->is_pilot;
              });
    
    for (auto& c : list) {
      if (c->cmd) // in case no-op
        app_next_(*(c->cmd));
      updateMaxExecSlot(c);
      c->status = Status::EXECUTED;
    }
  }

  return true;
}

} // namespace janus
