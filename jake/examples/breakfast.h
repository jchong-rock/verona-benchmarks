#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../safe_print.h"
#include "../rng.h"
#include <map>
#include <unistd.h>
#include <stdexcept>

#define MICROSECS 1000000

namespace jake_benchmark {

namespace breakfast {

    struct Bread {
        int slices;
        Bread(int slices): slices(slices) {}
        static const int toast_time = 8;
        bool toasted = false;

        void add_jam() {
            debug("Added jam to toast");
        }
        void add_butter() {
            debug("Buttered toast");
        }
        static void toast(const cown_ptr<Bread> & bread) {
            when (bread) << [=](acquired_cown<Bread> bread) {
                if (!bread->toasted) {
                    for (int i = 0; i < bread->slices; i++)
                        debug("Putting a slice of bread in the toaster");
                    debug("Begin toasting");
                    usleep(Bread::toast_time * MICROSECS);
                    bread->toasted = true;
                    debug("Remove toast from toaster");
                }
                else {
                    throw std::runtime_error("Burned toast!");
                }
            };
        }
    };

    struct Juice {
        Juice() {
            debug("Pouring orange juice");
        }
    };

    struct Coffee {
        Coffee() {
            debug("Pouring coffee");
        }
    };

    struct Bacon {
        int count;
        Bacon(int count): count(count) {}
        static const int cook_time = 10;
        
        static void fry(const cown_ptr<Bacon> & bacon) {
            when (bacon) << [=](auto bacon) {
                debug("Putting ", bacon->count, " slices of bacon in the pan");
                debug("Cooking first side of bacon");
                usleep(Bacon::cook_time * MICROSECS);
                for (int i = 0; i < bacon->count; i++) 
                    debug("Flipping a side of bacon");
                debug("Cooking second side of bacon");
                usleep(Bacon::cook_time * MICROSECS);
            };
        }
    };


    struct Egg {
        int count;
        Egg(int count): count(count) {}
        static const int cook_time = 5;
        
        static void fry(const cown_ptr<Egg> & egg) {
            when (egg) << [=](auto egg) {
                debug("Warming the egg pan");
                usleep(Egg::cook_time * MICROSECS);
                debug("Cracking ", egg->count, " eggs");
                debug("Cooking the eggs");
                usleep(Egg::cook_time * MICROSECS);
            };
        }
    };
};

struct Breakfast : public ActorBenchmark {
    int bacon_num;
    int egg_num;
    Breakfast(int bacon_num, int egg_num): bacon_num(bacon_num), egg_num(egg_num) {}

    void run() {
        using namespace breakfast;
        when (make_cown<Breakfast>(bacon_num,egg_num)) << [=](acquired_cown<Breakfast> bk) {
            Coffee();
            debug("Coffee is ready");

            cown_ptr<Bread> bread = make_cown<Bread>(2);
            cown_ptr<Bacon> bacon = make_cown<Bacon>(3);
            cown_ptr<Egg> egg = make_cown<Egg>(3);

            Bacon::fry(bacon);
            Egg::fry(egg);
            Bread::toast(bread);
            
            when (bread) << [](acquired_cown<Bread> bread) {
                bread->add_butter();
                bread->add_jam();
            };

            when (bread) << [](acquired_cown<Bread> bread) {
                    debug("Toast is ready");
            };
            when (egg) << [](acquired_cown<Egg> egg) {
                    debug("Eggs are ready");
            };
            when (bacon) << [](acquired_cown<Bacon> bacon) {
                    debug("Bacon is ready");
            };

            when (bread, egg, bacon) << [=](auto bread, auto egg, auto bacon) {
                Juice();
                debug("OJ is ready");
                debug("Finished making breakfast");
                std::exit(0);
            };
        };
    }
};

};