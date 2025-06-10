#include "util/bench.h"
#include "util/random.h"
#include "../../typecheck.h"
#include "../../rng.h"
#include "../../safe_print.h"
#include <chrono>

#define TIMEPOINT std::chrono::time_point<std::chrono::high_resolution_clock>

namespace jake_benchmark {

struct LeaderRingBoCTimed;

namespace leader_ring_boc_timed {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Node {
    uint64_t id;
    uint64_t highest_id;
    cown_ptr<Node> next;
    cown_ptr<jake_benchmark::LeaderRingBoCTimed> ld;
    State state = Follower;

    Node(uint64_t id, cown_ptr<jake_benchmark::LeaderRingBoCTimed> ld): id(id), highest_id(id), ld(ld) {
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
                //debug("Node ", self->id, " became leader");
                auto finish = std::chrono::high_resolution_clock::now();
                when (self->ld) << [=](auto ld) {
                    ld->finish(finish);
                };
            }
        };
    }    
};
};

struct LeaderRingBoCTimed {
    TIMEPOINT start;
    uint64_t servers;
    void finish(TIMEPOINT time) {
        auto ms = std::chrono::duration_cast<std::chrono::nanoseconds>(time - start);
        debug("boc: ", ms.count());
        std::exit(0);
    }

    LeaderRingBoCTimed(uint64_t servers): servers(servers) {} 
    static void make(uint64_t servers, std::vector<uint64_t> & ids, uint64_t starter) {
        using namespace leader_ring_boc_timed;
        when (make_cown<LeaderRingBoCTimed>(servers)) << [=](acquired_cown<LeaderRingBoCTimed> ld) {
            std::vector<cown_ptr<leader_ring_boc_timed::Node>> server_list;
            cown_ptr<uint64_t> completed = make_cown<uint64_t>(0);
            for (uint64_t i = 0; i < servers; i++) {
                server_list.emplace_back(make_cown<Node>(ids[i], ld.cown()));
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
            when (completed, ld.cown()) << [=](auto completed, auto ld) {
                if (completed == servers) {
                    ld->start = std::chrono::high_resolution_clock::now();
                    Node::share_ids(server_list[starter], server_list[starter + 1]);
                }
            };
        };
    }
};

};