#pragma once

#include <stddef.h>

namespace sca {
  template<typename T1, typename T2>
  struct PHash {
    size_t operator()(const std::pair<T1, T2>& p) const {
      return (std::hash<T1>()(p.first) >> 1) ^ (std::hash<T2>()(p.second));
    }
  };
}
