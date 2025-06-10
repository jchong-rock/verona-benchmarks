#include "util/bench.h"
#include "util/random.h"
#include "../../typecheck.h"
#include "../../rng.h"
#include "../../safe_print.h"
#include <chrono>

#define TIMEPOINT std::chrono::time_point<std::chrono::high_resolution_clock>

namespace jake_benchmark {

struct LeaderRingTimed;

namespace leader_ring_timed {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Node {
    uint64_t id;
    cown_ptr<Node> next;
    cown_ptr<jake_benchmark::LeaderRingTimed> ld;
    State state = Follower;

    Node(uint64_t id, cown_ptr<jake_benchmark::LeaderRingTimed> ld): id(id), ld(ld) {
        //debug(" Made Node with id : ", id);
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
        when (self) << [=](acquired_cown<Node> self) {
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
struct LeaderRingTimed {
    uint64_t servers;
    TIMEPOINT start;
    LeaderRingTimed(uint64_t servers): servers(servers) {} 

    void finish(TIMEPOINT time) {
        auto ms = std::chrono::duration_cast<std::chrono::nanoseconds>(time - start);
        debug("act: ", ms.count());
        std::exit(0);
    }

    static void make(uint64_t servers, std::vector<uint64_t> & ids, uint64_t starter) {
        using namespace leader_ring_timed;
        when (make_cown<LeaderRingTimed>(servers)) << [=](acquired_cown<LeaderRingTimed> ld) {
            std::vector<cown_ptr<leader_ring_timed::Node>> server_list;
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
                ld->start = std::chrono::high_resolution_clock::now();
                Node::propagate_id(server_list[starter], 0);
            };
        };
    }
};

};