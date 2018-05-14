#include "Rule.h"

#include <assert.h>

#include <algorithm>

#include "SCA.h"
#include "matching.h"
#include <iostream>
namespace sca {
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
        v1.begin() + oldSize - size2, v1.begin() + oldSize,
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
  std::optional<size_t> SimpleRule::tryReplace(
      const SCA& sca, MString& str, size_t index) const {
    MatchCapture mc;
    auto start = str.begin() + index;
    auto end = start;
    // Check if text itself matches
    for (const MChar& ac : alpha) {
      if (end == str.end()) return std::nullopt;
      if (!charsMatch(sca, ac, *end, mc)) return std::nullopt;
      ++end;
    }
    // Check for context
    auto lr = lambda.rbegin();
    auto ll = lambda.rend();
    auto cmp = std::reverse_iterator(start);
    for (auto it = lr; it != ll; ++it) {
      if (cmp == str.rend()) return std::nullopt;
      if (!charsMatch(sca, *it, *cmp, mc)) return std::nullopt;
      ++cmp;
    }
    auto cmp2 = end;
    for (const MChar& rc : rho) {
      if (cmp2 == str.end()) return std::nullopt;
      if (!charsMatch(sca, rc, *cmp2, mc)) return std::nullopt;
      ++cmp2;
    }
    assert(end >= start);
    size_t s = (size_t) (end - start);
    // Now replace subrange
    MString omegaApp = omega;
    for (MChar& oc : omegaApp)
      oc = applyOmega(sca, std::move(oc), mc);
    replaceSubrange(
      str, start, end, omegaApp.begin(), omegaApp.end());
    return s;
  }
  void SimpleRule::verify(std::vector<Error>& errors) const {
    // A rule should not have both unlabelled and labelled matchers.
    // Unlabelled matchers across different categories are fine.
    bool hasLabelledMatchers = false;
    bool hasUnlabelledMatchers = false;
    auto checkString = [&](const MString& st) {
      for (const MChar& c : st) {
        if (std::holds_alternative<CharMatcher>(c)) {
          const CharMatcher& m = std::get<CharMatcher>(c);
          bool unlabelled = m.index == 0;
          if (unlabelled ? hasLabelledMatchers : hasUnlabelledMatchers)
            errors.push_back(ErrorCode::mixedMatchers);
          (unlabelled ? hasUnlabelledMatchers : hasLabelledMatchers) = true;
        }
      }
    };
    checkString(alpha);
    checkString(omega);
    checkString(lambda);
    checkString(rho);
    for (size_t i = 1; i < lambda.size(); ++i) {
      if (std::holds_alternative<Space>(lambda[i])) {
        errors.push_back(ErrorCode::spacesWrong);
      }
    }
    if (rho.size() > 0) {
      for (size_t i = 0; i < rho.size() - 1; ++i) {
        if (std::holds_alternative<Space>(rho[i])) {
          errors.push_back(ErrorCode::spacesWrong);
        }
      }
    }
  }
  std::optional<size_t> CompoundRule::tryReplace(
      const SCA& sca, MString& str, size_t index) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplace(sca, str, index);
      if (res.has_value()) return res;
    }
    return std::nullopt;
  }
  void CompoundRule::verify(std::vector<Error>& errors) const {
    for (const SimpleRule& s : components) {
      s.verify(errors);
    }
  }
}
