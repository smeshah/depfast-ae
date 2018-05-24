
#pragma once
#include "deptran/rococo/scheduler.h"

namespace janus {

class RccGraph;
class JanusCommo;
class SchedulerJanus : public SchedulerRococo {
 public:
  using SchedulerRococo::SchedulerRococo;

  map<txnid_t, shared_ptr<TxRococo>> Aggregate(RccGraph& graph);

  void OnPreAccept(const txnid_t txnid,
                   const vector<SimpleCommand> &cmds,
                   RccGraph* graph,
                   int32_t *res,
                   RccGraph *res_graph,
                   function<void()> callback);


  void OnAccept(const txnid_t txn_id,
                const ballot_t& ballot,
                const RccGraph& graph,
                int32_t* res,
                function<void()> callback);

//  void OnCommit(const txnid_t txn_id,
//                const RccGraph &graph,
//                int32_t *res,
//                TxnOutput *output,
//                const function<void()> &callback);

  void OnCommit(const txnid_t txn_id,
                RccGraph* graph,
                int32_t *res,
                TxnOutput *output,
                const function<void()> &callback);

//  void OnCommitWoGraph(const txnid_t cmd_id,
//                       int32_t* res,
//                       TxnOutput* output,
//                       const function<void()>& callback);

  int OnInquire(epoch_t epoch,
                cmdid_t cmd_id,
                RccGraph *graph,
                const function<void()> &callback) override;
  JanusCommo* commo();

  virtual bool HandleConflicts(Tx& dtxn,
                               innid_t inn_id,
                               vector<string>& conflicts) {
    verify(0);
  };
};
} // namespace rococo
