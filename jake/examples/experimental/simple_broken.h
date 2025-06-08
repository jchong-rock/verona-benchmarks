#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../rng.h"
#include <sstream>

namespace jake_benchmark {

namespace simple_broken {

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
        std::cout << " Made node with id : " << id << std::endl;
    }

    Node(uint64_t id, cown_ptr<Node> parent): id(id), highest_id(id), parent(parent) {
        std::ostringstream oss;
        oss << " Made node with id : " << id << std::endl;
        std::string var = oss.str();
        std::cout << var;
        when (parent) << [=](acquired_cown<Node> parent) {
            std::ostringstream oss;
            oss << " id "<< id << ": My parent is : " << parent->id << std::endl;
            std::string var = oss.str();
            std::cout << var;
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
            std::ostringstream oss;
            oss << " start from : " << sender_id << " to id : " << self->id << std::endl;
            std::string var = oss.str();
            std::cout << var;
            if (self->state != Candidate) {
                self->state = Candidate;
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
            std::ostringstream oss;
            oss << " propagate : " << self->highest_id << std::endl;
            std::string var = oss.str();
            std::cout << var;
            if (self->state == Candidate)
                send(self->parent, Message(self->highest_id, HighestID));
        };
    }

    static void receive_id(const cown_ptr<Node> & self, std::shared_ptr<Message> msg) {
        if (msg->msg_type != HighestID)
            return;
        when(self) << [=](acquired_cown<Node> self) {
            std::ostringstream oss;
            oss << " id : " << self->id << " -- recv prop from : " << msg->sender_id << std::endl;
            std::string var = oss.str();
            std::cout << var;
            if (self->state == Candidate) {
                self->highest_id = std::max(msg->sender_id, self->highest_id);
                self->known_ids++;
                std::ostringstream oss;
                oss << " id : " << self->id << " kids : " << self->known_ids <<","<< self->children.size() << std::endl;
                std::string var = oss.str();
                std::cout << var;
                if (self->known_ids == self->children.size()) {
                    if (self->parent) {
                        std::cout << " here1 "<< std::endl;
                        propagate_ids(self.cown());
                    }
                    else {
            
                        std::ostringstream oss;
                        oss << " here2 " << self->id << std::endl;
                        std::string var = oss.str();
                        std::cout << var;
                        declare_leader(self.cown());
                    }
                }
            }
        };
    }

    static void declare_leader(const cown_ptr<Node> & self) {
        when(self) << [=](acquired_cown<Node> self) {
            std::ostringstream oss;
            oss << " Leader elected with id : " << self->highest_id << std::endl;
            std::string var = oss.str();
            std::cout << var;
            if (self->state == Candidate) {
                self->state = (self->id == self->highest_id) ? Leader : Follower;
                for (auto child : self->children) {
                    send(child, Message(self->highest_id, Elected));
                }
            }
        };
    }

    static void receive_elected(const cown_ptr<Node> & self, std::shared_ptr<Message> msg) {
        if (msg->msg_type != Elected)
            return;
        when(self) << [=](acquired_cown<Node> self) {
            if (self->state == Candidate) {
                self->highest_id = msg->sender_id;
                declare_leader(self.cown());
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
        Mailbox::handle_mail(self->mailbox, tag, Node::receive_elected);
        Mailbox::handle_mail(self->mailbox, tag, Node::receive_id);
    };
}

};

struct SimpleBroken: public ActorBenchmark {
    uint64_t servers = 10;
    uint64_t max_nodes_per_layer = 4;
    
    SimpleBroken(uint64_t servers, uint64_t max_nodes_per_layer) {} 

    template <typename K>
    void init_children(cown_ptr<simple_broken::Node> parent, 
                    cown_ptr<std::vector<K>> children_per_node, 
                    cown_ptr<std::vector<K>> ids, 
                    cown_ptr<simple_broken::Counter<K>> counter) {
        using namespace simple_broken;
        when (children_per_node, parent, ids) << [=](
                    acquired_cown<std::vector<K>> children_per_node_list, 
                    acquired_cown<simple_broken::Node> parent, 
                    acquired_cown<std::vector<K>> id_list) {
            if (id_list->empty()) {
                Counter<K>::done(counter);
            } else {
                K num_children = children_per_node_list->back();
                children_per_node_list->pop_back();
                for (K i = 0; i < num_children; i++) {
                    simple_broken::Node::check_mail(parent.cown());
                    cown_ptr<simple_broken::Node> c = make_cown<simple_broken::Node>(id_list->back(), parent.cown());
                    parent->children.push_back(c);
                    id_list->pop_back();
                    Counter<K>::add(counter);
                    init_children<K>(c, children_per_node, ids, counter);     
                }
            }
        };
    }

    void make() {
        using namespace simple_broken;
        std::vector<uint64_t> i {1,5,4,6,7,2,8,9,0,3};
        cown_ptr<std::vector<uint64_t>> ids = make_cown<std::vector<uint64_t>>(i);
        when (make_cown<SimpleBroken>(servers, max_nodes_per_layer)) << [=](acquired_cown<SimpleBroken> ld) {
            std::vector<uint64_t> c {2,2,1,1,1,2};
            cown_ptr<std::vector<uint64_t>> children_per_node = make_cown<std::vector<uint64_t>>(c);
            // PRE: sum(children_per_node) == ids.size
            when (ids) << [=](acquired_cown<std::vector<uint64_t>> ids) mutable {
                // INV: children_per_node.size == 0 ==> ids.size == 0
                cown_ptr<simple_broken::Node> root = make_cown<simple_broken::Node>(ids->back());
                ids->pop_back();
                cown_ptr<Counter<uint64_t>> counter = make_cown<Counter<uint64_t>>(servers-1, [=]() {
                    Node::start(root, make_cown<uint64_t>(100));
                });
                init_children<uint64_t>(root, children_per_node, ids.cown(), counter);
            };
        };
    }

    void run() {
        SimpleBroken::make();
    }
};

};