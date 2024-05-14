#include "../marshallable.h"
#include "service.h"
#include "server.h"


namespace janus {

QuePaxaServiceImpl::QuePaxaServiceImpl(TxLogServer *sched) : svr_((QuePaxaServer*)sched) {
	struct timespec curr_time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
	srand(curr_time.tv_nsec);
}

void QuePaxaServiceImpl::HandleStart(const MarshallDeputy& md_cmd,
                                    rrr::DeferredReply* defer) {
  shared_ptr<Marshallable> cmd = const_cast<MarshallDeputy&>(md_cmd).sp_data_;
  Coroutine::CreateRun([this, defer, cmd]() mutable {
    uint64_t index;
    svr_->Start(cmd, &index, std::bind(&rrr::DeferredReply::reply, defer));
  });
}
  void QuePaxaServiceImpl::HandleHelloRpc(const string& req,
                                     string* res,
                                     rrr::DeferredReply* defer) {
  /* Your code here */
  Log_debug("receive an rpc: %s", req.c_str());
  *res = "world";
  defer->reply();
  }    
  void QuePaxaServiceImpl::HandleSendToRecoderRpc(const uint64_t& curSlot,
                                    const uint64_t& step,
                                     const string& proposalData,
                                     string* slotStateData,
                                     rrr::DeferredReply* defer) {
  svr_->intervalSummaryRegister(curSlot, step, proposalData, slotStateData);
  defer->reply();
  }
  void QuePaxaServiceImpl::HandleSendCommitRpc(const MarshallDeputy& md_cmd,
                                     rrr::DeferredReply* defer) {

  std::shared_ptr<Marshallable> value = const_cast<MarshallDeputy&>(md_cmd).sp_data_;
  svr_->handleCommit(value);
  defer->reply();
  }
void QuePaxaServiceImpl::HandleCollectMetrics(uint64_t* fast_path_count,
                                             vector<double>* commit_times,
                                             vector<double>* exec_times,
                                             rrr::DeferredReply* defer) {
  *fast_path_count = svr_->fast;
  *commit_times = svr_->commit_times;
  defer->reply();
}
} // namespace janus;
