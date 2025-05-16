#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../safe_print.h"
#include "../rng.h"
#include <map>

namespace jake_benchmark {

namespace leader_arbitrary {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Node {
    uint64_t id;
    // mapping from neighbour ID to its cown
    std::map<uint64_t, cown_ptr<Node>> neighbours;
    State state = Follower;
    std::unordered_set<uint64_t> received_from;
    uint64_t highest_id;
    uint64_t total_servers;

    Node(uint64_t id, uint64_t counter): id(id), highest_id(id), total_servers(counter) {
        debug(" Made node with id : " , id);
    }

    static void add_neighbour(const cown_ptr<Node> & self, cown_ptr<Node> neighbour) {
        when (self, neighbour) << [=](acquired_cown<Node> self, acquired_cown<Node> neighbour) {
            self->neighbours[neighbour->id] = neighbour.cown();
            neighbour->neighbours[self->id] = self.cown();
        };
    }

    static void start(const cown_ptr<Node> & self, cown_ptr<uint64_t> sender_id) {
        when (self, sender_id) << [=](acquired_cown<Node> self, acquired_cown<uint64_t> sender_id) {
            debug(" start from : ", sender_id, " to id : ", self->id);
            if (self->state != Candidate) {
                self->state = Candidate;
                self->received_from.insert(self->id);
                for (auto const& [_, child] : self->neighbours)
                    start(child, make_cown<uint64_t>(self->id));
                propagate_ids(self.cown());
            }
        };
    }

    static void propagate_ids(const cown_ptr<Node> & self) {
        when(self) << [=](acquired_cown<Node> self) {
            if (self->state == Candidate) {
                for (auto const& [_, child] : self->neighbours)
                    receive_id(child, self->received_from, self->highest_id);
            }
        };
    }

    static void receive_id(const cown_ptr<Node> & self, std::unordered_set<uint64_t> seen, uint64_t highest_id) {
        when(self) << [=](acquired_cown<Node> self) {
            if (self->state == Candidate) {
                debug(" id : ", self->id, " -- recv prop : ", highest_id /*," seen: ", self->received_from.size()*/);
                self->received_from.insert(seen.begin(), seen.end());;
                if (self->received_from.size() == self->total_servers) {
                    declare_leader(self.cown());
                }
                else if (self->highest_id < highest_id) {
                    propagate_ids(self.cown());
                }
                self->highest_id = std::max(highest_id, self->highest_id);
            }
        };
    }

    static void election_result(const cown_ptr<Node> & self, uint64_t sender_id) {
        when(self) << [=](acquired_cown<Node> self) {
            if (self->state == Candidate) {
                self->highest_id = sender_id;
                declare_leader(self.cown());
            }
        };
    }

    static void declare_leader(const cown_ptr<Node> & self) {
        when(self) << [=](acquired_cown<Node> self) {
            debug(" Leader elected with id : ", self->highest_id);
            if (self->state == Candidate) {
                self->state = (self->id == self->highest_id) ? Leader : Follower;
                for (auto const& [_, child] : self->neighbours)
                    election_result(child, self->highest_id);
            }
        };
    }
};

};

struct LeaderArbitrary: public ActorBenchmark {
    uint64_t servers;
    uint64_t edges;
    
    LeaderArbitrary(uint64_t servers, uint64_t edges): servers(servers), edges(edges) {} 

    template <typename K>
    void init_nodes(cown_ptr<leader_arbitrary::Node> root, 
                    cown_ptr<std::vector<K>> ids,
                    std::function<void()> cb) {
        using namespace leader_arbitrary;
        when (root, ids) << [=](
                    acquired_cown<Node> root, 
                    acquired_cown<std::vector<K>> id_list) {
            std::vector<cown_ptr<Node>> nodes;
            while (!id_list->empty()) {
                nodes.push_back(make_cown<Node>(id_list->back(), servers));
                id_list->pop_back();
            }
            nodes.push_back(root.cown());
            for (K i = 1; i < nodes.size(); i++)
                Node::add_neighbour(nodes[i-1], nodes[i]);

            std::mt19937 rng;
            std::uniform_int_distribution<K> dist(0, nodes.size()-1);

            K remaining = edges;
            while (remaining > 0) {
                K from = dist(rng);
                K to = dist(rng);
                if (from != to) {
                    Node::add_neighbour(nodes[from], nodes[to]);
                    remaining--;
                }
            }
            cb();
        };
    }

    void run() {
        using namespace leader_arbitrary;
        cown_ptr<std::vector<uint64_t>> ids = make_cown<std::vector<uint64_t>>(gen_x_unique_randoms<uint64_t>(servers));
        when (make_cown<LeaderArbitrary>(servers, edges)) << [=](acquired_cown<LeaderArbitrary> ld) {
            // PRE: sum(children_per_node) == ids.size
            when (ids) << [=](acquired_cown<std::vector<uint64_t>> ids) mutable {
                // INV: children_per_node.size == 0 ==> ids.size == 0
                cown_ptr<Node> root = make_cown<Node>(ids->back(), servers);
                ids->pop_back();
                init_nodes<uint64_t>(root, ids.cown(), [=]() {
                    Node::start(root, make_cown<uint64_t>(1000000));
                });
            };
        };
    }
};

};