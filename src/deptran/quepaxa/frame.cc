#include "../__dep__.h"
#include "../constants.h"
#include "frame.h"
#include "server.h"
#include "service.h"
#include "test.h"

namespace janus {

REG_FRAME(MODE_QUEPAXA, vector<string>({"quepaxa"}), QuePaxaFrame);

#ifdef QUEPAXA_TEST_CORO
std::mutex QuePaxaFrame::quepaxa_test_mutex_;
std::shared_ptr<Coroutine> QuePaxaFrame::quepaxa_test_coro_ = nullptr;
uint16_t QuePaxaFrame::n_replicas_ = 0;
QuePaxaFrame *QuePaxaFrame::replicas_[5];
uint16_t QuePaxaFrame::n_commo_ = 0;
bool QuePaxaFrame::tests_done_ = false;
#endif

QuePaxaFrame::QuePaxaFrame(int mode) : Frame(mode) {}

QuePaxaFrame::~QuePaxaFrame() {}

TxLogServer *QuePaxaFrame::CreateScheduler() {
  if (svr_ == nullptr) {
    svr_ = new QuePaxaServer(this);
  } else {
    verify(0);
  }
  Log_debug("create quepaxa sched loc: %d", this->site_info_->locale_id);

  #ifdef QUEPAXA_TEST_CORO
  quepaxa_test_mutex_.lock();
  verify(n_replicas_ < 5);
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

  #ifdef QUEPAXA_TEST_CORO
  quepaxa_test_mutex_.lock();
  verify(n_replicas_ == 5);
  n_commo_++;
  quepaxa_test_mutex_.unlock();
  if (site_info_->locale_id == 0) {
    verify(quepaxa_test_coro_.get() == nullptr);
    Log_debug("Creating QuePaxa test coroutine");
    quepaxa_test_coro_ = Coroutine::CreateRun([this] () {
      // Yield until all 5 communicators are initialized
      Coroutine::CurrentCoroutine()->Yield();
      // Run tests
      verify(n_replicas_ == 5);
      auto testconfig = new QuePaxaTestConfig(replicas_);
      QuePaxaLabTest test(testconfig);
      test.Run();
      test.Cleanup();
      // Turn off Reactor loop
      Reactor::GetReactor()->looping_ = false;
      return;
    });
    Log_info("quepaxa_test_coro_ id=%d", quepaxa_test_coro_->id);
    // wait until n_commo_ == 5, then resume the coroutine
    quepaxa_test_mutex_.lock();
    while (n_commo_ < 5) {
      quepaxa_test_mutex_.unlock();
      sleep(0.1);
      quepaxa_test_mutex_.lock();
    }
    quepaxa_test_mutex_.unlock();
    Reactor::GetReactor()->ContinueCoro(quepaxa_test_coro_);
  }
  #endif

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

} // namespace janus

