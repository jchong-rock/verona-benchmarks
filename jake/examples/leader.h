#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../rng.h"

namespace jake_benchmark {

namespace leader {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Message {
    uint64_t sender_id;
    State sender_state;

    Message(uint64_t sender_id, State sender_state): sender_id(sender_id), sender_state(sender_state) {}
};

struct Server;

struct Mailbox {
    std::queue<std::shared_ptr<Message>> mb;
    Mailbox() {}
    static void recv(const cown_ptr<Mailbox> & self, std::shared_ptr<Message> msg) {
        when (self) << [=](acquired_cown<Mailbox> self) mutable {
            self->mb.push(msg);
        };
    }

    static void handle_mail(const cown_ptr<Mailbox> & self, cown_ptr<Server> svr, std::function<void (cown_ptr<Server>, std::shared_ptr<Message>)> action);
};

struct Server {
    uint64_t id;
    cown_ptr<Mailbox> mailbox = make_cown<Mailbox>();
    cown_ptr<Server> next;
    State state = Follower;

    Server(uint64_t id): id(id) {
        std::cout << " Made server with id : " << id << std::endl ;
    }

    static void check_mail(const cown_ptr<Server> & self);
    static void electionMessage(const cown_ptr<Server> & self, std::shared_ptr<Message> m);

    static void recv(const cown_ptr<Server> & self, std::shared_ptr<Message> msg) {
        when (self) << [=](acquired_cown<Server> self) {
            Mailbox::recv(self->mailbox, msg);
        };
    }

    static inline void send(const cown_ptr<Server> & recipient, Message msg) {
        Server::recv(recipient, std::make_shared<Message>(msg));
    }

    static void broadcast(const cown_ptr<Server> & self, Message msg) {
        when (self) << [=](acquired_cown<Server> self) {
            Server::recv(self->next, std::make_shared<Message>(msg));
        };
    }

    static void election(const cown_ptr<Server> & self, uint64_t message_id) {
        when (self) << [=, tag=self](acquired_cown<Server> self) {
            self->state = Candidate;
            if (message_id == self->id) {
                self->state = Leader;
                Server::broadcast(tag, Message(message_id, Leader));
            }
            else {
                uint64_t highest_id = std::max(message_id, self->id);
                Server::broadcast(tag, Message(highest_id, Candidate));
            }
        };
    }
};

void Mailbox::handle_mail(const cown_ptr<Mailbox> & self, cown_ptr<Server> svr, std::function<void (cown_ptr<Server>, std::shared_ptr<Message>)> action) {
    when (self) << [=](acquired_cown<Mailbox> self) mutable {
        if (!self->mb.empty()) {
            std::shared_ptr<Message> m = self->mb.front();
            action(svr, m);
            self->mb.pop();
        }
    };
    Server::check_mail(svr);
}

void Server::check_mail(const cown_ptr<Server> & self) {
    when (self) << [tag=self](acquired_cown<Server> self) mutable {
        Mailbox::handle_mail(self->mailbox, tag, Server::electionMessage);
    };
}

void Server::electionMessage(const cown_ptr<Server> & self, std::shared_ptr<Message> m) {
    switch (m->sender_state) {
        case Leader:
            when (self) << [=, tag=self](acquired_cown<Server> self) {
                if (self->state != Leader) {
                    self->state = Follower;
                    Server::broadcast(tag, Message(m->sender_id, Leader));
                }
                else {
                    std::cout << "Server " << self->id << " became leader" << std::endl;
                    std::exit(0);
                }
            };
            break;
        case Candidate:
            Server::election(self, m->sender_id);
            break;
        default:
            std::cerr << "Bad message: " << m->sender_state << std::endl;
    }
}

};
struct Leader: public ActorBenchmark {
    uint64_t servers;
    Leader(uint64_t servers): servers(servers) {} 

    void make() {
        using namespace leader;
        std::vector<uint64_t> ids = gen_x_unique_randoms<uint64_t>(servers);
        when (make_cown<Leader>(servers)) << [=](acquired_cown<Leader> ld) {
            std::vector<cown_ptr<leader::Server>> server_list;
            for (uint64_t i = 0; i < servers; i++) {
                server_list.emplace_back(make_cown<Server>(ids[i]));
            }
            for (uint64_t i = 0; i < servers - 1; i++) {
                when (server_list[i]) << [next=server_list[i + 1]](acquired_cown<Server> svr) {
                    svr->next = next;
                };
                Server::check_mail(server_list[i]);
            }

            when (server_list[servers - 1]) << [first=server_list[0]](acquired_cown<Server> svr) {
                svr->next = first;
            };
            Server::check_mail(server_list[servers - 1]);
            Server::send(server_list[0], Message(0, Candidate));
        };
    }

    void run() {
        Leader::make();
    }
};

};