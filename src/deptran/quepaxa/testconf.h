#pragma once

#include "frame.h"
#include "coordinator.h"

namespace janus {

#ifdef QUEPAXA_TEST_CORO

// 5 servers in test configuration
#define NSERVERS 5
// slow network connections have latency up to 26 milliseconds
#define MAXSLOW 27
// servers have 1/10 chance of being disconnected to the network
#define DOWNRATE_N 1
#define DOWNRATE_D 10
// Give a generous 5 seconds for elections
#define ELECTIONTIMEOUT 5000000

// elections are expected to take at most 3 RPCs per server
#define ELECTIONRPCS (3 * NSERVERS)
// expected # of RPCs taken to commit n agreements
#define COMMITRPCS(n) ((n + 1) * NSERVERS)


#define Print(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)

extern int _test_id_g;
#define Init(test_id, description) \
  Print("TEST %d: " description, test_id); \
  _test_id_g = test_id
#define Failed(msg, ...) Print("TEST %d Failed: " msg, _test_id_g, ##__VA_ARGS__)
#define Passed() Print("TEST %d Passed", _test_id_g)

class CommitIndex {
 private:
  uint64_t val_;
 public:
  CommitIndex(uint64_t val) : val_(val) {}
  uint64_t getval(void) { return val_; }
  void setval(uint64_t val) { val_ = val; }
};

class QuePaxaTestConfig {

 private:
  static QuePaxaFrame **replicas;
  static std::function<void(Marshallable &)> commit_callbacks[NSERVERS];
  static std::vector<int> committed_cmds[NSERVERS];
  static uint64_t rpc_count_last[NSERVERS];

  // disconnected_[svr] true if svr is disconnected by Disconnect()/Reconnect()
  bool disconnected_[NSERVERS];
  // guards disconnected_ between Disconnect()/Reconnect() and netctlLoop
  std::mutex disconnect_mtx_;

 public:
  QuePaxaTestConfig(QuePaxaFrame **replicas);

  // sets up learner action functions for the servers
  // so that each committed command on each server is
  // logged to this test's data structures.
  void SetLearnerAction(void);

 
  // Returns number of servers that think log entry at index is committed.
  // Checks if the committed value for index is the same across servers.
  int NCommitted(uint64_t index);

  // Disconnects server from rest of servers
  void Disconnect(int svr);

  // Reconnects disconnected server
  void Reconnect(int svr);

  // Returns number of disconnected servers
  int NDisconnected(void);

  // Sets/unsets network unreliable
  // Blocks until network successfully set to unreliable/reliable
  // If unreliable == true, previous call to SetUnreliable must have been false
  // and vice versa
  void SetUnreliable(bool unreliable = true);

  bool IsUnreliable(void);

  // Reconnects all disconnected servers
  // Waits on unreliable thread
  void Shutdown(void);

  // Resets RPC counts to zero
  void RpcCountReset(void);

  // Returns total RPC count for a server
  // if reset, the next time RpcCount called for
  // svr, the count will exclude all RPCs before this call
  uint64_t RpcCount(int svr, bool reset = true);

  // Returns total RPC count across all servers
  // since server setup.
  uint64_t RpcTotal(void);

 private:
  // vars & subroutine for unreliable network setting
  std::thread th_;
  std::mutex cv_m_; // guards cv_, unreliable_, finished_
  std::condition_variable cv_;
  bool unreliable_ = false;
  bool finished_ = false;
  void netctlLoop(void);

  // internal disconnect/reconnect/slow functions
  std::recursive_mutex connection_m_;
  bool isDisconnected(int svr);
  void disconnect(int svr, bool ignore = false);
  void reconnect(int svr, bool ignore = false);
  void slow(int svr, uint32_t msec);


 public:
  QuePaxaServer *GetServer(int svr);

};

#endif

}
