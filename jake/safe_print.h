#ifndef SAFE_PRINT_H
#define SAFE_PRINT_H

#include <sstream>

template<typename ...Args>
inline void debug(Args && ...args) {
    std::ostringstream oss;
    ((oss << args), ...);
    oss << std::endl;
    std::string var = oss.str();
    std::cout << var;
}

#endif