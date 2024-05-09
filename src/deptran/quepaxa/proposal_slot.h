#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


enum ROLE {
    PROPOSER,
    RECORDER
};


class Proposal {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & priority;
        ar & proposerId;
        ar & value;
    }

public:
    Proposal() : priority(0), proposerId(0), value(0) {}
    Proposal(uint64_t p, uint64_t pid, uint64_t v) : priority(p), proposerId(pid), value(v) {}

    bool operator==(const Proposal& other) const {
        return priority == other.priority &&
               proposerId == other.proposerId &&
               value == other.value;
    }

public:
    uint64_t priority;
    uint64_t proposerId;
    uint64_t value;
};

class SlotState {
private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & currentStep;
        ar & Fc;
        ar & Ac;
        ar & Ap;
    }

public:
    SlotState() : currentStep(0), Fc(0, 0, 0), Ac(0, 0, 0), Ap(0, 0, 0) {}

public:
    uint64_t currentStep; // current step
    Proposal Fc; // First proposal seen in current step
    Proposal Ac; // Maximum priority proposal seen till now
    Proposal Ap; // Maximum priority proposal seen in previous step
};
