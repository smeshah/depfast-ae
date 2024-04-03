#pragma once

#include "../communicator.h"
#include "../frame.h"
#include "../constants.h"
#include "commo.h"
#include "server.h"
#include "coordinator.h"

namespace janus {

class QuePaxaFrame : public Frame {
 private:
  #ifdef QUEPAXA_TEST_CORO
  static std::mutex quepaxa_test_mutex_;
  static uint16_t n_replicas_;
  #endif

 public:
  #ifdef QUEPAXA_TEST_CORO
  static QuePaxaFrame *replicas_[NSERVERS];
  #endif
  QuePaxaCommo *commo_ = nullptr;
  QuePaxaServer *svr_ = nullptr;

  QuePaxaFrame(int mode);
  virtual ~QuePaxaFrame();

  Coordinator *CreateCoordinator(cooid_t coo_id,
                                 Config *config,
                                 int benchmark,
                                 ClientControlServiceImpl *ccsi,
                                 uint32_t id,
                                 shared_ptr<TxnRegistry> txn_reg);

  TxLogServer *CreateScheduler() override;
  
  Communicator *CreateCommo(PollMgr *poll = nullptr) override;
  
  vector<rrr::Service *> CreateRpcServices(uint32_t site_id,
                                           TxLogServer *dtxn_sched,
                                           rrr::PollMgr *poll_mgr,
                                           ServerControlServiceImpl *scsi) override;

  void setupCoordinator(QuePaxaCoordinator *coord, Config *config);
};

} // namespace janus
