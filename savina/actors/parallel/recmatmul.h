#include <cpp/when.h>
#include <util/bench.h>
#include <util/random.h>
#include <cmath>
#include <unordered_map>
#include <tuple>

namespace actor_benchmark {

namespace recmatmul {

using namespace std;

struct Worker;
struct Collector;

struct Master {
  vector<cown_ptr<Worker>> workers;
  uint64_t length;
  uint64_t num_blocks;

  vector<vector<uint64_t>> matrix_a;
  vector<vector<uint64_t>> matrix_b;
  cown_ptr<Collector> collector;

  uint64_t sent;
  uint64_t num_workers;

  Master() {}

  static cown_ptr<Master> make(uint64_t workers, uint64_t data_length, uint64_t threshold);
  void send_work(uint64_t priority, uint64_t srA, uint64_t scA, uint64_t srB, uint64_t scB, uint64_t srC, uint64_t scC, uint64_t length, uint64_t dimension);
  static void work(cown_ptr<Master> self, uint64_t priority, uint64_t srA, uint64_t scA, uint64_t srB, uint64_t scB, uint64_t srC, uint64_t scC, uint64_t length, uint64_t dimension);
  static void done(cown_ptr<Master>);
};

struct Collector {
  cown_ptr<Master> master;
  uint64_t length;
  vector<vector<uint64_t>> result;

  Collector(cown_ptr<Master> master, uint64_t length): master(master), length(length) {
    for (uint64_t i = 0; i < length; ++i)
      result.push_back(vector<uint64_t>(length, 0));
  }

  static void collect(cown_ptr<Collector> self, vector<tuple<uint64_t, uint64_t, uint64_t>> partial_result) {
    when(self) << [partial_result=move(partial_result)](acquired_cown<Collector> self) {
      for (uint64_t n = 0; n < partial_result.size(); ++n) {
        auto coord = partial_result[n];
        auto i = get<0>(coord);
        auto j = get<1>(coord);
        auto r = get<2>(coord);

        self->result[i][j] = r;
      }

      Master::done(self->master);
    };
  }

};

struct Worker {
  cown_ptr<Master> master;
  cown_ptr<Collector> collector;
  vector<vector<uint64_t>> matrix_a;
  vector<vector<uint64_t>> matrix_b;
  uint64_t threshold;
  bool did_work;

  Worker(cown_ptr<Master> master, cown_ptr<Collector> collector, vector<vector<uint64_t>> matrix_a, vector<vector<uint64_t>> matrix_b, uint64_t threshold)
    :master(master), collector(collector), matrix_a(matrix_a), matrix_b(matrix_b), threshold(threshold), did_work(false) {}

  static void work(cown_ptr<Worker> self, uint64_t priority, uint64_t srA, uint64_t scA, uint64_t srB, uint64_t scB, uint64_t srC, uint64_t scC, uint64_t length, uint64_t dimension);
};

cown_ptr<Master> Master::make(uint64_t workers, uint64_t data_length, uint64_t threshold) {
  cown_ptr<Master> master = make_cown<Master>();
  when(master) << [workers, data_length, threshold, tag=master](acquired_cown<Master> master) {
    master->length = data_length;
    master->num_blocks = data_length * data_length;
    master->sent = 0;
    master->num_workers = workers;
    master->collector = make_cown<Collector>(tag, data_length);

    vector<vector<uint64_t>> a;
    vector<vector<uint64_t>> b;

    for (uint64_t i = 0; i < data_length; ++i) {
      vector<uint64_t> aI;
      vector<uint64_t> bI;

      for (uint64_t j = 0; j < data_length; ++j) {
        aI.push_back(i);
        bI.push_back(j);
      }

      a.push_back(aI);
      b.push_back(bI);
    }

    master->matrix_a = a;
    master->matrix_b = b;

    for (uint64_t k = 0; k < workers; ++k) {
      master->workers.push_back(make_cown<Worker>(tag, master->collector, a, b, threshold));
    }

    master->send_work(0, 0, 0, 0, 0, 0, 0, master->num_blocks, data_length);
  };
  return master;
}

void Master::send_work(uint64_t priority, uint64_t srA, uint64_t scA, uint64_t srB, uint64_t scB, uint64_t srC, uint64_t scC, uint64_t length, uint64_t dimension) {
  Worker::work(workers[(srC + scC) % workers.size()], priority, srA, scA, srB, scB, srC, scC, length, dimension);
  sent++;
}

void Master::work(cown_ptr<Master> self, uint64_t priority, uint64_t srA, uint64_t scA, uint64_t srB, uint64_t scB, uint64_t srC, uint64_t scC, uint64_t length, uint64_t dimension) {
  when(self) << [=](acquired_cown<Master> self) {
    self->send_work(priority, srA, scA, srB, scB, srC, scC, length, dimension);
  };
}

void Master::done(cown_ptr<Master> self) {
  when(self) << [](acquired_cown<Master> self) {
    if (--self->num_workers == 0) {
      /* done */
    }
  };
}

void Worker::work(cown_ptr<Worker> self, uint64_t priority, uint64_t srA, uint64_t scA, uint64_t srB, uint64_t scB, uint64_t srC, uint64_t scC, uint64_t length, uint64_t dimension) {
  when(self) << [=] (acquired_cown<Worker> self) mutable {
    if (length > self->threshold) {
      auto new_priority = priority + 1;
      auto new_dimension = dimension / 2;
      auto new_length = length / 4;

      Master::work(self->master, new_priority, srA, scA, srB, scB, srC, scC, new_length, new_dimension);
      Master::work(self->master, new_priority, srA, scA + new_dimension, srB + new_dimension, scB, srC, scC, new_length, new_dimension);
      Master::work(self->master, new_priority, srA, scA, srB, scB + new_dimension, srC, scC + new_dimension, new_length, new_dimension);
      Master::work(self->master, new_priority, srA, scA + new_dimension, srB + new_dimension, scB + new_dimension, srC, scC + new_dimension, new_length, new_dimension);
      Master::work(self->master, new_priority, srA + new_dimension, scA, srB, scB, srC + new_dimension, scC, new_length, new_dimension);
      Master::work(self->master, new_priority, srA + new_dimension, scA + new_dimension, srB + new_dimension, scB, srC + new_dimension, scC, new_length, new_dimension);
      Master::work(self->master, new_priority, srA + new_dimension, scA, srB, scB + new_dimension, srC + new_dimension, scC + new_dimension, new_length, new_dimension);
      Master::work(self->master, new_priority, srA + new_dimension, scA + new_dimension, srB + new_dimension, scB + new_dimension, srC + new_dimension, scC + new_dimension, new_length, new_dimension);
    } else {
      auto i = srC;
      auto dim = dimension;
      auto endR = i + dim;
      auto endC = scC + dim;

      vector<tuple<uint64_t, uint64_t, uint64_t>> partial_result;
      while (i < endR) {
        auto j = scC;

        while (j < endC) {
          uint64_t k = 0;
          uint64_t product = 0;

          while (k < dim) {
            product += self->matrix_a[i][scA + k] * self->matrix_b[srB + k][j];
            k++;
          }

          partial_result.push_back(make_tuple(i, j, product));
          j++;
        }

        i++;
      }

      Collector::collect(self->collector, move(partial_result));
    }
  };
}

};

struct Recmatmul: public AsyncBenchmark {
  uint64_t workers;
  uint64_t length;
  uint64_t threshold;

  Recmatmul(uint64_t workers, uint64_t length, uint64_t threshold, uint64_t priorities):
    workers(workers), length(length), threshold(threshold) {}

  void run() {
    recmatmul::Master::make(workers, length, threshold);
  }

  std::string name() { return "Recursive Matrix Multiplication"; }
};

};
