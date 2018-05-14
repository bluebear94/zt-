#pragma once

#include <optional>

#include "Rule.h"
#include "SCA.h"

namespace sca {
  struct PhonemeSpec;
  class SCA;
  template<typename T1, typename T2>
  struct PHash {
    size_t operator()(const std::pair<T1, T2>& p) const {
      return (std::hash<T1>()(p.first) >> 1) ^ (std::hash<T2>()(p.second));
    }
  };
  using MatchCapture = std::unordered_map<
    std::pair<size_t, size_t>,
    const PhonemeSpec*,
    PHash<size_t, size_t>>;
  bool charsMatch(
    const SCA& sca, const MChar& fr, const MChar& fi, MatchCapture& mc);
  MChar applyOmega(const SCA& sca, MChar&& old, const MatchCapture& mc);
}
