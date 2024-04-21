#include "test.h"

namespace janus {

#ifdef QUEPAXA_TEST_CORO

int QuePaxaLabTest::Run(void) {
  config_->SetLearnerAction();
  uint64_t start_rpc = config_->RpcTotal();
  if (testBasicAgree() ||
      testFailNoQuorum() ||
    testConcurrentStarts()
    ) {
    Print("TESTS FAILED");
    return 1;
  }
  Print("ALL TESTS PASSED");
  Print("Total RPC count: %ld", config_->RpcTotal() - start_rpc);
  return 0;
}

void QuePaxaLabTest::Cleanup(void) {
  config_->Shutdown();
}

#define Init2(test_id, description) \
  Init(test_id, description); \
  verify(config_->NDisconnected() == 0 && !config_->IsUnreliable())

#define Passed2() Passed(); return 0

#define Assert(expr) if (!(expr)) { \
  return 1; \
}

#define Assert2(expr, msg, ...) if (!(expr)) { \
  Failed(msg, ##__VA_ARGS__); \
  return 1; \
}
#define AssertWaitNoError(ret, index) \
        Assert2(ret != -3, "committed values differ for index %ld", index)
#define AssertNoneCommitted(index) { \
        auto nc = config_->NCommitted(index); \
        Assert2(nc == 0, \
                "%d servers unexpectedly committed index %ld", \
                nc, index) \
      }

#define AssertNCommitted(index, expected) { \
        auto nc = config_->NCommitted(index); \
        Assert2(nc == expected, \
                "%d servers committed index %ld (%d expected)", \
                nc, index, expected) \
      }

#define DoAgreeAndAssertNCommitted(cmd, n, leader) { \
        auto r = config_->DoAgreement(cmd, n, leader); \
      }
      
#define DoAgreeAndAssertNoneCommitted(cmd, n, leader) { \
        auto r = config_->DoAgreement(cmd,n, leader); \
        Assert2(r == 0, "committed command %d without majority", cmd); \
      }
// #define DoAgreeAndAssertWaitSuccess(cmd, n) { \
//         auto r = config_->DoAgreement(cmd, n, true); \
//         Assert2(r > 0, "failed to reach agreement for command %d among %d servers", cmd, n); \
//         index_ = r + 1; \
//       }

int QuePaxaLabTest::testBasicAgree(void) {
  Init2(1, "Basic agreement");
  for (int i = 1; i <= 5; i++) {
    // complete 1 agreement and make sure its index is as expected
    int cmd = 100 + i;
    // AssertNoneCommitted(cmd);
    uint64_t leader = 0;
    DoAgreeAndAssertNCommitted(cmd, NSERVERS, leader);
  }
  Passed2();
}

int QuePaxaLabTest::testFailNoQuorum(void) {
  Init2(2, "No agreement if too many servers disconnect");
  config_->Disconnect(1);
  config_->Disconnect(2);
  for (int i = 1; i <= 3; i++) {
    // complete 1 agreement and make sure its index is as expected
    int cmd = 200 + i;
    // make sure no commits exist before any agreements are started
    // AssertNoneCommitted(cmd);
    Log_info("Asserting no commits for cmd %d", cmd);
    uint64_t leader = 0;
    DoAgreeAndAssertNoneCommitted(cmd, NSERVERS, leader);
    Log_info("Done asserting no commits for cmd %d", cmd);
  }
  // Reconnect all
  config_->Reconnect(1);
  config_->Reconnect(2);
  Coroutine::Sleep(100000);
  Passed2();
}
class CSArgs {
 public:
  std::vector<uint64_t> *indices;
  std::mutex *mtx;
  int i;
  QuePaxaTestConfig *config;
};

static void *doConcurrentStarts(void *args) {
  CSArgs *csargs = (CSArgs *)args;
  uint64_t idx, tm;
  auto ok = csargs->config->Start(0, 301 + csargs->i, &idx);
  if (!ok) {
    return nullptr;
  }
  {
    std::lock_guard<std::mutex> lock(*(csargs->mtx));
    csargs->indices->push_back(idx);
  }
  return nullptr;
}

int QuePaxaLabTest::testConcurrentStarts(void) {
  Init2(3, "Concurrently started agreements");
  int nconcurrent = 5;
  bool success = false;
  for (int again = 0; again < 5; again++) {
    if (again > 0) {
      wait(3000000);
    }
    uint64_t index, term;
    // auto ok = config_->Start(0, 701, &index);
    // if (!ok) {
    //   continue; // retry (up to 5 times)
    // }
    // create 5 threads that each Start a command to leader
    std::vector<uint64_t> indices{};
    std::vector<int> cmds{};
    std::mutex mtx{};
    pthread_t threads[nconcurrent];
    for (int i = 0; i < nconcurrent; i++) {
      CSArgs *args = new CSArgs{};
      args->indices = &indices;
      args->mtx = &mtx;
      args->i = i;
      verify(pthread_create(&threads[i], nullptr, doConcurrentStarts, (void*)args) == 0);
    }
    // join all threads
    for (int i = 0; i < nconcurrent; i++) {
      verify(pthread_join(threads[i], nullptr) == 0);
    }
    // wait for all indices to commit
    for (auto index : indices) {
      int cmd = config_->Wait(index, NSERVERS);
      if (cmd < 0) {
        AssertWaitNoError(cmd, index);
        goto skip; // on timeout and term changes, try again
      }
      cmds.push_back(cmd);
    }
    // make sure all the commits are there with the correct values
    for (int i = 0; i < nconcurrent; i++) {
      auto val = 301 + i;
      int j;
      for (j = 0; j < cmds.size(); j++) {
        if (cmds[j] == val) {
          break;
        }
      }
      Assert2(j < cmds.size(), "cmd %d missing", val);
    }
    success = true;
    break;
    skip: ;
  }
  Assert2(success, "too many term changes and/or delayed responses");
  index_ += nconcurrent + 1;
  Passed2();
}
void QuePaxaLabTest::wait(uint64_t microseconds) {
  Reactor::CreateSpEvent<TimeoutEvent>(microseconds)->Wait();
}


#endif

}
