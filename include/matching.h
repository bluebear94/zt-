#pragma once

#include <optional>

#include "PUnique.h"
#include "Rule.h"
#include "SCA.h"

namespace sca {
  struct PhonemeSpec;
  class SCA;
  class MatchResult {
  public:
    MatchResult(const PhonemeSpec* ps, bool owned, size_t index) :
      ps(ps), owned(owned), index(index) {}
    //MatchResult(const PhonemeSpec* ps, bool owned) : ps(ps), owned(owned) {}
    MatchResult(const MatchResult& mr) :
      ps(new PhonemeSpec(*mr.ps)), owned(true) {}
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
    size_t index;
  };
  bool charsMatch(
    const SCA& sca, const MChar& fr, const PhonemeSpec& fi, MatchCapture& mc);
  const PUnique<PhonemeSpec> applyOmega(
    const SCA& sca, MChar&& old, const MatchCapture& mc);
}
