#include "../__dep__.h"
#include "../constants.h"
#include "coordinator.h"
#include "commo.h"

#include "server.h"

namespace janus {

QuePaxaCoordinator::QuePaxaCoordinator(uint32_t coo_id,
                                     int32_t benchmark,
                                     ClientControlServiceImpl* ccsi,
                                     uint32_t thread_id)
    : Coordinator(coo_id, benchmark, ccsi, thread_id) {
}

void QuePaxaCoordinator::Submit(shared_ptr<Marshallable>& cmd,
                               const function<void()>& commit_callback,
                               const function<void()>& exe_callback) {
	std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(!in_submission_);
  verify(cmd_ == nullptr);
  in_submission_ = true;
  cmd_ = cmd;
  verify(cmd_->kind_ != MarshallDeputy::UNKNOWN);
  commit_callback_ = commit_callback;
}

} // namespace janus