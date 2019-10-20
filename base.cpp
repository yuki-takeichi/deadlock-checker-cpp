#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <string>

using namespace std;

// abb. transitions to trans

// Thread Transitions (Choices)

struct SharedVars {
  int x;
  int t0;
  int t1;

  bool operator==(const SharedVars &other) const {
    return this->x == other.x
        && this->t0 == other.t0
        && this->t1 == other.t1
        ;
  };
};

template<> struct hash<SharedVars> {
  unsigned operator()(const SharedVars &v) const {
    return v.x | (v.t0 << 3) | (v.t1 << 6); // unsigned is 32bits
  };
};

using Guard = bool(*)(SharedVars);
using Action = SharedVars(*)(SharedVars);
using Location = unsigned int;

struct ThreadTrans {
  Guard guard;
  Action action;
  string label;
  Location dest; // 0 <= l < model.trans.count() + 1

  ThreadTrans(const string label): label(label) {};
};

struct ThreadModel {
  vector<vector<ThreadTrans>> trans;
};

struct Thread {
  const unsigned int id;
  const string name;
  const ThreadModel &model;

  Thread(const unsigned int id, const string name, const ThreadModel &model): id(id), name(name), model(model) {
  };
};

// State Transitions (Execution Paths)

using SystemStateId = unsigned int;

struct SystemStateRef {
  SystemStateId destId;
  string label;
  SystemStateRef(SystemStateId destId, const string label): destId(destId), label(label) {};
};

struct SystemState {
  SystemStateId id;

  // XXX separate these as a distinct struct
  vector<Location> locations;
  SharedVars sharedVars;

  vector<SystemStateRef> adjs; // adjs list

  bool operator==(const SystemState &other) const {
    return this->locations == other.locations
        && this->sharedVars == other.sharedVars
        ;
  };
};

template<> struct hash<SystemState> {
  unsigned operator()(const SystemState &v) const {
    unsigned ret = hash<SharedVars>()(v.sharedVars);
    int i = 9;
    for (auto l: v.locations) {
      ret |= (l >> i);
      i += 3; 
    }
    return ret;
  }
};

// Visualize execution

void printDotState(SystemState s) {
  cout
    << s.id
    << " [label=\""
    << s.id << "\\n"
    << "P" << s.locations[0]
    << " Q" << s.locations[1] << "\\n"
    << "x=" << s.sharedVars.x << ", "
    << "t0=" << s.sharedVars.t0 << ", "
    << "t1=" << s.sharedVars.t1
    << "\"]"
    << ";"
    << endl;
}

void printDotTransition(SystemStateId cur, SystemStateRef ref) {
  cout
    << cur
    << " -> "
    << ref.destId
    << " [label=\""
    << ref.label
    << "\"]"
    << ";"
    << endl;
}

void printDotComposision(unordered_map<SystemStateId, SystemState> composed) {
  cout << "digraph {" << endl;

  for (auto &it: composed) {
    SystemState s = it.second;
    printDotState(s);
    for (auto adj: s.adjs) {
      printDotTransition(s.id, adj);
    }
  }

  cout << "}" << endl;
}

// Composition

unordered_map<SystemStateId, SystemState> concurrentComposition(vector<Thread> ts, SystemState &init) {
  unordered_map<SystemStateId, SystemState> res;
  unordered_set<SystemState> visited;

  queue<SystemState> q;
  q.push(init);
  res[init.id] = init;

  SystemStateId id = init.id;
  while (q.size() > 0) {
    SystemState s = q.front();
    q.pop();

    for (Thread &t: ts) {
      Location l = s.locations[t.id];
      if (l >= t.model.trans.size()) {
        continue;
      }
      for (ThreadTrans trans: t.model.trans[l]) {
        if (trans.guard(s.sharedVars)) {
          SystemState newState = s;
          newState.sharedVars = trans.action(s.sharedVars);
          newState.locations[t.id] = trans.dest;

          if (visited.count(newState) > 0) {
            res[s.id].adjs.push_back(SystemStateRef(visited.find(newState)->id, trans.label));
            continue;
          }

          newState.id = ++id;
          res[newState.id] = newState;
          res[s.id].adjs.push_back(SystemStateRef(newState.id, trans.label));

          q.push(newState);
          visited.insert(newState);
        }
      }
    }
  }

  return res;
}

// Thread Instances (m_inc2)

int main() {
  // P: Read-Incr-Write

  // Atomic read
  ThreadTrans read("readP");
  read.dest = 1;
  read.guard = [](SharedVars s) {
    return true;
  };
  read.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  // Atomic increment
  ThreadTrans incr("incrP");
  incr.dest = 2;
  incr.guard = [](SharedVars s) {
    return true;
  };
  incr.action = [](SharedVars s) {
    auto sPrime = s;
    sPrime.t0 = s.t0 + 1;
    return sPrime;
  };

  // Atomic write
  ThreadTrans write("writeP");
  write.dest = 3;
  write.guard = [](SharedVars s) {
    return true;
  };
  write.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.x = s.t0;
    return sPrime;
  };

  ThreadModel model;
  vector<vector<ThreadTrans>>trans {
    vector<ThreadTrans>{read},
    vector<ThreadTrans>{incr},
    vector<ThreadTrans>{write}
  };
  model.trans = trans;


  // Q: Read-Incr-Write

  // Atomic read
  ThreadTrans readQ("readQ");
  readQ.dest = 1;
  readQ.guard = [](SharedVars s) {
    return true;
  };
  readQ.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.t1 = s.x;
    return sPrime;
  };

  // Atomic increment
  ThreadTrans incrQ("incrQ");
  incrQ.dest = 2;
  incrQ.guard = [](SharedVars s) {
    return true;
  };
  incrQ.action = [](SharedVars s) {
    auto sPrime = s;
    sPrime.t1 = s.t1 + 1;
    return sPrime;
  };

  // Atomic write
  ThreadTrans writeQ("writeQ");
  writeQ.dest = 3;
  writeQ.guard = [](SharedVars s) {
    return true;
  };
  writeQ.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.x = s.t1;
    return sPrime;
  };

  ThreadModel modelQ;
  vector<vector<ThreadTrans>>transQ {
    vector<ThreadTrans>{readQ},
    vector<ThreadTrans>{incrQ},
    vector<ThreadTrans>{writeQ}
  };
  modelQ.trans = transQ;
  Thread p(0, "p", model), q(1, "q", modelQ);

  SystemState init;
  init.sharedVars.x = 0;
  init.sharedVars.t0 = 0;
  init.sharedVars.t1 = 0;
  init.locations = vector<Location>{0, 0};
  auto res = concurrentComposition(vector<Thread>{p, q}, init);
  printDotComposision(res);
  return 0;
}
