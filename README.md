
## Janus 
[![Build Status](https://travis-ci.org/NYU-NEWS/janus.svg?branch=master)](https://travis-ci.org/NYU-NEWS/janus)

Code repo for our OSDI '16 paper:
[Consolidating Concurrency Control and Consensus for Commits under Conflicts](http://mpaxos.com/pub/janus-osdi16.pdf)


## Quick start (with Ubuntu 16.04 or newer)

Install dependencies:

```
sudo apt-get update
sudo apt-get install -y \
    git \
    pkg-config \
    build-essential \
    clang \
    libapr1-dev libaprutil1-dev \
    libboost-all-dev \
    libyaml-cpp-dev \
    python3-dev \
    python3-pip \
    libgoogle-perftools-dev
sudo pip install -r requirements.txt
```

Get source code:
```
git clone --recursive https://github.com/NYU-NEWS/janus.git
```

Build:

```
python3 waf configure build -t

```
Test run:
```
python3 test_run.py -m janus
```

## More
Check out the doc directory to find more about how to build the system on older or newer distros, how to run the system in a distributed setup, and how to generate figures in the paper, etc.
<!-- 
## Do some actual good
For every star collected on this project, I will make a $25 charity loan via [Kiva] (https://www.kiva.org/invitedby/gzcdm3147?utm_campaign=permurl-share-invite-normal&utm_medium=referral&utm_content=gzcdm3147&utm_source=mpaxos.com).
-->

### Run paxos only

One-site paxos
```
./build/deptran_server -b -d 60 -f 'config/1c1s1p.yml' -f 'config/occ_paxos.yml' -f 'config/tpcc.yml' -f 'config/concurrent_1.yml' -P 'localhost' -p 5555 -t 10 -r '/home/vm1/janus/log' 1>'/home/vm1/janus/log/proc-localhost.log' 2>'/home/vm1/janus/log/proc-localhost.err'
```

Multi-site paxos (buggy when program runs to the end)
```
./build/deptran_server -b -d 60 -f 'config/1c1s3r1p.yml' -f 'config/occ_paxos.yml' -f 'config/tpcc.yml' -f 'config/concurrent_1.yml' -P 'localhost' -p 5555 -t 10 -r '/home/vm1/janus/log' 1>'/home/vm1/janus/log/proc-localhost.log' 2>'/home/vm1/janus/log/proc-localhost.err'
```