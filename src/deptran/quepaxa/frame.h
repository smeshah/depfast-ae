#pragma once

#include "../communicator.h"
#include "../frame.h"
#include "../constants.h"
#include "commo.h"
#include "server.h"

namespace janus {

class QuePaxaFrame : public Frame {
 private:
  #ifdef QUEPAXA_TEST_CORO
  static std::mutex quepaxa_test_mutex_;
  static std::shared_ptr<Coroutine> quepaxa_test_coro_;
  static uint16_t n_replicas_;
  static QuePaxaFrame *replicas_[5];
  static uint16_t n_commo_;
  static bool tests_done_;
  #endif

 public:
  QuePaxaCommo *commo_ = nullptr;
  QuePaxaServer *svr_ = nullptr;

  QuePaxaFrame(int mode);
  virtual ~QuePaxaFrame();

  TxLogServer *CreateScheduler() override;
  
  Communicator *CreateCommo(PollMgr *poll = nullptr) override;
  
  vector<rrr::Service *> CreateRpcServices(uint32_t site_id,
                                           TxLogServer *dtxn_sched,
                                           rrr::PollMgr *poll_mgr,
                                           ServerControlServiceImpl *scsi) override;
};

} // namespace janus
