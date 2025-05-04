#include "util/bench.h"
#include "util/random.h"
#include "typecheck.h"
#include "safe_print.h"
#include <unordered_set>

namespace actor_benchmark {

namespace leader_dag {

typedef enum {
    HighestID,
    Elected
} MsgType;

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Message {
    uint64_t sender_id;
    MsgType msg_type;

    Message(uint64_t sender_id, MsgType msg_type): sender_id(sender_id), msg_type(msg_type) {}
};

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

struct Node;

struct Mailbox {
    std::queue<std::shared_ptr<Message>> mb;
    Mailbox() {}
    static void recv(const cown_ptr<Mailbox> & self, std::shared_ptr<Message> msg) {
        when (self) << [=](acquired_cown<Mailbox> self) mutable {
            self->mb.push(msg);
        };
    }

    static void handle_mail(const cown_ptr<Mailbox> & self, cown_ptr<Node> svr, std::function<void (cown_ptr<Node>, std::shared_ptr<Message>)> action);
};

struct Node {
    uint64_t id;
    cown_ptr<Node> parent;
    std::vector<cown_ptr<Node>> children;
    State state = Follower;
    uint64_t known_ids = 0;
    uint64_t highest_id;
    cown_ptr<Mailbox> mailbox = make_cown<Mailbox>();

    

    Node(uint64_t id): id(id), highest_id(id) {
        debug(" Made node with id : " , id);
    }

    Node(uint64_t id, cown_ptr<Node> parent): id(id), highest_id(id), parent(parent) {
        debug(" Made node with id : " , id);
        when (parent) << [=](acquired_cown<Node> parent) {
            debug(" id ", id, ": My parent is : ", parent->id);
        };
    }

    static void check_mail(const cown_ptr<Node> & self);

    static void recv(const cown_ptr<Node> & self, std::shared_ptr<Message> msg) {
        when (self) << [=](acquired_cown<Node> self) {
            Mailbox::recv(self->mailbox, msg);
        };
    }

    static inline void send(const cown_ptr<Node> & recipient, Message msg) {
        Node::recv(recipient, std::make_shared<Message>(msg));
    }

    static void start(const cown_ptr<Node> & self, cown_ptr<uint64_t> sender_id) {
        when (self, sender_id) << [=](acquired_cown<Node> self, acquired_cown<uint64_t> sender_id) {
            debug(" start from : ", sender_id, " to id : ", self->id);
            if (self->state != Candidate) {
                self->state = Candidate;
                if (self->parent)
                    start(self->parent, make_cown<uint64_t>(self->id));
                if (self->children.size() > 0) {
                    for (cown_ptr<Node> child : self->children)
                        start(child, make_cown<uint64_t>(self->id));
                }
                else
                    propagate_ids(self.cown());
            }
        };
    }

    static void propagate_ids(const cown_ptr<Node> & self) {
        when(self) << [=](acquired_cown<Node> self) {
            debug(" propagate : ", self->highest_id);
            if (self->state == Candidate)
                send(self->parent, Message(self->highest_id, HighestID));
        };
    }

    static void receive_id(const cown_ptr<Node> & self, std::shared_ptr<Message> msg) {
        switch (msg->msg_type) {
            case HighestID:
                when(self) << [=](acquired_cown<Node> self) {
                    debug(" id : ", self->id, " -- recv prop from : ", msg->sender_id);
                    if (self->state == Candidate) {
                        self->highest_id = std::max(msg->sender_id, self->highest_id);
                        self->known_ids++;
                        debug(" id : ", self->id, " kids : ", self->known_ids, ",", self->children.size());
                        if (self->known_ids == self->children.size()) {
                            if (self->parent) {
                                debug(" here1 ");
                                propagate_ids(self.cown());
                            }
                            else {
                                debug(" here2 ", self->id);
                                declare_leader(self.cown());
                            }
                        }
                    }
                };
                break;
            case Elected:
                when(self) << [=](acquired_cown<Node> self) {
                    if (self->state == Candidate) {
                        self->highest_id = msg->sender_id;
                        declare_leader(self.cown());
                    }
                };
                break;
            default:
                std::cerr << "Bad message: " << msg->msg_type << std::endl;
        }
    }

    static void declare_leader(const cown_ptr<Node> & self) {
        when(self) << [=](acquired_cown<Node> self) {
            debug(" Leader elected with id : ", self->highest_id);
            if (self->state == Candidate) {
                self->state = (self->id == self->highest_id) ? Leader : Follower;
                for (auto child : self->children) {
                    send(child, Message(self->highest_id, Elected));
                }
            }
        };
    }
};

void Mailbox::handle_mail(const cown_ptr<Mailbox> & self, cown_ptr<Node> svr, std::function<void (cown_ptr<Node>, std::shared_ptr<Message>)> action) {
    when (self) << [=](acquired_cown<Mailbox> self) mutable {
        if (!self->mb.empty()) {
            std::shared_ptr<Message> m = self->mb.front();
            action(svr, m);
            self->mb.pop();
        }
    };
    Node::check_mail(svr);
}

void Node::check_mail(const cown_ptr<Node> & self) {
    when (self) << [tag=self](acquired_cown<Node> self) mutable {
        Mailbox::handle_mail(self->mailbox, tag, Node::receive_id);
    };
}

};

struct LeaderDAG: public ActorBenchmark {
    uint64_t servers;
    uint64_t max_nodes_per_layer;
    
    LeaderDAG(uint64_t servers, uint64_t max_nodes_per_layer): servers(servers), max_nodes_per_layer(max_nodes_per_layer) {} 

    template <typename K>
    std::vector<K> gen_x_unique_randoms(K x) {
        static_assert(std::is_integral<K>::value &&
            std::is_unsigned<K>::value, "K must be an unsigned integer.");
        std::unordered_set<K> num_set;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<K> dist(0, 65536);
        while (num_set.size() < x) {
            num_set.insert(dist(gen));
        }
        return std::vector<K>(num_set.begin(), num_set.end());
    }

    template <typename K>
    std::vector<K> divide_randomly(K dividend, K max_quotient) {
        static_assert(std::is_integral<K>::value &&
            std::is_unsigned<K>::value, "K must be an unsigned integer.");
        K x = dividend;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::vector<K> divisors;
        std::uniform_int_distribution<K> dist(1, max_quotient);
        while (x > 0) {
            K val = std::min(dist(gen), x);
            divisors.push_back(val);
            x -= val;
        }
        return divisors;
    }

    template <typename K>
    void init_children(cown_ptr<leader_dag::Node> parent, 
                    cown_ptr<std::vector<K>> children_per_node, 
                    cown_ptr<std::vector<K>> ids, 
                    cown_ptr<leader_dag::Counter<K>> counter) {
        using namespace leader_dag;
        when (children_per_node, parent, ids) << [=](
                    acquired_cown<std::vector<K>> children_per_node_list, 
                    acquired_cown<leader_dag::Node> parent, 
                    acquired_cown<std::vector<K>> id_list) {
            if (id_list->empty()) {
                Counter<K>::done(counter);
            } else {
                K num_children = children_per_node_list->back();
                children_per_node_list->pop_back();
                for (K i = 0; i < num_children; i++) {
                    leader_dag::Node::check_mail(parent.cown());
                    cown_ptr<leader_dag::Node> c = make_cown<leader_dag::Node>(id_list->back(), parent.cown());
                    parent->children.push_back(c);
                    id_list->pop_back();
                    Counter<K>::add(counter);
                    init_children<K>(c, children_per_node, ids, counter);     
                }
            }
        };
    }

    void make() {
        using namespace leader_dag;
        cown_ptr<std::vector<uint64_t>> ids = make_cown<std::vector<uint64_t>>(gen_x_unique_randoms<uint64_t>(servers));
        when (make_cown<LeaderDAG>(servers, max_nodes_per_layer)) << [=](acquired_cown<LeaderDAG> ld) {
            cown_ptr<std::vector<uint64_t>> children_per_node = make_cown<std::vector<uint64_t>>(divide_randomly(servers-1, max_nodes_per_layer));
            // PRE: sum(children_per_node) == ids.size
            when (ids) << [=](acquired_cown<std::vector<uint64_t>> ids) mutable {
                // INV: children_per_node.size == 0 ==> ids.size == 0
                cown_ptr<leader_dag::Node> root = make_cown<leader_dag::Node>(ids->back());
                ids->pop_back();
                cown_ptr<Counter<uint64_t>> counter = make_cown<Counter<uint64_t>>(servers-1, [=]() {
                    Node::start(root, make_cown<uint64_t>(100));
                });
                init_children<uint64_t>(root, children_per_node, ids.cown(), counter);
            };
        };
    }

    void run() {
        LeaderDAG::make();
    }
};

};