#include "util/bench.h"
#include "util/random.h"

namespace boc_benchmark {

struct Dictler;

namespace dictler {

using namespace std;

struct Master;

struct Dictionary;

struct Worker {
  cown_ptr<Master> master;
  cown_ptr<Dictionary> dictionary;

  Worker(cown_ptr<Master> master, uint64_t index, cown_ptr<Dictionary> dictionary):
    master(move(master)), dictionary(move(dictionary)) {}

  static vector<string> splitBySpace(const string & input) {
        vector<string> tokens;
        stringstream ss(input);
        string token;

        while (ss >> token)
            tokens.push_back(token);

        return tokens;
    }

  static void work(const cown_ptr<Worker>& self);
};

//template <typename T>
struct Dictionary {
  unordered_map<uint64_t, uint64_t> map;
  Dictionary() {}
  static void write(const cown_ptr<Dictionary>& self, cown_ptr<Worker> worker, uint64_t key, uint64_t value);
  static void read(const cown_ptr<Dictionary>& self, cown_ptr<Worker> worker, uint64_t key);
};

struct Master {
  uint64_t workers;
  Master(uint64_t workers): workers(workers) {}

  static void make(uint64_t workers, uint64_t messages, uint64_t percentage) {
    when(make_cown<Master>(workers)) << [workers, messages, percentage](acquired_cown<Master> master) mutable {
      auto dictionary = make_cown<Dictionary>();

      for (uint64_t i = 0; i < workers; ++i) {
        Worker::work(make_cown<Worker>(master.cown(), i, dictionary));
      }
    };
  }

  static void done(const cown_ptr<Master>& self) {
    when(self) << [](acquired_cown<Master> self) mutable {
        /* done */
      
    };
  }
};

void Worker::work(const cown_ptr<Worker>& self) {
    when(self) << [tag=self](acquired_cown<Worker> self) mutable {
        string input;
        cout << "> ";
        getline(cin, input); // Read entire line including spaces

        vector<string> v = splitBySpace(input);
        
        if (v[0] == "w") {
            uint64_t key;
            istringstream iss(v[1]);
            iss >> key;
            uint64_t value;
            istringstream iss2(v[2]);
            iss2 >> value;
            Dictionary::write(self->dictionary, tag, key, value);
        }
        else if (v[0] == "r") {
            uint64_t key;
            istringstream iss(v[1]);
            iss >> key;
            Dictionary::read(self->dictionary, tag, key);
        }
        else {
            cout << "inwalid input." << endl;
        }
    };
}

void Dictionary::write(const cown_ptr<Dictionary>& self, cown_ptr<Worker> worker, uint64_t key, uint64_t value) {
  when(self) << [worker=move(worker), key, value](acquired_cown<Dictionary> self) mutable {
    self->map[key] = value;
    std::cout << "writing walue " << value << " to entry " << key << std::endl;
    Worker::work(worker);
  };
}

// Somehow read-only makes it slower?
void Dictionary::read(const cown_ptr<Dictionary>& self, cown_ptr<Worker> worker, uint64_t key) {
  when(verona::cpp::read(self)) << [=, worker=move(worker)](acquired_cown<const Dictionary> self) mutable {
    auto it = self->map.find(key);
    if (it == self->map.end()) 
        std::cout << "no walue found for key " << key << std::endl;
    else
        std::cout << "read walue " << it->second << " at entry " << key << std::endl;
    Worker::work(worker);
  };
}

};

struct Dictler: public BocBenchmark {
    void run() {
        dictler::Master::make(1, 100, 100);
    }
};

};