#include "../__dep__.h"
#include "../constants.h"
#include "frame.h"
#include "server.h"
#include "service.h"

namespace Janus {
    REG_FRAME(MODE_EPAXOS, vector<string>({"quepaxa"}), QuePaxaFrame);

    QuePaxaFrame :: QuePaxaFrame(int mode) : Frame(mode) {
    }
    QuePaxaFrame :: ~QuePaxaFrame() {
    }

    Coordinator *QuePaxaFrame :: CreateCoordinator(cooid_t coo_id,
                                                   Config *config,
                                                   int benchmark,
                                                   ClientControlServiceImpl *ccsi,
                                                   uint32_t id,
                                                   shared_ptr<TxnRegistry> txn_reg) {
        verify (config != nullptr);
        auto coord = new CoordinatorQuePaxa(coo_id, benchmark, ccsi, id);
        coord->frame_ = this;
        verify(commo_ != nullptr);
        coord->commo_ = commo_;
        verify(svr_ != nullptr);
        coord->sch_ = svr_;
        coord->n_replica_ = config->GetPartitionSize(site_info_->partition_id_);
        coord->loc_id_ = site_info_->locale_id;
        verify(coord->n_replica_ != 0);

        return coord;
    }
    TxLogServer *QuePaxaFrame :: CreateScheduler() {
        if (svr_ == nullptr) {
            svr_ = new EpaxosServer(this);
        } else {
            verify(0);
        }
        return svr_;
    }
    Communicator *QuePaxaFrame :: CreateCommo(PollMgr *poll) {
        if (commo_ == nullptr) {
            commo_ = new QuePaxaCommo(poll);
        } else {
            verify(0);
        }
        return commo_;
    }
    vector<rrr::Service *> QuePaxaFrame :: CreateRpcServices(uint32_t site_id,
                                                            TxLogServer *dtxn_sched,
                                                            rrr::PollMgr *poll_mgr,
                                                            ServerControlServiceImpl *scsi) {
        auto config = Config::GetConfig();
        auto result = vector<Service *>();
        switch (config->replica_proto_) {
        case MODE_QUEPAXA:
            result.push_back(new EpaxosServiceImpl(rep_sched));
            break;
        default:
            break;
        }
        return result;
    }
}