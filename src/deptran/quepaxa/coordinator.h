#pragma once

#include "../__dep__.h"
#include "../coordinator.h"
#include "../frame.h"

namespace janus {

class QuePaxaCommo;
class QuePaxaServer;
class QuePaxaCoordinator : public Coordinator {
 public:
  QuePaxaServer* sch_ = nullptr;
 private:

	QuePaxaCommo *commo() {
    verify(commo_ != nullptr);
    return (QuePaxaCommo *) commo_;
  }
  
 public:
  shared_ptr<Marshallable> cmd_{nullptr};
  QuePaxaCoordinator(uint32_t coo_id,
                    int32_t benchmark,
                    ClientControlServiceImpl *ccsi,
                    uint32_t thread_id);
  uint32_t n_replica_ = 0; 
  uint32_t n_replica() {
    verify(n_replica_ > 0);
    return n_replica_;
  }

  void Reset() override {}
  void DoTxAsync(TxRequest &req) override {}
  void Restart() override { verify(0); }
};

} //namespace janus