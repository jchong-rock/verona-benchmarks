#ifndef RNG_H
#define RNG_H

#include <unordered_set>
#include <random>

template <typename K>
std::vector<K> gen_x_unique_randoms(K x, K max = 65535) {
    static_assert(std::is_integral<K>::value &&
        std::is_unsigned<K>::value, "K must be an unsigned integer.");
    std::unordered_set<K> num_set;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<K> dist(0, max);
    while (num_set.size() < x) {
        num_set.insert(dist(gen));
    }
    return std::vector<K>(num_set.begin(), num_set.end());
}

template <typename K>
std::vector<K> divide_randomly(K dividend, K max_quotient) {
    static_assert(std::is_integral<K>::value &&
        std::is_unsigned<K>::value, "K must be an unsigned integer.");
    K x = dividend;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<K> divisors;
    std::uniform_int_distribution<K> dist(1, max_quotient);
    while (x > 0) {
        K val = std::min(dist(gen), x);
        divisors.push_back(val);
        x -= val;
    }
    return divisors;
}

#endif