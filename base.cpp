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
  string label; // XXX
  //const string label;
  Location dest; // 0 <= l < model.trans.count() + 1

  ThreadTrans(const string label): label(label) {};
};

struct ThreadModel {
  unordered_map<Location, vector<ThreadTrans>> trans;
};

struct Thread {
  const string name;
  const ThreadModel &model;

  Thread(const string name, const ThreadModel &model): name(name), model(model) {
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

// Visualize

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

// Thread Instances (m_inc2)

int main() {
  // Read-Incr-Write (a.k.a Read-Modify-Write)

  // Read
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

  // Incr
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

  // Write
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
  unordered_map<Location, vector<ThreadTrans>> trans;
  trans[0] = vector<ThreadTrans>{read};
  trans[1] = vector<ThreadTrans>{incr};
  trans[2] = vector<ThreadTrans>{write};
  model.trans = trans;

  Thread p("p", model);
  printThreadTransition(p);
  return 0;
}
