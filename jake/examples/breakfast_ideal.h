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

namespace breakfast_ideal {

    // We use inheritance to make it easy to extend this program
    // e.g. we could add Bagels, which could inherit from Toastable

    struct Food {
        virtual bool ready() = 0;
        virtual std::string item_name() = 0;
    };

    struct Appliance {
        
    };

    struct Toastable : public Food {
        bool toasted = false;
        virtual int toast_time() = 0;
        bool ready() override {
            return toasted;
        }
    };

    struct Bread : public Toastable {
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
                    usleep(MICROSECS / 4);
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
                    usleep(MICROSECS / 4);
                    self->has_butter = true;
                    debug("Buttered toast");
                }
                else {
                    throw std::runtime_error("Ewww, buttered raw bread!");
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
                    throw std::runtime_error("Full " + self->item_name());
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
                    throw std::runtime_error("Full " + self->item_name());
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
                    throw std::runtime_error("Cannot drink from empty cup");
                }
            };
        }
    };

    struct Pan;

    class Fryable : public Food {
      protected:
        bool cooked = false;
        virtual int cook_time() = 0;
        friend class Pan;
      public:
        bool ready() override {
            return cooked;
        }
    };

    struct Bacon : public Fryable {
        int number;
        Bacon(int number): number(number) {
            //debug("made bacon ", number);
        };
        int cook_time() override {
            return 10;
        }
        std::string item_name() override {
            return "bacon " + std::to_string(number);
        }
    };

    struct Egg : public Fryable {
        int number;
        Egg(int number): number(number) {
            //debug("cracked egg ", number);
        };
        int cook_time() override {
            return 5;
        }
        std::string item_name() override {
            return ("egg " + std::to_string(number));
        }
    };

    // since Verona's cown_ptr type doesnt support inheritance, we can cheat using variant and visit
    // we will store cowns in vectors using variant and dispatch their behaviours using visit
    using FryableCown = std::variant<cown_ptr<Bacon>, cown_ptr<Egg>>;
    using FoodCown = std::variant<cown_ptr<Bacon>, cown_ptr<Egg>, cown_ptr<Bread>, cown_ptr<Cup>>;
    using ToastableCown = std::variant<cown_ptr<Bread>>;

    // Bells are generic so we can use them for other appliances, e.g. microwaves
    template <typename T>
    struct Bell {
        std::function<void(T)> queued_callback;
        static void await(const cown_ptr<Bell<T>> & self, std::function<void(T)> callback) {
            when (self) << [=](acquired_cown<Bell<T>> self)  {
                if (self->queued_callback == nullptr) {
                    self->queued_callback = callback;
                }
            };
        }
        static void notify(const cown_ptr<Bell<T>> & self, T item) {
            when (self) << [=](acquired_cown<Bell<T>> self)  {
                if (self->queued_callback != nullptr) {
                    self->queued_callback(item);
                    self->queued_callback = nullptr;
                }
            };
        }
    };

    struct Toaster : public Appliance {
        int temperature = 0;
        cown_ptr<Bell<ToastableCown>> bell;

        Toaster(cown_ptr<Bell<ToastableCown>> bell): bell(bell) {}
        
        static void toast(const cown_ptr<Toaster> & self, ToastableCown item) {
            when (self) << [=](acquired_cown<Toaster> tag) {
                tag->temperature = std::max(10, tag->temperature + 1);
                std::visit([=](auto & toastable_cown) {
                    when (self, toastable_cown) << [=](acquired_cown<Toaster> self, auto toastable) {
                        if (!toastable->toasted) {
                            debug("Begin toasting ", toastable->item_name());
                            usleep(toastable->toast_time() * MICROSECS - self->temperature * 1000);
                            toastable->toasted = true;
                            debug("Finished making ", toastable->item_name());
                            Bell<ToastableCown>::notify(self->bell, toastable_cown);
                        }
                        else {
                            throw std::runtime_error("Burned " + toastable->item_name());
                        }
                    };
                }, item);
            };
        }
    };

    struct Pan : public Appliance {
        bool warm = false;
        int capacity;
        int spaces;
        std::queue<FryableCown> queue;
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

        static void cook_item(const cown_ptr<Pan> & self, FryableCown & item) {
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
                                    throw std::runtime_error("Burned " + fryable->item_name());
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
                    throw std::runtime_error("Cannot cook on cold pan");
                }
            };
        }
    };
};

struct Bool {
    bool value;
    Bool(bool value): value(value) {}
};

struct BreakfastIdeal : public ActorBenchmark {
    int bacon_num;
    int egg_num;
    BreakfastIdeal(int bacon_num, int egg_num): bacon_num(bacon_num), egg_num(egg_num) {}
    // this is not ideal, I would prefer to be able to acquire all cowns at once, 
    // but C++ makes it very difficult to turn a vector into a VARARGS list,
    // especially when each member of the vector is of a different type.
    void finish(std::vector<breakfast_ideal::FoodCown> food) {
        using namespace breakfast_ideal;
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
                debug("Finished making breakfast_ideal");
                std::exit(0);
            }
            else {
                finish(food);
            }
        };
    }

    void run() {
        using namespace breakfast_ideal;
        when (make_cown<BreakfastIdeal>(bacon_num,egg_num)) << [=](acquired_cown<BreakfastIdeal> bk) {
            cown_ptr<Bread> bread = make_cown<Bread>();
            cown_ptr<Cup> cup = make_cown<Cup>();
            std::vector<FryableCown> fryables;

            cown_ptr<Pan> pan = make_cown<Pan>(bacon_num);
            cown_ptr<Bell<ToastableCown>> toaster_bell = make_cown<Bell<ToastableCown>>();
            cown_ptr<Toaster> toaster = make_cown<Toaster>(toaster_bell);
            Pan::heat_pan(pan);

            Bell<ToastableCown>::await(toaster_bell, [=](ToastableCown t) {
                std::visit([=](auto & bread_cown) {
                    Bread::add_jam(bread_cown);
                    Bread::add_butter(bread_cown);
                }, t);
            });
            Toaster::toast(toaster, bread);

            Cup::pour_coffee(cup);
            Cup::drink(cup);
            Cup::pour_juice(cup);

            for (int i = 1; i <= bacon_num; i++) {
                fryables.push_back(make_cown<Bacon>(i));
            }
            for (int i = 1; i <= egg_num; i++) {
                fryables.push_back(make_cown<Egg>(i));
            }

            std::vector<FoodCown> food;

            for (auto & f : fryables) {
                Pan::cook_item(pan, f);
                std::visit([&](auto && cown) {
                    food.emplace_back(cown);
                }, f);
            }

            food.push_back(cup);
            food.push_back(bread);
            finish(food);
        };
    }
};

};