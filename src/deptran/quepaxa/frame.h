#pragma once

#include "../communicator.h"
#include "../frame.h"
#include "../constants.h"
#include "commo.h"
#include "server.h"
#include "coordinator.h"

namespace Janus {
    class QuePaxaFrame : public Frame {
        QuePaxaCommo* commo_ = nullptr;
        QuePaxaServer* server_ = nullptr;

        QuePaxaFrame(int mode);

        virtual ~QuePaxaFrame() {
            delete commo_;
            delete server_;
        }

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
    };
}