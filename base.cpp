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
  string label; // XXX
  //const string label;
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

struct SystemState {
  SystemStateId id;

  vector<Location> locations;
  SharedVars sharedVars;
  vector<SystemStateId> adjs; // adjs list

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

// Visualize thread impl

void printDotTheadState(Location l) {
  cout << l << ";" << endl;
}

void printDotTheadTrans(Location l, ThreadTrans choice) {
  cout << l << " -> " << choice.dest << endl;
}

void printThreadTransition(Thread p) {
  cout << "digraph {" << endl;

  const ThreadModel &m = p.model;
  unordered_set<Location> visited;
  queue<Location> q;
  q.push(0 /* initial location */);
  while (q.size() > 0) {
    Location l = q.front();
    q.pop();
    if (visited.count(l) > 0) {
      continue;
    }
    visited.insert(l);
    printDotTheadState(l);

    // XXX https://stackoverflow.com/a/41130170
    vector<ThreadTrans> trans = m.trans.at(l);
    for (ThreadTrans &choice: trans) {
      if (choice.dest < m.trans.size()) {
        q.push(choice.dest);
        printDotTheadTrans(l, choice);
      }
    }
  }

  cout << "}" << endl;
}


// Visualize execution

void printDotState(SystemState s) {
  cout << s.id << ";" << endl;
}

void printDotTransition(SystemStateId cur, SystemStateId dest) {
  cout << cur << " -> " << dest << ";" << endl;
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
  unordered_map<SystemStateId, SystemState> visited;
  queue<SystemState> q;
  q.push(init);

  SystemStateId id = init.id;
  while (q.size() > 0) {
    SystemState &s = q.front();
    q.pop();
    if (visited.count(s.id) > 0) {
      continue;
    }
    visited[s.id] = s;

    for (Thread &t: ts) {
      Location l = init.locations[t.id];
      for (ThreadTrans trans: t.model.trans[l]) {
        if (trans.guard(s.sharedVars)) {
          SystemState newState = s;
          newState.id = ++id;
          newState.sharedVars = trans.action(s.sharedVars);
          newState.locations[t.id] = trans.dest;

          if (trans.dest < t.model.trans.size()) {
            q.push(newState);
            s.adjs.push_back(newState.id);
          }
        }
      }
    }
  }

  return visited;
}

// Thread Instances (m_inc2)

int main() {
  // Read-Incr-Write

  // Atomic read
  ThreadTrans read("read");
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
  ThreadTrans incr("incr");
  incr.dest = 2;
  incr.guard = [](SharedVars s) {
    return true;
  };
  incr.action = [](SharedVars s) {
    auto sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  // Atomic write
  ThreadTrans write("write");
  write.dest = 3;
  write.guard = [](SharedVars s) {
    return true;
  };
  write.action = [](SharedVars s) {
    SharedVars sPrime = s;
    sPrime.t0 = s.x;
    return sPrime;
  };

  ThreadModel model;
  vector<vector<ThreadTrans>>trans {
    vector<ThreadTrans>{read},
    vector<ThreadTrans>{incr},
    vector<ThreadTrans>{write}
  };
  model.trans = trans;

  Thread p(0, "p", model), q(1, "q", model);
  printThreadTransition(p);

  SystemState init;
  init.sharedVars.x = 0;
  init.sharedVars.t0 = 0;
  init.sharedVars.t1 = 0;
  init.locations = vector<Location>{0, 0};
  auto res = concurrentComposition(vector<Thread>{p, q}, init);
  printDotComposision(res);
  return 0;
}
