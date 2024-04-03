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
} // namespace janus;
