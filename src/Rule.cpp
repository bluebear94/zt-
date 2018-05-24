#include "Rule.h"

#include <assert.h>

#include <algorithm>
#include <iostream>

#include "SCA.h"
#include "iterutils.h"
#include "matching.h"

namespace sca {
  size_t PhonemeSpec::getFeatureValue(size_t f, const SCA& sca) const {
    return (f < featureValues.size()) ?
      featureValues[f] :
      sca.getFeatureByID(f).def;
  }
  void PhonemeSpec::setFeatureValue(size_t f, size_t i, const SCA& sca) {
    size_t os = featureValues.size();
    if (f >= os) {
      featureValues.resize(f + 1);
      for (size_t i = os; i < f; ++i) {
        featureValues[i] = sca.getFeatureByID(i).def;
      }
    }
    featureValues[f] = i;
  }
  size_t CharMatcher::Constraint::evaluate(
      size_t i, const MatchCapture& mc, const SCA& sca) const {
    return std::visit([&](const auto& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, size_t>) return arg;
      else {
        auto it = mc.find(arg);
        assert(it != mc.end());
        return it->second.ps->getFeatureValue(feature, sca);
      }
    }, instances[i]);
  }
  bool CharMatcher::Constraint::matches(
      size_t inst, const MatchCapture& mc, const SCA& sca) const {
    std::vector<size_t> itrans(instances.size());
    for (size_t i = 0; i < instances.size(); ++i) {
      itrans[i] = evaluate(i, mc, sca);
    }
    switch (c) {
      case Comparison::eq: {
        auto it = std::find(itrans.begin(), itrans.end(), inst);
        return it != itrans.end();
      }
      case Comparison::ne: {
        auto it = std::find(itrans.begin(), itrans.end(), inst);
        return it == itrans.end();
      }
      #define CMPCASE(name, op) \
        case Comparison::name: { \
          return std::all_of(itrans.begin(), itrans.end(), \
            [inst](size_t o) { \
              return inst op o; \
            }); \
        }
      CMPCASE(lt, <)
      CMPCASE(gt, >)
      CMPCASE(le, <=)
      CMPCASE(ge, >=)
    }
  }
  std::string CharMatcher::toString(const SCA& sca) const {
    return sca.getClassByID(charClass).name + ":" + std::to_string(index);
  }
  static const char* opNames[] = {
    "=", "!=",
    "<", ">",
    "<=", ">="
  };
  std::string CharMatcher::Constraint::toString(const SCA& sca) const {
    const auto& f = sca.getFeatureByID(feature);
    std::string s = f.featureName + opNames[(int) c];
    for (size_t i = 0; i < instances.size(); ++i) {
      if (i != 0) s += ' ';
      std::visit([&](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, size_t>) {
          s += f.instanceNames[arg];
        } else {
          s += sca.getClassByID(arg.first).name;
          s += ':';
          s += std::to_string(arg.second);
        }
      }, instances[i]);
    }
    return s;
  }
  // ------------------------------------------------------------------
  template<typename T>
  static void replaceSubrange(
      std::vector<T>& v1,
      size_t b1,
      size_t e1,
      typename std::vector<T>::iterator b2,
      typename std::vector<T>::iterator e2) {
    size_t size1 = (size_t) (e1 - b1);
    size_t size2 = (size_t) (e2 - b2);
    size_t oldSize = v1.size();
    // Shift left or right?
    if (size1 > size2) { // Left
      std::copy(
        v1.begin() + e1, v1.end(),
        v1.begin() + b1 + size2);
      v1.resize(v1.size() + size2 - size1);
    } else if (size1 < size2) { // Right
      v1.resize(v1.size() + size2 - size1);
      std::copy_backward(
        v1.begin() + e1, v1.begin() + oldSize,
        v1.end());
    }
    std::copy(b2, e2, v1.begin() + b1);
  }
  template<typename T>
  static void replaceSubrange(
      std::vector<T>& v1,
      typename std::vector<T>::iterator b1,
      typename std::vector<T>::iterator e1,
      typename std::vector<T>::iterator b2,
      typename std::vector<T>::iterator e2) {
    replaceSubrange(v1, b1 - v1.begin(), e1 - v1.begin(), b2, e2);
  }
  // ------------------------------------------------------------------
  template<typename Fwd>
  static std::optional<Fwd> matchesMChar(
    Fwd istart, Fwd iend,
    const MChar& ruleChar,
    const SCA& sca,
    MatchCapture& mc
  ) {
    if (ruleChar.isSingleCharacter()) {
      if (istart == iend) return std::nullopt;
      bool match = charsMatch(sca, ruleChar, *istart, mc);
      if (match) return istart + 1;
      else return std::nullopt;
    }
    // Does not necessarily match one and only one.
    return std::visit([&](const auto& arg) -> std::optional<Fwd> {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Alternation>) {
        // Try each option in succession and pick the first one that works
        for (const MString& opt : arg.options) {
          // Back up the MatchCapture, in case this option fails
          MatchCapture mcback(mc);
          auto it = matchesPattern(
            istart, iend,
            IRev<Fwd>::cbegin(opt), IRev<Fwd>::cend(opt),
            sca, mc
          );
          if (it.has_value()) { // Success!
            return *it;
          }
          mc = mcback; // Restore
        }
        return std::nullopt;
      } else if constexpr (std::is_same_v<T, Repeat>) {
        size_t nCopies = 0;
        Fwd it = istart;
        while (true) {
          if (it >= iend) break; // Passed the end; can't match anymore
          if (nCopies > arg.max) break; // Can't match more copies
          auto matchEnd = matchesPattern(
            it, iend,
            IRev<Fwd>::cbegin(arg.s), IRev<Fwd>::cend(arg.s),
            sca, mc
          );
          if (!matchEnd.has_value()) break; // No match
          ++nCopies; // Otherwise, record success and prepare for next
          it = *matchEnd;
        }
        if (nCopies < arg.min || nCopies > arg.max) return std::nullopt;
        return it;
      } else {
        std::cerr << "matchesMChar: We missed a case!\n";
        abort();
        return std::nullopt;
      }
    }, ruleChar.value);
  }
  // If the pattern matches the text at the start point, return the iterator
  // to the end of the match. Otherwise, return std::nullopt.
  template<typename Fwd, typename CFwd
  > // templated to handle both fwd and rev cases
  static std::optional<Fwd> matchesPattern(
    Fwd istart, // where to start looking
    Fwd iend, // end of string (stop looking when you reach here)
    CFwd rstart, // iterator to start of rule
    CFwd rend, // iterator to end of rule
    const SCA& sca,
    MatchCapture& mc
  ) {
    Fwd iit = istart;
    CFwd rit = rstart;
    while (true) {
      if (rit == rend) // all chars matched
        return iit;
      if (iit == iend) { // end of string but unmatched chars
        if (rit->template is<Space>()) return iit;
        return std::nullopt;
      }
      const auto& inRule = *rit;
      auto matchEnd = matchesMChar(iit, iend, inRule, sca, mc);
      if (!matchEnd.has_value()) return std::nullopt;
      ++rit;
      iit = *matchEnd;
    }
  }
  template<typename Fwd, typename CFwd>
  static std::optional<Fwd> matchesRule(
    Fwd istart, Fwd ipoint, Fwd iend, // text start / search start / text end
    CFwd astart, CFwd aend, // alpha
    const std::vector<std::pair<MString, MString>>& envs, // envs
    bool envInverted, // Match if environment is NOT matched (vs matched)?
    const SCA& sca,
    MatchCapture& mc
  ) {
    auto amatch = matchesPattern(ipoint, iend, astart, aend, sca, mc);
    if (!amatch) return std::nullopt;
    Fwd ipend = *amatch;
    auto matchesEnv = [=, &sca, &mc]() -> bool {
      // Special case: if there's no environment, then always pass
      // the environment check
      if (envs.empty()) return true;
      for (const auto& p : envs) {
        bool swap = IRev<Fwd>::shouldSwapLR;
        const auto& lambda = swap ? p.second : p.first;
        const auto& rho = swap ? p.first : p.second;
        CFwd lstart = IRev<Fwd>::cbegin(lambda);
        CFwd lend = IRev<Fwd>::cend(lambda);
        CFwd rstart = IRev<Fwd>::cbegin(rho);
        CFwd rend = IRev<Fwd>::cend(rho);
        bool matchesLeft = matchesPattern(
            reverseIterator(ipoint), reverseIterator(istart),
            reverseIterator(lend), reverseIterator(lstart),
            sca, mc).has_value();
        bool matchesRight = matchesPattern(
            ipend, iend,
            rstart, rend,
            sca, mc).has_value();
        if (matchesLeft && matchesRight)
          return true;
      }
      return false; // none matched
    };
    if (matchesEnv() == envInverted) return std::nullopt;
    return ipend;
  }
  // ------------------------------------------------------------------
  std::optional<size_t> SimpleRule::tryReplaceLTR(
      const SCA& sca, MString& str, size_t start) const {
    MatchCapture mc;
    auto istart = str.begin() + start;
    auto match = matchesRule(
      str.begin(), istart, str.end(),
      alpha.begin(), alpha.end(),
      envs,
      inv,
      sca,
      mc
    );
    if (!match) return std::nullopt;
    auto end = *match;
    assert(end >= istart);
    size_t s = (size_t) (end - istart);
    // Now replace subrange
    MString omegaApp = omega;
    for (MChar& oc : omegaApp)
      oc = applyOmega(sca, std::move(oc), mc);
    replaceSubrange(
      str, istart, end, omegaApp.begin(), omegaApp.end());
    return s;
  }
  std::optional<size_t> SimpleRule::tryReplaceRTL(
      const SCA& sca, MString& str, size_t start) const {
    MatchCapture mc;
    auto istart = str.rbegin() + start;
    auto match = matchesRule(
      str.rbegin(), istart, str.rend(),
      alpha.rbegin(), alpha.rend(),
      envs,
      inv,
      sca,
      mc
    );
    if (!match) return std::nullopt;
    auto end = *match;
    assert(end >= istart);
    size_t s = (size_t) (end - istart);
    // Now replace subrange
    MString omegaApp = omega;
    for (MChar& oc : omegaApp)
      oc = applyOmega(sca, std::move(oc), mc);
    replaceSubrange(
      str, end.base(), istart.base(), omegaApp.begin(), omegaApp.end());
    return s;
  }
  std::optional<size_t> CompoundRule::tryReplaceLTR(
      const SCA& sca, MString& str, size_t start) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplaceLTR(sca, str, start);
      if (res.has_value()) return *res;
    }
    return std::nullopt;
  }
  std::optional<size_t> CompoundRule::tryReplaceRTL(
      const SCA& sca, MString& str, size_t start) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplaceRTL(sca, str, start);
      if (res.has_value()) return *res;
    }
    return std::nullopt;
  }
}
