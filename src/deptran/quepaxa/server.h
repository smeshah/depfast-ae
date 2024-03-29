#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../classic/tpc_command.h"
#include "commo.h"
#include "graph.h"

namespace Janus {

    struct Proposal {
        int priority;
        int proposerId;
        int value;

        Proposal(int p = 0, int id = 0, int v = 0) : priority(p), proposerId(id), value(v) {}

        bool operator<(const Proposal& other) const {
            return priority == other.priority ? proposerId < other.proposerId : priority < other.priority;
        }
    };

    struct SlotState {
        int currentStep = 0;
        Proposal firstProposalInCurrentStep;
        Proposal maxProposalInCurrentStep;
        Proposal maxProposalInPreviousStep;
    };

    class QuePaxaServer {
    public:
        explicit QuePaxaServer(int serverId) : serverId_(serverId) {}

        // Methods for managing proposals and slots
        void propose(int step, const Proposal& proposal);
        SlotState processProposal(int step, const Proposal& proposal);

    private:
        int serverId_;
        std::unordered_map<int, SlotState> slotStates_;

    };

} 
