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

    class Food : public VCown {
        virtual bool ready();
        virtual std::string item_name();
    };

    class Toastable : public Food {
      public:
        bool toasted = false;
        ready() override {
            return toasted;
        }
        static void toast(cown_ptr<Toastable> & self) {
            when (self) << [=](acquired_cown<Toastable> self) {
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
    }

    struct Pan;

    class Fryable : public Food {
      public:
        bool cooked = false;
        ready() override {
            return cooked;
        }
        virtual int cook_time();
      protected:
        static void fry(cown_ptr<Fryable> & self) {
            when (self) << [=](acquired_cown<Fryable> self) {
                debug("Begin frying ", self->item_name());
                if (!self->cooked) {
                    usleep(self->cook_time() * MICROSECS);
                    self->cooked = true;
                    debug("Finished frying ", self->item_name());
                }
                else {
                    throw ("Burned " + self->item_name());
                }
            };
        }
        friend class Pan;
    };

    struct Bread : public Toastable {
        bool has_jam = false;
        bool has_butter = false;
        item_name() override {
            return toasted ? "toast" : "bread";
        }
        ready() override {
            return toasted && has_jam && has_butter;
        }
        static void add_jam(cown_ptr<Bread> & self) {
            when (self) << [=](acquired_cown<Bread> self) {
                debug("Begin adding jam");
                if (toasted) {
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
                if (toasted) {
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
        ready() override {
            return has_coffee || has_juice;
        }
        item_name() override {
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

    struct Bacon : public Fryable {
        int number;
        Bacon(int number): number(number) {};
        cook_time() override {
            return 10;
        }
        item_name() override {
            return "bacon " + std::to_string(number);
        }
    }

    struct Egg : public Fryable {
        int number;
        Egg(int number): number(number) {};
        cook_time() override {
            return 5;
        }
        item_name() override {
            return "egg " + std::to_string(number);
        }
    }

    struct Pan {
        bool warm = false;

        static void heat_pan(cown_ptr<Pan> & self) {
            when (self) << [=](acquired_cown<Pan> self) {
                if (!self->warm) {
                    usleep(4 * MICROSECS);
                }
            };
        }

        static void cook_item(cown_ptr<Pan> & self, cown_ptr<Fryable> & item) {
            when (self) << [=](acquired_cown<Pan> self) {
                if (self->warm) {
                    Fryable::fry(item);
                }
            };
        }
        
    }
};

struct Breakfast : public ActorBenchmark {
    int bacons;
    int eggs;
    Breakfast(int bacons, int eggs): bacons(bacons), eggs(eggs) {}
    void run() {
        using namespace breakfast;
        when (make_cown<Breakfast>(bacons,eggs)) << [=](acquired_cown<Breakfast> bk) {
            cown_ptr<Bread> bread = make_cown<Bread>();
            cown_ptr<Cup> cup = make_cown<Cup>();
            std::vector<cown_ptr<Fryable>> fryables;

            Bread::toast(bread);
            Bread::add_jam(bread);
            Bread::add_butter(bread);

            Cup::pour_coffee(cup);
            Cup::drink(cup);
            Cup::pour_juice(cup);

            for (int i = 1; i <= bacons; i++) {
                fryables.push_back(make_cown<Bacon>(i));
            }
            for (int i = 1; i <= eggs; i++) {
                fryables.push_back(make_cown<Egg>(i));
            }
            
            cown_ptr<Pan> pan = make_cown<Pan>();
            Pan::heat_pan(pan);
            
            for (cown_ptr<Fryable> f : fryables) {
                Pan::cook_item(pan, f);
            }            
        };
    }
};

};