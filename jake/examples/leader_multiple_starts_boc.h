#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../rng.h"

namespace jake_benchmark {

namespace leader_multi_start_boc {

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

    Node(uint64_t id): id(id) {
        std::cout << " Made Node with id : " << id << std::endl ;
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
                //std::exit(0);
            }
        };
    }
};

};
struct LeaderMultiStartBoC: public ActorBenchmark {
    uint64_t servers;
    uint64_t starters;
    
    LeaderMultiStartBoC(uint64_t servers, uint64_t starters): servers(servers), starters(starters) {} 

    void run() {
        using namespace leader_multi_start_boc;
        std::vector<uint64_t> ids = gen_x_unique_randoms<uint64_t>(servers, 65535);
        when (make_cown<LeaderMultiStartBoC>(servers, starters)) << [=](acquired_cown<LeaderMultiStartBoC> ld) {
            std::vector<cown_ptr<leader_multi_start_boc::Node>> server_list;
            for (uint64_t i = 0; i < servers; i++) {
                server_list.emplace_back(make_cown<Node>(ids[i]));
            }
            for (uint64_t i = 0; i < servers - 1; i++) {
                when (server_list[i]) << [next=server_list[i + 1]](acquired_cown<Node> svr) {
                    svr->next = next;
                };
            }

            when (server_list[servers - 1]) << [first=server_list[0]](acquired_cown<Node> svr) {
                svr->next = first;
            };
            std::vector<uint64_t> starts = gen_x_unique_randoms<uint64_t>(starters, servers-2);
            for (uint64_t i = 0; i < starters; i++) {
                Node::share_ids(server_list[starts[i]], server_list[starts[i+1]]);
            }
        };
    }
};

};