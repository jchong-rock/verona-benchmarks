#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../safe_print.h"
#include "../rng.h"

namespace jake_benchmark {

namespace leader_tree {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

template <typename K>
struct Counter {
    K remaining;
    std::function<void()> on_complete;

    Counter(K initial, std::function<void()> cb) : remaining(initial), on_complete(cb) {}

    static void done(cown_ptr<Counter<K>> self) {
        when(self) << [=](acquired_cown<Counter<K>> self) {
            if (self->remaining == 0)
                self->on_complete();
        };
    }

    static void add(cown_ptr<Counter<K>> self, K n = 1) {
        when(self) << [=](acquired_cown<Counter<K>> self) {
            self->remaining -= n;
        };
    }
};

struct Node {
    uint64_t id;
    cown_ptr<Node> parent;
    std::vector<cown_ptr<Node>> children;
    State state = Follower;
    uint64_t known_ids = 0;
    uint64_t highest_id;

    Node(uint64_t id): id(id), highest_id(id) {
        debug(" Made node with id : " , id);
    }

    Node(uint64_t id, cown_ptr<Node> parent): id(id), highest_id(id), parent(parent) {
        when (parent) << [=](acquired_cown<Node> parent) {
            debug(" Made node with id ", id, ": My parent is : ", parent->id);
        };
    }

    static void start(const cown_ptr<Node> & self) {
        when (self) << [=](acquired_cown<Node> self) {
            if (self->state != Candidate) {
                self->state = Candidate;
                if (self->parent)
                    start(self->parent);
                if (self->children.size() > 0) {
                    for (cown_ptr<Node> child : self->children)
                        start(child);
                }
                else
                    propagate_ids(self.cown());
            }
        };
    }

    static void propagate_ids(const cown_ptr<Node> & self) {
        when(self) << [=](acquired_cown<Node> self) {
            //debug(" propagate : ", self->highest_id);
            if (self->state == Candidate)
                receive_id(self->parent, self->highest_id);
        };
    }

    static void receive_id(const cown_ptr<Node> & self, uint64_t sender_id) {
        when(self) << [=](acquired_cown<Node> self) {
            //debug(" id : ", self->id, " -- recv prop from : ", sender_id);
            if (self->state == Candidate) {
                self->highest_id = std::max(sender_id, self->highest_id);
                self->known_ids++;
                //debug(" id : ", self->id, " kids : ", self->known_ids, ",", self->children.size());
                if (self->known_ids == self->children.size()) {
                    if (self->parent)
                        propagate_ids(self.cown());
                    else
                        declare_leader(self.cown());
                }
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
                for (auto child : self->children)
                    election_result(child, self->highest_id);
            }
        };
    }
};

};

struct LeaderTree: public ActorBenchmark {
    uint64_t servers;
    uint64_t max_nodes_per_layer;
    
    LeaderTree(uint64_t servers, uint64_t max_nodes_per_layer): servers(servers), max_nodes_per_layer(max_nodes_per_layer) {} 

    template <typename K>
    void init_children(cown_ptr<leader_tree::Node> parent, 
                    cown_ptr<std::vector<K>> children_per_node, 
                    cown_ptr<std::vector<K>> ids, 
                    cown_ptr<leader_tree::Counter<K>> counter) {
        using namespace leader_tree;
        when (children_per_node, parent, ids) << [=](
                    acquired_cown<std::vector<K>> children_per_node_list, 
                    acquired_cown<leader_tree::Node> parent, 
                    acquired_cown<std::vector<K>> id_list) {
            if (id_list->empty()) {
                Counter<K>::done(counter);
            } else {
                K num_children = children_per_node_list->back();
                children_per_node_list->pop_back();
                for (K i = 0; i < num_children; i++) {
                    cown_ptr<leader_tree::Node> c = make_cown<leader_tree::Node>(id_list->back(), parent.cown());
                    parent->children.push_back(c);
                    id_list->pop_back();
                    Counter<K>::add(counter);
                    init_children<K>(c, children_per_node, ids, counter);     
                }
            }
        };
    }

    void run() {
        using namespace leader_tree;
        cown_ptr<std::vector<uint64_t>> ids = make_cown<std::vector<uint64_t>>(gen_x_unique_randoms<uint64_t>(servers));
        when (make_cown<LeaderTree>(servers, max_nodes_per_layer)) << [=](acquired_cown<LeaderTree> ld) {
            cown_ptr<std::vector<uint64_t>> children_per_node = make_cown<std::vector<uint64_t>>(divide_randomly(servers-1, max_nodes_per_layer));
            // PRE: sum(children_per_node) == ids.size
            when (ids) << [=](acquired_cown<std::vector<uint64_t>> ids) mutable {
                // INV: children_per_node.size == 0 ==> ids.size == 0
                cown_ptr<leader_tree::Node> root = make_cown<leader_tree::Node>(ids->back());
                ids->pop_back();
                cown_ptr<Counter<uint64_t>> counter = make_cown<Counter<uint64_t>>(servers-1, [=]() {
                    Node::start(root);
                });
                init_children<uint64_t>(root, children_per_node, ids.cown(), counter);
            };
        };
    }
};

};