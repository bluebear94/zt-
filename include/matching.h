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
  class MatchResult {
  public:
    MatchResult(const PhonemeSpec* ps, bool owned) : ps(ps), owned(owned) {}
    MatchResult(const MatchResult& mr) = delete;
    MatchResult(MatchResult&& mr) : ps(mr.ps), owned(mr.owned) {
      mr.ps = nullptr;
      mr.owned = false;
    }
    MatchResult& operator=(MatchResult&& mr) {
      ps = mr.ps;
      owned = mr.owned;
      mr.ps = nullptr;
      mr.owned = false;
      return *this;
    }
    ~MatchResult() {
      if (owned) delete ps;
    }
    const PhonemeSpec* ps;
    bool owned;
  };
  using MatchCapture = std::unordered_map<
    std::pair<size_t, size_t>,
    MatchResult,
    PHash<size_t, size_t>>;
  bool charsMatch(
    const SCA& sca, const MChar& fr, const MChar& fi, MatchCapture& mc);
  MChar applyOmega(const SCA& sca, MChar&& old, const MatchCapture& mc);
}
