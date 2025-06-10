#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../rng.h"
#include "../safe_print.h"

namespace jake_benchmark {

namespace leader_ring {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Node {
    uint64_t id;
    cown_ptr<Node> next;
    State state = Follower;

    Node(uint64_t id): id(id) {
        debug(" Made Node with id : ", id);
    }

    static void propagate_id(const cown_ptr<Node> & self, uint64_t message_id) {
        when (self) << [=, tag=self](acquired_cown<Node> self) {
            self->state = Candidate;
            if (message_id == self->id) {
                self->state = Leader;
                declare_leader(tag, message_id);
            }
            else {
                uint64_t highest_id = std::max(message_id, self->id);
                propagate_id(self->next, highest_id);
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
                debug("Node ", self->id, " became leader");
                std::exit(0);
            }
        };
    }
};

};
struct LeaderRing: public ActorBenchmark {
    uint64_t servers;
    uint64_t starters;
    
    LeaderRing(uint64_t servers, uint64_t starters): servers(servers), starters(starters) {} 

    void run() {
        using namespace leader_ring;
        std::vector<uint64_t> ids = gen_x_unique_randoms<uint64_t>(servers, 65535);
        when (make_cown<LeaderRing>(servers, starters)) << [=](acquired_cown<LeaderRing> ld) {
            std::vector<cown_ptr<leader_ring::Node>> server_list;
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
            std::vector<uint64_t> starts = gen_x_unique_randoms<uint64_t>(starters, servers-1);
            for (uint64_t i = 0; i < starters; i++) {
                Node::propagate_id(server_list[starts[i]], 0);
            }
        };
    }
};

};