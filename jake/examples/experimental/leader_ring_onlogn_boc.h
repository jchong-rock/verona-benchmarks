#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../rng.h"
#include "../safe_print.h"                                                          

namespace jake_benchmark {

namespace leader_ring_onlogn_boc {

typedef enum {
    Leader,
    Follower,
    Candidate
} State;

struct Node {
    const uint64_t id;
    cown_ptr<Node> left;
    cown_ptr<Node> right;
    State state = Candidate;

    Node(uint64_t id): id(id) {
        debug(" Made server with id : ", id);
    }

    static void election(const cown_ptr<Node> & left, const cown_ptr<Node> & self, const cown_ptr<Node> & right) {
        // if both left and right are the same node, then we deadlock. this is not ideal
        /*if (left == self || self == right) {
            when (self) << [=](acquired_cown<Node> self) {
                self->state = Leader;                                   
                    debug("Node ", self->id, " became leader");             
                    std::exit(0);   
            };
        }
        else*/
        if (false) {
            when (left, self) << [=](acquired_cown<Node> left, acquired_cown<Node> self) {
                debug("L=", left->id, ",S=", self->id, ",R=", left->id);
                if (left->id > self->id) {
                    debug("L ", left->id); 
                    self->state = Follower;       
                    election(left->left, left.cown(), left->right);
                }
                else {
                    debug("S ", self->id); 
                    left->state = Follower;       
                    election(self->left, self.cown(), self->right);
                }
                std::exit(5);
            };
        }
        else {
            when (left, self, right) << [=](acquired_cown<Node> left, acquired_cown<Node> self, acquired_cown<Node> right) {
                debug("L=", left->id, ",S=", self->id, ",R=", right->id);   
                if (left->state == Follower && right->state == Follower) {  
                    self->state = Leader;                                   
                    debug("Node ", self->id, " became leader");             
                    std::exit(0);                                           
                }                                                           
                else if (left->id > right->id) {     
                    debug("S ", self->id);                                  
                    left->state = Follower;                                 
                    right->state = Follower;                                
                    election(left->left, self.cown(), right->right);        
                }                                                           
                else if (left->id > self->id && left->id > right->id) {     
                    debug("L ", left->id);                                   
                    self->state = Follower;                                 
                    right->state = Follower;                                
                    election(left->left, left.cown(), right->right);        
                }                                                           
                else if (right->id > self->id && right->id > left->id) {    
                    debug("R ", right->id);                                 
                    left->state = Follower;                                 
                    self->state = Follower;                                 
                    election(left->left, right.cown(), right->right);       
                }
            };
        }
    } 
};
};

struct LeaderRingOnlognBoC: public BocBenchmark {
    uint64_t servers;
    LeaderRingOnlognBoC(uint64_t servers): servers(servers) {} 
    void run() {
        using namespace leader_ring_onlogn_boc;
        std::vector<uint64_t> ids = gen_x_unique_randoms<uint64_t>(servers, 11);
        when (make_cown<LeaderRingOnlognBoC>(servers)) << [=](acquired_cown<LeaderRingOnlognBoC> ld) {
            std::vector<cown_ptr<leader_ring_onlogn_boc::Node>> server_list;
            for (uint64_t i = 0; i < servers; i++) {
                server_list.emplace_back(make_cown<Node>(ids[i]));
            }
            for (uint64_t i = 0; i < servers - 1; i++) {
                when (server_list[i], server_list[i + 1]) << [](acquired_cown<Node> svr, acquired_cown<Node> next) {
                    svr->right = next.cown();
                    next->left = svr.cown();
                };
            }
            when (server_list[servers - 1], server_list[0]) << [](acquired_cown<Node> svr, acquired_cown<Node> first) {
                svr->right = first.cown();
                first->left = svr.cown();
            };
            Node::election(server_list[0], server_list[1], server_list[2]);
        };
    }
};

};