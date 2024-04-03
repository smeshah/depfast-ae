#include "../__dep__.h"
#include "../constants.h"
#include "frame.h"
#include "server.h"
#include "service.h"
#include "coordinator.h"

namespace janus {

REG_FRAME(MODE_QUEPAXA, vector<string>({"quepaxa"}), QuePaxaFrame);

#ifdef QUEPAXA_TEST_CORO
std::mutex QuePaxaFrame::quepaxa_test_mutex_;
uint16_t QuePaxaFrame::n_replicas_ = 0;
QuePaxaFrame *QuePaxaFrame::replicas_[NSERVERS];
#endif

QuePaxaFrame::QuePaxaFrame(int mode) : Frame(mode) {}

QuePaxaFrame::~QuePaxaFrame() {}

Coordinator *QuePaxaFrame::CreateCoordinator(cooid_t coo_id,
                                            Config *config,
                                            int benchmark,
                                            ClientControlServiceImpl *ccsi,
                                            uint32_t id,
                                            shared_ptr<TxnRegistry> txn_reg) {
  verify(config != nullptr);
  // TODO: pool used coordinator to avoid creating every time
  auto coord = new QuePaxaCoordinator(coo_id, benchmark, ccsi, id);
  setupCoordinator(coord, config);  
  Log_debug("create new coord, coo_id: %d", (int)coord->coo_id_);
  return coord;
}

TxLogServer *QuePaxaFrame::CreateScheduler() {
  if (svr_ == nullptr) {
    svr_ = new QuePaxaServer(this);
  } else {
    verify(0);
  }
  Log_debug("create quepaxa sched loc: %d", this->site_info_->locale_id);

  #ifdef QUEPAXA_TEST_CORO
  quepaxa_test_mutex_.lock();
  verify(n_replicas_ < NSERVERS);
  replicas_[this->site_info_->id] = this;
  n_replicas_++;
  quepaxa_test_mutex_.unlock();
  #endif
  return svr_;
}

Communicator *QuePaxaFrame::CreateCommo(PollMgr *poll) {
  if (commo_ == nullptr) {
    commo_ = new QuePaxaCommo(poll);
  }
  return commo_;
}

vector<rrr::Service *>
QuePaxaFrame::CreateRpcServices(uint32_t site_id,
                                TxLogServer *rep_sched,
                                rrr::PollMgr *poll_mgr,
                                ServerControlServiceImpl *scsi) {
  auto config = Config::GetConfig();
  auto result = vector<Service *>();
  switch (config->replica_proto_) {
    case MODE_QUEPAXA:
      result.push_back(new QuePaxaServiceImpl(rep_sched));
      break;
    default:
      break;
  }
  return result;
}

void QuePaxaFrame::setupCoordinator(QuePaxaCoordinator *coord, Config *config) {
  coord->frame_ = this;
  verify(commo_ != nullptr);
  coord->commo_ = commo_;
  verify(svr_ != nullptr);
  coord->sch_ = svr_;
  coord->n_replica_ = config->GetPartitionSize(site_info_->partition_id_);
  coord->loc_id_ = site_info_->locale_id;
  verify(coord->n_replica_ != 0);
}

} // namespace janus

