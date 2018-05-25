#pragma once

#include <stddef.h>

#include <functional>
#include <utility>

namespace sca {
  class SCA;
  struct PhonemeSpec;
  template<typename T1, typename T2>
  struct PHash {
    size_t operator()(const std::pair<T1, T2>& p) const {
      return (std::hash<T1>()(p.first) >> 1) ^ (std::hash<T2>()(p.second));
    }
  };
  struct PSHash {
    size_t operator()(const PhonemeSpec& ps) const;
    const SCA* sca;
  };
  bool arePhonemeSpecsEqual(
    const SCA& sca, const PhonemeSpec& a, const PhonemeSpec& b);
  struct PSEqual {
    size_t operator()(const PhonemeSpec& a, const PhonemeSpec& b) const {
      return arePhonemeSpecsEqual(*sca, a, b);
    }
    const SCA* sca;
  };
}
