#include <iostream>
#include <vector>
#include <unordered_set>
#include <queue>
#include <string>

using namespace std;

// abb. transitions to trans

// Thread Transitions (Choices)

struct SharedVars {
  int x;
  int t0;
  int t1;

  bool operator==(const SharedVars &other) {
    return this->x == other.x
        && this->t0 == other.t0
        && this->t1 == other.t1
        ;
  };
};

template<> struct hash<SharedVars> {
  unsigned operator()(const SharedVars &v) {
    return v.x ^ (v.t0 << 3) ^ (v.t1 << 6); // unsigned is 32bits
  };
};

using Guard = bool(*)(SharedVars);
using Action = SharedVars(*)(SharedVars);
using Location = unsigned int;

struct ThreadTrans {
  Guard guard;
  Action action;
  Location dest; // 0 <= l < model.trans.count() + 1
};

struct ThreadModel {
  vector<ThreadTrans> trans;
};

struct Thread {
  const string name;
  ThreadModel model;

  Thread(const string name, ThreadModel model): name(name), model(model) {
  };
};

// State Transitions (Execution Paths)

using ThreadStateRef = unsigned int;

struct StateTransition {
  ThreadStateRef dest;
};

struct ThreadState {
  vector<Location> dest;
  SharedVars sharedVars;
  vector<StateTransition> trans; // adjs list

  bool operator==(const ThreadState &other) {
    return this->dest == other.dest
        && this->sharedVars == other.sharedVars
        ;
  };
};

template<> struct hash<ThreadState> {
  unsigned operator()(const ThreadState &v) {
    unsigned ret = hash<SharedVars>()(v.sharedVars);
    int i = 9;
    for (auto l: v.dest) {
      ret ^= (l >> i);
      i += 3; 
    }
    return ret;
  }
};

// Thread Instances (m_inc2)

int main() {
  // Read-Incr-Write (a.k.a Read-Modify-Write)

  // Read
  ThreadTrans read;
  read.dest = 1;
  read.guard = [](SharedVars s) {
    return true;
  };
  read.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  // Incr
  ThreadTrans incr;
  incr.dest = 2;
  incr.guard = [](SharedVars s) {
    return true;
  };
  incr.action = [](SharedVars s) {
    auto sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  // Write
  ThreadTrans write;
  incr.dest = 3;
  write.guard = [](SharedVars s) {
    return true;
  };
  write.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  ThreadModel model;
  model.trans = vector<ThreadTrans>{read, incr, write};

  Thread p("p", model), q("q", model);
  return 0;
}
