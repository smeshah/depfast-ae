
#include "tx.h"

namespace janus {

void TxTroad::DispatchExecute(SimpleCommand &cmd,
                              map<int32_t, Value> *output) {
  phase_ = PHASE_RCC_DISPATCH;
  for (auto& c: dreqs_) {
    if (c.inn_id() == cmd.inn_id()) // already handled?
      return;
  }
  TxnPieceDef& piece = txn_reg_.lock()->get(cmd.root_type_, cmd.type_);
  auto& conflicts = piece.conflicts_;
  for (auto& c: conflicts) {
    vector<Value> pkeys;
    for (int i = 0; i < c.primary_keys.size(); i++) {
      pkeys.push_back(cmd.input.at(c.primary_keys[i]));
    }
    auto row = Query(GetTable(c.table), pkeys, c.row_context_id);
    verify(row != nullptr);
    for (auto col_id : c.columns) {
      TraceDep(row, col_id, cmd.rank_);
    }
  }
  dreqs_.push_back(cmd);

  // TODO are these preemptive actions proper?
  int ret_code;
  cmd.input.Aggregate(ws_);
  piece.proc_handler_(nullptr,
                          *this,
                          cmd,
                          &ret_code,
                          *output);
  ws_.insert(*output);
}

void TxTroad::CommitExecute() {
  phase_ = PHASE_RCC_COMMIT;
  committed_ = true;
  if (global_validation_result_ < 0) {
    return;
  }
  TxWorkspace ws;
  for (auto &cmd: dreqs_) {
    verify (cmd.rank_ == RANK_I || cmd.rank_ == RANK_D);
    if (! __mocking_janus_) {
      if (cmd.rank_ != current_rank_) {
        continue;
      }
    }
    TxnPieceDef& p = txn_reg_.lock()->get(cmd.root_type_, cmd.type_);
    int tmp;
    cmd.input.Aggregate(ws);
    auto& m = output_[cmd.inn_id_];
    p.proc_handler_(nullptr, *this, cmd, &tmp, m);
    ws.insert(m);
  }
}

} // namespace janus
