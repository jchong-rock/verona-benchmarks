#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../rng.h"

namespace jake_benchmark {

namespace leader_ring_boc {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Node {
    uint64_t id;
    uint64_t highest_id;
    cown_ptr<Node> next;
    State state = Follower;

    Node(uint64_t id): id(id), highest_id(id) {
        std::cout << " Made server with id : " << id << std::endl;
    }

    static void share_ids(const cown_ptr<Node> & self, const cown_ptr<Node> & next) {
        when (self, next) << [=](acquired_cown<Node> self, acquired_cown<Node> next) {
            self->state = Candidate;
            if (self->highest_id == next->id) {
                next->state = Leader;
                declare_leader(next.cown(), next->id);
            }
            else {
                uint64_t highest_id = std::max(next->highest_id, self->highest_id);
                self->highest_id = highest_id;
                next->highest_id = highest_id;
                share_ids(next.cown(), next->next);
            }
        };
    }

    static void declare_leader(const cown_ptr<Node> & self, uint64_t id) {
        when (self) << [=, tag=self](acquired_cown<Node> self) {
            if (self->state != Leader) {
                self->state = Follower;
                declare_leader(self->next, id);
            }
            else {
                std::cout << "Node " << self->id << " became leader" << std::endl;
                std::exit(0);
            }
        };
    }    
};
};

struct LeaderRingBoC: public BocBenchmark {
    uint64_t servers;
    uint64_t starters;
    LeaderRingBoC(uint64_t servers, uint64_t starters): servers(servers), starters(starters) {} 
    void run() {
        using namespace leader_ring_boc;
        std::vector<uint64_t> ids = gen_x_unique_randoms<uint64_t>(servers);
        when (make_cown<LeaderRingBoC>(servers, starters)) << [=](acquired_cown<LeaderRingBoC> ld) {
            std::vector<cown_ptr<leader_ring_boc::Node>> server_list;
            cown_ptr<uint64_t> completed = make_cown<uint64_t>(0);
            for (uint64_t i = 0; i < servers; i++) {
                server_list.emplace_back(make_cown<Node>(ids[i]));
            }
            for (uint64_t i = 0; i < servers - 1; i++) {
                when (server_list[i], completed) << [next=server_list[i + 1]](auto svr, auto completed) {
                    svr->next = next;
                    completed++;
                };
            }
            when (server_list[servers - 1], completed) << [first=server_list[0]](auto svr, auto completed) {
                svr->next = first;
                completed++;
            };
            when (completed) << [=](auto completed) {
                if (completed == servers) {
                    std::vector<uint64_t> starts = gen_x_unique_randoms<uint64_t>(1, servers-2);
                    //for (uint64_t i = 0; i < starters; i++)
                    //    Node::share_ids(server_list[starts[i]], server_list[starts[i]+1]);
                    Node::share_ids(server_list[starts[0]], server_list[starts[0]+1]);
                }
            };
        };
    }
};

};