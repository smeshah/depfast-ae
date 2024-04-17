#include "test.h"

namespace janus {

#ifdef QUEPAXA_TEST_CORO

int QuePaxaLabTest::Run(void) {
  config_->SetLearnerAction();
  uint64_t start_rpc = config_->RpcTotal();
  if (testBasicAgree()
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

#define DoAgreeAndAssertNCommitted(cmd, n) { \
        auto r = config_->DoAgreement(cmd, n); \
      }
      
#define DoAgreeAndAssertNoneCommitted(cmd, n) { \
        auto r = config_->DoAgreement(cmd, n); \
      }

// #define DoAgreeAndAssertWaitSuccess(cmd, n) { \
//         auto r = config_->DoAgreement(cmd, n, true); \
//         Assert2(r > 0, "failed to reach agreement for command %d among %d servers", cmd, n); \
//         index_ = r + 1; \
//       }

int QuePaxaLabTest::testBasicAgree(void) {
  Init2(1, "Basic agreement");
  for (int i = 1; i <= 2; i++) {
    // complete 1 agreement and make sure its index is as expected
    int cmd = 100 + i;
    string dkey = to_string(cmd);
    DoAgreeAndAssertNCommitted(cmd, NSERVERS);
  }
  Passed2();
}


#endif

}
