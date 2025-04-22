
#include "util/bench.h"
#include "util/random.h"
#include "typecheck.h"
#include <unordered_set>

uint64_t messages_we_sent = 0;
namespace actor_benchmark {

namespace leader {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Message {
    virtual ~Message() {};
};

struct ElectionMessage : public Message {
    uint64_t sender_id;
    State sender_state;

    ElectionMessage(uint64_t sender_id, State sender_state): sender_id(sender_id), sender_state(sender_state) {}
};

struct Server;

struct Mailbox {
    std::queue<std::shared_ptr<Message>> mb;
    Mailbox() {}
    static void recv(const cown_ptr<Mailbox> & self, std::shared_ptr<Message> msg) {
        //DEBUG("a message to you");
        when (self) << [=](acquired_cown<Mailbox> self) mutable {
            self->mb.push(msg);
            if (self->mb.size()) {
                //DEBUG("rudy");
            }
        };
    }

    template <typename T>
    static void handle_mail(const cown_ptr<Mailbox> & self, cown_ptr<Server> svr, std::function<void (cown_ptr<Server>, std::shared_ptr<T>)> action);
};

struct Server {
    uint16_t id;
    cown_ptr<Mailbox> mailbox = make_cown<Mailbox>();
    std::vector<cown_ptr<Server>> known_servers;
    State state = Follower;

    Server(uint64_t id): id(id) {
        DEBUG("id " << id);
    }

    static void check_mail(const cown_ptr<Server> & self);
    static void electionMessage(const cown_ptr<Server> & self, std::shared_ptr<ElectionMessage> m);

    static void recv(const cown_ptr<Server> & self, std::shared_ptr<Message> msg) {
        when (self) << [=](acquired_cown<Server> self) {
            //DEBUG("message to " << self->id);
            Mailbox::recv(self->mailbox, msg);
        };
    }

    template <typename T>
    static inline void send(const cown_ptr<Server> & recipient, T msg) {
        static_assert(std::is_base_of<Message, T>::value, "T must be a Message");
        //DEBUG("called send");
        Server::recv(recipient, std::make_shared<T>(msg));
    }

    template <typename T>
    static void broadcast(const cown_ptr<Server> & self, T msg) {
        static_assert(std::is_base_of<Message, T>::value, "T must be a Message");
        when (self) << [=](acquired_cown<Server> self) {
            for (auto s : self->known_servers)
                Server::recv(s, std::make_shared<T>(msg));
        };
    }

    static void election(const cown_ptr<Server> & self, uint16_t message_id) {
        when (self) << [=, tag=self](acquired_cown<Server> self) {
            self->state = Candidate;
            if (message_id == self->id) {
                self->state = Leader;
                Server::broadcast<ElectionMessage>(tag, ElectionMessage(message_id, Leader));
            }
            else {
                uint16_t highest_id = std::max(message_id, self->id);
                Server::broadcast<ElectionMessage>(tag, ElectionMessage(highest_id, Candidate));
            }
        };
    }
};

template <typename T>
void Mailbox::handle_mail(const cown_ptr<Mailbox> & self, cown_ptr<Server> svr, std::function<void (cown_ptr<Server>, std::shared_ptr<T>)> action) {
    when (self) << [=](acquired_cown<Mailbox> self) mutable {
        //DEBUG("doing a handle_mail size: " << self->mb.size());
        if (!self->mb.empty()) {
            std::shared_ptr<Message> m = self->mb.front();
            auto partial = [&](std::shared_ptr<T> k) { action(svr, k); };
            do_if_type<T>(m, partial);
            self->mb.pop();
        }
    };
    Server::check_mail(svr);
}

void Server::check_mail(const cown_ptr<Server> & self) {
    when (self) << [tag=self](acquired_cown<Server> self) mutable {
        //DEBUG(self->id << " did a check_mail, knows " << self->known_servers.size() << " others");
        /*if (messages_we_sent++ > 3000) {
            DEBUG("killing early due to message threshold");
            std::exit(2);
        } */
        Mailbox::handle_mail<ElectionMessage>(self->mailbox, tag, Server::electionMessage);
    };
}

void Server::electionMessage(const cown_ptr<Server> & self, std::shared_ptr<ElectionMessage> m) {
    //DEBUG( "Message with ID " << m->sender_id << " with state " << m->sender_state );
    switch (m->sender_state) {
        case Leader:
            when (self) << [=, tag=self](acquired_cown<Server> self) {
                if (self->state != Leader) {
                    self->state = Follower;
                    Server::broadcast(tag, ElectionMessage(m->sender_id, Leader));
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
template <uint16_t servers>
struct Leader: public ActorBenchmark {
    
    Leader() {} 

    template <typename K>
    static std::vector<K> gen_x_unique_randoms(K x) {
        static_assert(std::is_integral<K>::value &&
            std::is_unsigned<K>::value, "K must be an unsigned integer.");
        std::unordered_set<K> num_set;
        std::random_device rd;
        std::mt19937 gen(rd());
        K max_value = (1 << (sizeof(K) * 8)) - 1;
        std::uniform_int_distribution<K> dist(0, 65535);
        while (num_set.size() < x) {
            num_set.insert(dist(gen));
        }
        return std::vector<K>(num_set.begin(), num_set.end());
    }

    static void make() {
        using namespace leader;
        std::vector<uint16_t> ids = gen_x_unique_randoms<uint16_t>(servers);
        when (make_cown<Leader>()) << [=](acquired_cown<Leader> ld) {
            std::vector<cown_ptr<leader::Server>> server_list;
            for (uint16_t i = 0; i < servers; i++) {
                server_list.emplace_back(make_cown<Server>(ids[i]));
            }
            for (uint16_t i = 0; i < servers - 1; i++) {
                when (server_list[i], server_list[i + 1]) << [](acquired_cown<Server> svr, acquired_cown<Server> next) {
                    //DEBUG("made it 1");
                    svr->known_servers.emplace_back(next.cown());
                };
                Server::check_mail(server_list[i]);
            }

            when (server_list[servers - 1], server_list[0]) << [](acquired_cown<Server> svr, acquired_cown<Server> first) {
                //DEBUG("made it 2");
                svr->known_servers.emplace_back(first.cown());
            };
            Server::check_mail(server_list[servers - 1]);
            Server::send(server_list[0], ElectionMessage(0, Candidate));
        };
    }

    void run() {
        Leader::make();
    }
};

};