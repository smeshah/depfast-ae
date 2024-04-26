#include "../marshallable.h"
#include "service.h"
#include "server.h"


namespace janus {

QuePaxaServiceImpl::QuePaxaServiceImpl(TxLogServer *sched) : svr_((QuePaxaServer*)sched) {
	struct timespec curr_time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
	srand(curr_time.tv_nsec);
}
  void QuePaxaServiceImpl::HandleHelloRpc(const string& req,
                                     string* res,
                                     rrr::DeferredReply* defer) {
  /* Your code here */
  Log_info("receive an rpc: %s", req.c_str());
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

} // namespace janus;
