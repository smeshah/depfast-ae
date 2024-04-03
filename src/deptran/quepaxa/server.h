#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../classic/tpc_command.h"
#include "commo.h"

namespace janus {

#define NOOP_DKEY string("")


class QuePaxaServer : public TxLogServer {
 private:

 public:

 private:

  /* Helpers */


  /* RPC handlers */

 public:


  /* Client request handlers */

  public:

  /* Do not modify this class below here */

 public:
  QuePaxaServer(Frame *frame) ;
  ~QuePaxaServer() ;
  
 private:
  bool disconnected_ = false;
  void Setup();

 public:
  void Disconnect(const bool disconnect = true);
  void Reconnect() {
    Disconnect(false);
  }
  bool IsDisconnected();

  virtual bool HandleConflicts(Tx& dtxn,
                               innid_t inn_id,
                               vector<string>& conflicts) {
    verify(0);
  };

  QuePaxaCommo* commo() {
    return (QuePaxaCommo*)commo_;
  }

};
} // namespace janus
