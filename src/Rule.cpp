#include "Rule.h"

#include <assert.h>

#include <algorithm>
#include <unordered_set>

#include "SCA.h"
#include "matching.h"

namespace sca {
  size_t PhonemeSpec::getFeatureValue(size_t f, const SCA& sca) const {
    return (f < featureValues.size()) ?
      featureValues[f] :
      sca.getFeatureByID(f).def;
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
  template<typename It>
  struct IRev {
    static auto get(It it) {
      return std::reverse_iterator<It>(it);
    }
  };
  template<typename It>
  struct IRev<std::reverse_iterator<It>> {
    static auto get(std::reverse_iterator<It> it) {
      return it.base();
    }
  };
  template<typename It>
  auto reverseIterator(It it) {
    return IRev<It>::get(it);
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
      const auto& inText = *iit;
      if (!charsMatch(sca, inRule, inText, mc)) return std::nullopt;
      ++iit;
      ++rit;
    }
  }
  template<typename Fwd, typename CFwd>
  static std::optional<Fwd> matchesRule(
    Fwd istart, Fwd ipoint, Fwd iend, // text start / search start / text end
    CFwd astart, CFwd aend, // alpha
    CFwd lstart, CFwd lend, // lambda
    CFwd rstart, CFwd rend, // rho
    bool envInverted, // Match if environment is NOT matched (vs matched)?
    const SCA& sca,
    MatchCapture& mc
  ) {
    auto amatch = matchesPattern(ipoint, iend, astart, aend, sca, mc);
    if (!amatch) return std::nullopt;
    Fwd ipend = *amatch;
    auto matchesEnv = [=, &sca, &mc]() -> bool {
      if (!matchesPattern(
          reverseIterator(ipoint), reverseIterator(istart),
          reverseIterator(lend), reverseIterator(lstart),
          sca, mc))
        return false;
      return matchesPattern(ipend, iend, rstart, rend, sca, mc).has_value();
    };
    if (matchesEnv() == envInverted) return std::nullopt;
    return ipend;
  }
  // ------------------------------------------------------------------
  std::optional<MSI> SimpleRule::tryReplaceLTR(
      const SCA& sca, MString& str, MSI start) const {
    MatchCapture mc;
    auto match = matchesRule(
      str.begin(), start, str.end(),
      alpha.begin(), alpha.end(),
      lambda.begin(), lambda.end(),
      rho.begin(), rho.end(),
      inv,
      sca,
      mc
    );
    if (!match) return std::nullopt;
    auto end = *match;
    assert(end >= start);
    // Now replace subrange
    MString omegaApp = omega;
    for (MChar& oc : omegaApp)
      oc = applyOmega(sca, std::move(oc), mc);
    replaceSubrange(
      str, start, end, omegaApp.begin(), omegaApp.end());
    return end;
  }
  std::optional<MSRI> SimpleRule::tryReplaceRTL(
      const SCA& sca, MString& str, MSRI start) const {
    MatchCapture mc;
    auto match = matchesRule(
      str.rbegin(), start, str.rend(),
      alpha.rbegin(), alpha.rend(),
      rho.rbegin(), rho.rend(),
      lambda.rbegin(), lambda.rend(),
      inv,
      sca,
      mc
    );
    if (!match) return std::nullopt;
    auto end = *match;
    assert(end >= start);
    // Now replace subrange
    MString omegaApp = omega;
    for (MChar& oc : omegaApp)
      oc = applyOmega(sca, std::move(oc), mc);
    replaceSubrange(
      str, end.base(), start.base(), omegaApp.begin(), omegaApp.end());
    return end;
  }
  void SimpleRule::verify(std::vector<Error>& errors, const SCA& sca) const {
    // A rule should not have both unlabelled and labelled matchers.
    // Unlabelled matchers across different categories are fine.
    bool hasLabelledMatchers = false;
    bool hasUnlabelledMatchers = false;
    std::unordered_set<std::pair<size_t, size_t>, PHash<size_t, size_t>>
    defined; // Set of defined symbols
    auto checkString = [&](const MString& st, bool write) {
      for (const MChar& c : st) {
        if (c.is<CharMatcher>()) {
          const CharMatcher& m = c.as<CharMatcher>();
          bool unlabelled = m.index == 0;
          if (unlabelled ? hasLabelledMatchers : hasUnlabelledMatchers)
            errors.push_back(Error(ErrorCode::mixedMatchers).at(line, col));
          (unlabelled ? hasUnlabelledMatchers : hasLabelledMatchers) = true;
          if (write) {
            defined.insert(std::pair(m.charClass, m.index));
          } else {
            // Reject if not already defined...
            if (defined.count(std::pair(m.charClass, m.index)) == 0) {
              errors.push_back((ErrorCode::undefinedMatcher % (
                sca.getClassByID(m.charClass).name + ":" +
                std::to_string(m.index)
              )).at(line, col));
            }
            // ... or tries to set a non-core feature
            for (const CharMatcher::Constraint& con : m.constraints) {
              const auto& f = sca.getFeatureByID(con.feature);
              if (!f.isCore) {
                errors.push_back((ErrorCode::nonCoreFeatureSet % (
                  f.featureName + "=" + f.instanceNames[con.instance]))
                  .at(line, col));
              }
            }
          }
        }
      }
    };
    checkString(alpha, true);
    checkString(rho, true);
    checkString(lambda, true);
    checkString(omega, false);
    for (size_t i = 1; i < lambda.size(); ++i) {
      if (lambda[i].is<Space>()) {
        errors.push_back(Error(ErrorCode::spacesWrong).at(line, col));
      }
    }
    if (rho.size() > 0) {
      for (size_t i = 0; i < rho.size() - 1; ++i) {
        if (rho[i].is<Space>()) {
          errors.push_back(Error(ErrorCode::spacesWrong).at(line, col));
        }
      }
    }
  }
  std::optional<MSI> CompoundRule::tryReplaceLTR(
      const SCA& sca, MString& str, MSI start) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplaceLTR(sca, str, start);
      if (res.has_value()) return *res;
    }
    return std::nullopt;
  }
  std::optional<MSRI> CompoundRule::tryReplaceRTL(
      const SCA& sca, MString& str, MSRI start) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplaceRTL(sca, str, start);
      if (res.has_value()) return *res;
    }
    return std::nullopt;
  }
  void CompoundRule::verify(std::vector<Error>& errors, const SCA& sca) const {
    for (const SimpleRule& s : components) {
      s.verify(errors, sca);
    }
  }
}
