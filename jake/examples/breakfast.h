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

    // We use inheritance to make it easy to extend this program
    // e.g. we could add Bagels, which could inherit from Toastable

    struct Food {
        virtual bool ready() = 0;
        virtual std::string item_name() = 0;
    };

    struct Toastable : public Food {
        bool toasted = false;
        virtual int toast_time() = 0;
        bool ready() override {
            return toasted;
        }
    };

    struct Bread : public Toastable {
        int slices;
        Bread(int slices): slices(slices) {

        }
        bool has_jam = false;
        bool has_butter = false;
        std::string item_name() override {
            return toasted ? "toast" : "bread";
        }
        bool ready() override {
            return toasted && has_jam && has_butter;
        }
        int toast_time() override {
            return 8;
        }
        static void add_jam(cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                debug("Begin adding jam");
                if (self->toasted) {
                    self->has_jam = true;
                    debug("Added jam to toast");
                }
                else {
                    throw std::runtime_error("Cannot add jam to untoasted bread");
                }
            };
        }
        static void add_butter(cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                debug("Begin buttering toast");
                if (self->toasted) {
                    self->has_butter = true;
                    debug("Buttered toast");
                }
                else {
                    throw std::runtime_error("Ewww, buttered raw bread!");
                }
            };
        }
        static void toast(const cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                if (!self->toasted) {
                    for (int i = 0; i < self->slices; i++)
                        debug("Putting a slice of ", self->item_name(), " in the toaster");
                    debug("Begin toasting ", self->item_name());
                    usleep(self->toast_time() * MICROSECS);
                    self->toasted = true;
                    debug("Finished making ", self->item_name());
                }
                else {
                    throw std::runtime_error("Burned " + self->item_name());
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

    struct Fryable;
    struct Egg;
    struct Bacon;

    // since Verona's cown_ptr type doesnt support inheritance, we can cheat using variant and visit
    // we will store cowns in vectors using variant and dispatch their behaviours using visit
    using FryableCown = std::variant<cown_ptr<Bacon>, cown_ptr<Egg>>;
    using FoodCown = std::variant<cown_ptr<Bacon>, cown_ptr<Egg>, cown_ptr<Bread>>;

    class Fryable : public Food {
      protected:
        bool cooked = false;
        virtual int cook_time() = 0;
        virtual std::string begin_message() = 0;
        virtual std::string middle_message() = 0;
        std::string finish_message() {
            return "Put " + item_name() + " on plate";
        }
      public:
        bool ready() override {
            return cooked;
        }
        static void fry(const FryableCown & item) {
            std::visit([=](auto & fryable_cown) {
                when (fryable_cown) << [=](auto fryable) {
                    debug(fryable->begin_message());
                    usleep(fryable->cook_time() * MICROSECS);
                    debug(fryable->middle_message());
                    usleep(fryable->cook_time() * MICROSECS);
                    if (!fryable->cooked) {
                        fryable->cooked = true;
                        debug(fryable->finish_message());
                    }
                    else {
                        throw std::runtime_error("Burned " + fryable->item_name());
                    }
                };
            }, item);
        }
    };

    struct Bacon : public Fryable {
        int count;
        Bacon(int count): count(count) {

        }
        int cook_time() override {
            return 10;
        }
        std::string item_name() override {
            return "bacon";
        }
        std::string begin_message() override {
            return std::string("Putting ") + std::to_string(count) + " slices of bacon in the pan\nFrying first side of " + item_name();
        }
        std::string middle_message() override {
            std::string output;
            for (int i = 0; i < count; i++) 
                output += "Flipping a side of " + item_name() + "\n";
            return output + "Frying second side of " + item_name();
        }
    };

    struct Egg : public Fryable {
        int count;
        Egg(int count): count(count) {

        }
        int cook_time() override {
            return 5;
        }
        std::string item_name() override {
            return "eggs";
        }
        std::string begin_message() override {
            return "Warming the " + item_name() + " pan";
        }
        std::string middle_message() override {
            return std::string("Cracking ") + std::to_string(count) + " egg" + (count == 1 ? "\n" : "s\n") + "Cooking " + item_name();
        }
    };
};

struct Bool {
    bool value;
    Bool(bool value): value(value) {}
};

struct Breakfast : public ActorBenchmark {
    int bacon_num;
    int egg_num;
    Breakfast(int bacon_num, int egg_num): bacon_num(bacon_num), egg_num(egg_num) {}
    // this is not ideal, I would prefer to be able to acquire all cowns at once, 
    // but C++ makes it very difficult to turn a vector into a VARARGS list,
    // especially when each member of the vector is of a different type.
    void finish(std::vector<breakfast::FoodCown> food, std::function<void()> callback) {
        using namespace breakfast;
        cown_ptr<Bool> finished = make_cown<Bool>(true);
        for (auto & f : food) {
            std::visit([=](auto & food_cown) {
                when (food_cown, finished) << [=](auto food, auto finished) {
                    finished->value &= food->ready();
                };
            }, f);
        }
        when (finished) << [=](acquired_cown<Bool> finished) {
            if (finished->value) {
                callback();
                std::exit(0);
            }
            else {
                finish(food, callback);
            }
        };
    }

    void run() {
        using namespace breakfast;
        when (make_cown<Breakfast>(bacon_num,egg_num)) << [=](acquired_cown<Breakfast> bk) {
            cown_ptr<Bread> bread = make_cown<Bread>(2);
            cown_ptr<Bacon> bacon = make_cown<Bacon>(3);
            cown_ptr<Egg> egg = make_cown<Egg>(3);
            
            Coffee();

            Fryable::fry(bacon);
            Fryable::fry(egg);
            Bread::toast(bread);
            Bread::add_butter(bread);
            Bread::add_jam(bread);

            std::vector<FoodCown> food;
            
            food.push_back(egg);
            food.push_back(bacon);
            food.push_back(bread);
            finish(food, [=]() {
                Juice();
                debug("Finished making breakfast");
            });
        };
    }
};

};