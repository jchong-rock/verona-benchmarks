#ifndef TYPECHECK_H
#define TYPECHECK_H
#include <cpp/when.h>

//#define PRINT(...) for (auto p : __VA_ARGS__) {std::cout << p;} std::cout << std::endl;

#define DEBUG(k) std::cout<<k<<std::endl;

template <typename T, typename B>
void do_if_type(std::shared_ptr<B> value, std::function<void (std::shared_ptr<T>)> action) {
    auto casted = std::dynamic_pointer_cast<T>(value);
    if (casted != nullptr) {
        action(casted);
    }
}
#endif // TYPECHECK_H