#include "util/bench.h"
#include "util/random.h"
#include "../typecheck.h"
#include "../safe_print.h"
#include "../rng.h"
#include <map>
#include <unistd.h>

#define MICROSECS 1000000

namespace jake_benchmark {

namespace breakfast {

    struct Food {
        virtual bool ready() = 0;
        virtual std::string item_name() = 0;
    };

    struct Bread : public Food {
        bool toasted = false;
        bool has_jam = false;
        bool has_butter = false;
        std::string item_name() override {
            return toasted ? "toast" : "bread";
        }
        bool ready() override {
            return toasted && has_jam && has_butter;
        }
        static void toast(cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                if (!self->toasted) {
                    debug("Begin toasting ", self->item_name());
                    usleep(8 * MICROSECS);
                    self->toasted = true;
                    debug("Finished making ", self->item_name());
                }
                else {
                    throw ("Burned " + self->item_name());
                }
            };
        }
        static void add_jam(cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                debug("Begin adding jam");
                if (self->toasted) {
                    usleep(MICROSECS / 4);
                    self->has_jam = true;
                    debug("Added jam to toast");
                }
                else {
                    throw "Cannot add jam to untoasted bread";
                }
            };
        }
        static void add_butter(cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                debug("Begin buttering toast");
                if (self->toasted) {
                    usleep(MICROSECS / 4);
                    self->has_butter = true;
                    debug("Buttered toast");
                }
                else {
                    throw "Ewww, buttered raw bread!";
                }
            };
        }
    };

    struct Cup : public Food {
        bool has_coffee = false;
        bool has_juice = false;
        bool ready() override {
            return has_coffee || has_juice;
        }
        std::string item_name() override {
            if (has_coffee) {
                return "cup of coffee";
            }
            if (has_juice) {
                return "cup of juice";
            }
            return "empty cup";
        }
        static void pour_coffee(cown_ptr<Cup> & self) {
            when (self) << [=](acquired_cown<Cup> self) {
                debug("Begin pouring coffee");
                if (self->has_coffee || self->has_juice) {
                    throw ("Full " + self->item_name());
                }
                else {
                    usleep(MICROSECS / 2);
                    self->has_coffee = true;
                    debug("Poured coffee");
                }
            };
        }
        static void pour_juice(cown_ptr<Cup> & self) {
            when (self) << [=](acquired_cown<Cup> self) {
                debug("Begin pouring juice");
                if (self->has_coffee || self->has_juice) {
                    throw ("Full " + self->item_name());
                }
                else {
                    usleep(MICROSECS / 2);
                    self->has_juice = true;
                    debug("Poured juice");
                }
            };
        }
        static void drink(cown_ptr<Cup> & self) {
            when (self) << [=](acquired_cown<Cup> self) {
                if (self->has_coffee) {
                    debug("Begin drinking coffee");
                    usleep(6 * MICROSECS);
                    self->has_coffee = false;
                    debug("Finished drinking coffee");
                }
                else if (self->has_juice) {
                    debug("Begin drinking juice");
                    usleep(3 * MICROSECS);
                    self->has_juice = false;
                    debug("Finished drinking juice");
                }
                else {
                    throw ("Cannot drink from empty cup");
                }
            };
        }
    };

    struct Bacon : public Food {
        int number;
        bool cooked = false;
        bool ready() override {
            return cooked;
        }
        Bacon(int number): number(number) {
            //debug("made bacon ", number);
        };
        int cook_time() {
            return 10;
        }
        std::string item_name() override {
            return "bacon " + std::to_string(number);
        }
    };

    struct Egg : public Food {
        int number;
        bool cooked = false;
        bool ready() override {
            return cooked;
        }
        Egg(int number): number(number) {
            //debug("cracked egg ", number);
        };
        int cook_time() {
            return 5;
        }
        std::string item_name() override {
            return ("egg " + std::to_string(number));
        }
    };

    using Fryable = std::variant<cown_ptr<Bacon>, cown_ptr<Egg>>;

    struct Pan {
        bool warm = false;
        int capacity;
        int spaces;
        std::queue<Fryable> queue;
        Pan(int capacity): capacity(capacity), spaces(capacity) {}

        static void heat_pan(cown_ptr<Pan> & self) {
            when (self) << [=](acquired_cown<Pan> self) {
                if (!self->warm) {
                    debug("Heating pan");
                    usleep(4 * MICROSECS);
                    self->warm = true;
                    debug("Pan is ready");
                }
            };
        }

        static void finish(const cown_ptr<Pan> & self) {
            when (self) << [=](acquired_cown<Pan> self) {
                self->spaces = std::min(self->capacity, self->spaces + 1);
                if (!self->queue.empty()) {
                    Pan::cook_item(self.cown(), self->queue.front());
                    self->queue.pop();
                }
            };
        }

        static void cook_item(const cown_ptr<Pan> & self, Fryable & item) {
            when (self) << [=](acquired_cown<Pan> tag) {
                if (tag->warm) {
                    if (tag->spaces > 0) {
                        tag->spaces--;
                        std::visit([=](auto & fryable_cown) {
                            when (fryable_cown) << [=](auto fryable) {
                                debug("Begin frying ", fryable->item_name());
                                usleep(fryable->cook_time() * MICROSECS);
                                if (!fryable->cooked) {
                                    fryable->cooked = true;
                                    debug("Finished frying ", fryable->item_name());
                                }
                                else {
                                    throw ("Burned " + fryable->item_name());
                                }
                                Pan::finish(self);
                            };
                        }, item);
                    }
                    else {
                        debug("Pan is full, queueing item");
                        tag->queue.push(item);
                    }
                }
                else {
                    throw "Cannot cook on cold pan";
                }
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
            cown_ptr<Bread> bread = make_cown<Bread>();
            cown_ptr<Cup> cup = make_cown<Cup>();
            std::vector<Fryable> fryables;

            cown_ptr<Pan> pan = make_cown<Pan>(bacon_num);
            Pan::heat_pan(pan);

            Bread::toast(bread);
            Bread::add_jam(bread);
            Bread::add_butter(bread);

            Cup::pour_coffee(cup);
            Cup::drink(cup);
            Cup::pour_juice(cup);

            for (int i = 1; i <= bacon_num; i++) {
                fryables.push_back(make_cown<Bacon>(i));
            }
            for (int i = 1; i <= egg_num; i++) {
                fryables.push_back(make_cown<Egg>(i));
            }
            
            for (auto & f : fryables) {
                Pan::cook_item(pan, f);
            }            
        };
    }
};

};