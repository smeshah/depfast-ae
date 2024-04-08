#include "testconf.h"
#include "marshallable.h"

namespace janus {

#ifdef QUEPAXA_TEST_CORO

int _test_id_g = 0;

QuePaxaFrame **QuePaxaTestConfig::replicas = nullptr;
std::function<void(Marshallable &)> QuePaxaTestConfig::commit_callbacks[NSERVERS];
std::vector<int> QuePaxaTestConfig::committed_cmds[NSERVERS];
uint64_t QuePaxaTestConfig::rpc_count_last[NSERVERS];

QuePaxaTestConfig::QuePaxaTestConfig(QuePaxaFrame **replicas) {
  verify(QuePaxaTestConfig::replicas == nullptr);
  QuePaxaTestConfig::replicas = replicas;
  for (int i = 0; i < NSERVERS; i++) {
    QuePaxaTestConfig::replicas[i]->svr_->rep_frame_ = QuePaxaTestConfig::replicas[i]->svr_->frame_;
    QuePaxaTestConfig::committed_cmds[i].push_back(-1);
    QuePaxaTestConfig::rpc_count_last[i] = 0;
    disconnected_[i] = false;
  }
  th_ = std::thread([this](){ netctlLoop(); });
}

void QuePaxaTestConfig::SetLearnerAction(void) {
  for (int i = 0; i < NSERVERS; i++) {
    QuePaxaTestConfig::commit_callbacks[i] = [i](Marshallable& cmd) {
      verify(cmd.kind_ == MarshallDeputy::CMD_TPC_COMMIT);
      auto& command = dynamic_cast<TpcCommitCommand&>(cmd);
      Log_debug("server %d committed value %d", i, command.tx_id_);
      QuePaxaTestConfig::committed_cmds[i].push_back(command.tx_id_);
    };
    QuePaxaTestConfig::replicas[i]->svr_->RegLearnerAction(QuePaxaTestConfig::commit_callbacks[i]);
  }
}



int QuePaxaTestConfig::NCommitted(uint64_t index) {
  int cmd, n = 0;
  for (int i = 0; i < NSERVERS; i++) {
    if (QuePaxaTestConfig::committed_cmds[i].size() > index) {
      auto curcmd = QuePaxaTestConfig::committed_cmds[i][index];
      if (n == 0) {
        cmd = curcmd;
      } else {
        if (curcmd != cmd) {
          return -1;
        }
      }
      n++;
    }
  }
  return n;
}


void QuePaxaTestConfig::Disconnect(int svr) {
  verify(svr >= 0 && svr < NSERVERS);
  std::lock_guard<std::mutex> lk(disconnect_mtx_);
  verify(!disconnected_[svr]);
  disconnect(svr, true);
  disconnected_[svr] = true;
}

void QuePaxaTestConfig::Reconnect(int svr) {
  verify(svr >= 0 && svr < NSERVERS);
  std::lock_guard<std::mutex> lk(disconnect_mtx_);
  verify(disconnected_[svr]);
  reconnect(svr);
  disconnected_[svr] = false;
}

int QuePaxaTestConfig::NDisconnected(void) {
  int count = 0;
  for (int i = 0; i < NSERVERS; i++) {
    if (disconnected_[i])
      count++;
  }
  return count;
}

void QuePaxaTestConfig::SetUnreliable(bool unreliable) {
  std::unique_lock<std::mutex> lk(cv_m_);
  verify(!finished_);
  if (unreliable) {
    verify(!unreliable_);
    // lk acquired cv_m_ in state 1 or 0
    unreliable_ = true;
    // if cv_m_ was in state 1, must signal cv_ to wake up netctlLoop
    lk.unlock();
    cv_.notify_one();
  } else {
    verify(unreliable_);
    // lk acquired cv_m_ in state 2 or 0
    unreliable_ = false;
    // wait until netctlLoop moves cv_m_ from state 2 (or 0) to state 1,
    // restoring the network to reliable state in the process.
    lk.unlock();
    lk.lock();
  }
}

bool QuePaxaTestConfig::IsUnreliable(void) {
  return unreliable_;
}

void QuePaxaTestConfig::Shutdown(void) {
  // trigger netctlLoop shutdown
  {
    std::unique_lock<std::mutex> lk(cv_m_);
    verify(!finished_);
    // lk acquired cv_m_ in state 0, 1, or 2
    finished_ = true;
    // if cv_m_ was in state 1, must signal cv_ to wake up netctlLoop
    lk.unlock();
    cv_.notify_one();
  }
  // wait for netctlLoop thread to exit
  th_.join();
  // Reconnect() all Deconnect()ed servers
  for (int i = 0; i < NSERVERS; i++) {
    if (disconnected_[i]) {
      Reconnect(i);
    }
  }
}

uint64_t QuePaxaTestConfig::RpcCount(int svr, bool reset) {
  std::lock_guard<std::recursive_mutex> lk(
    QuePaxaTestConfig::replicas[svr]->commo_->rpc_mtx_);
  uint64_t count = QuePaxaTestConfig::replicas[svr]->commo_->rpc_count_;
  uint64_t count_last = QuePaxaTestConfig::rpc_count_last[svr];
  if (reset) {
    QuePaxaTestConfig::rpc_count_last[svr] = count;
  }
  verify(count >= count_last);
  return count - count_last;
}

uint64_t QuePaxaTestConfig::RpcTotal(void) {
  uint64_t total = 0;
  for (int i = 0; i < NSERVERS; i++) {
    total += QuePaxaTestConfig::replicas[i]->commo_->rpc_count_;
  }
  return total;
}


void QuePaxaTestConfig::netctlLoop(void) {
  int i;
  bool isdown;
  // cv_m_ unlocked state 0 (finished_ == false)
  std::unique_lock<std::mutex> lk(cv_m_);
  while (!finished_) {
    if (!unreliable_) {
      {
        std::lock_guard<std::mutex> prlk(disconnect_mtx_);
        // unset all unreliable-related disconnects and slows
        for (i = 0; i < NSERVERS; i++) {
          if (!disconnected_[i]) {
            reconnect(i, true);
            slow(i, 0);
          }
        }
      }
      // sleep until unreliable_ or finished_ is set
      // cv_m_ unlocked state 1 (unreliable_ == false && finished_ == false)
      cv_.wait(lk, [this](){ return unreliable_ || finished_; });
      continue;
    }
    {
      std::lock_guard<std::mutex> prlk(disconnect_mtx_);
      for (i = 0; i < NSERVERS; i++) {
        // skip server if it was disconnected using Disconnect()
        if (disconnected_[i]) {
          continue;
        }
        // server has DOWNRATE_N / DOWNRATE_D chance of being down
        if ((rand() % DOWNRATE_D) < DOWNRATE_N) {
          // disconnect server if not already disconnected in the previous period
          disconnect(i, true);
        } else {
          // Server not down: random slow timeout
          // Reconnect server if it was disconnected in the previous period
          reconnect(i, true);
          // server's slow timeout should be btwn 0-(MAXSLOW-1) ms
          slow(i, rand() % MAXSLOW);
        }
      }
    }
    // change unreliable state every 0.1s
    usleep(100000);
    // Coroutine::Sleep(100000);
    lk.unlock();
    // cv_m_ unlocked state 2 (unreliable_ == true && finished_ == false)
    lk.lock();
  }
  // If network is still unreliable, unset it
  if (unreliable_) {
    unreliable_ = false;
    {
      std::lock_guard<std::mutex> prlk(disconnect_mtx_);
      // unset all unreliable-related disconnects and slows
      for (i = 0; i < NSERVERS; i++) {
        if (!disconnected_[i]) {
          reconnect(i, true);
          slow(i, 0);
        }
      }
    }
  }
  // cv_m_ unlocked state 3 (unreliable_ == false && finished_ == true)
}

bool QuePaxaTestConfig::isDisconnected(int svr) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  return QuePaxaTestConfig::replicas[svr]->svr_->IsDisconnected();
}

void QuePaxaTestConfig::disconnect(int svr, bool ignore) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  if (!isDisconnected(svr)) {
    // simulate disconnected server
    QuePaxaTestConfig::replicas[svr]->svr_->Disconnect();
  } else if (!ignore) {
    verify(0);
  }
}

void QuePaxaTestConfig::reconnect(int svr, bool ignore) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  if (isDisconnected(svr)) {
    // simulate reconnected server
    QuePaxaTestConfig::replicas[svr]->svr_->Reconnect();
  } else if (!ignore) {
    verify(0);
  }
}

void QuePaxaTestConfig::slow(int svr, uint32_t msec) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  verify(!isDisconnected(svr));
  QuePaxaTestConfig::replicas[svr]->commo_->rpc_poll_->slow(msec * 1000);
}

QuePaxaServer *QuePaxaTestConfig::GetServer(int svr) {
  return QuePaxaTestConfig::replicas[svr]->svr_;
}

#endif

}
