#include <iostream>
#include <vector>
#include <unordered_set>
#include <queue>
#include <string>

using namespace std;

// abb. transitions to trans

// Thread Transitions (Choices)

struct ThreadState;
using Guard = bool(*)(ThreadState);
using Action = ThreadState(*)(ThreadState);

struct ThreadTrans {
  Guard guard;
  Action action;
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
  vector<StateTransition> trans;
  //
  int x;
  int t0;
  int t1;
};

// Thread Instances (m_inc2)

int main() {
  // Read-Incr-Write (a.k.a Read-Modify-Write)

  // Read
  ThreadTrans read;
  read.guard = [](ThreadState s) {
    return true;
  };
  read.action = [](ThreadState s) {
    ThreadState sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  // Incr
  ThreadTrans incr;
  incr.guard = [](ThreadState s) {
    return true;
  };
  incr.action = [](ThreadState s) {
    ThreadState sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  // Write
  ThreadTrans write;
  write.guard = [](ThreadState s) {
    return true;
  };
  write.action = [](ThreadState s) {
    ThreadState sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  ThreadModel model;
  model.trans = vector<ThreadTrans>{read, incr, write};

  Thread p("p", model), q("q", model);

  return 0;
}


