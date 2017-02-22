#include <memory>

#ifndef MAKE_UNIQUE_H
#define MAKE_UNIQUE_H

// Obtained from Item 21 in Scott Meyer's Effective Modern C++

template<typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params)
{
  return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

#endif
