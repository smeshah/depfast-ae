#pragma once

#include "testconf.h"

namespace janus {

#ifdef QUEPAXA_TEST_CORO

class QuePaxaLabTest {

 private:
  QuePaxaTestConfig *config_;
  uint64_t index_;
  uint64_t init_rpcs_;

 public:
  QuePaxaLabTest(QuePaxaTestConfig *config) : config_(config), index_(1) {}
  int Run(void);
  void Cleanup(void);

 private:
  void wait(uint64_t microseconds);

};

#endif

} // namespace janus
