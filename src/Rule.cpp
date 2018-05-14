#include "Rule.h"

#include "SCA.h"
#include "matching.h"

namespace sca {
  std::optional<size_t> SimpleRule::matchesFromStart(
      const SCA& sca, const MString& str, size_t index) const {
    auto start = str.cbegin() + index;
    auto end = start;
    // Check if text itself matches
    for (const MChar& ac : alpha) {
      if (end == str.cend()) return std::nullopt;
      if (!charsMatch(sca, ac, *end)) return std::nullopt;
      ++end;
    }
    // Check for context
    auto lr = lambda.crbegin();
    auto ll = lambda.crend();
    auto cmp = std::reverse_iterator(start);
    for (auto it = lr; it != ll; ++it) {
      if (cmp == str.crend()) return std::nullopt;
      if (!charsMatch(sca, *it, *cmp)) return std::nullopt;
      ++cmp;
    }
    auto cmp2 = end;
    for (const MChar& rc : rho) {
      if (cmp2 == str.cend()) return std::nullopt;
      if (!charsMatch(sca, rc, *cmp2)) return std::nullopt;
      ++cmp2;
    }
    return end - start;
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
    for (size_t i = 0; i < rho.size() - 1; ++i) {
      if (std::holds_alternative<Space>(rho[i])) {
        errors.push_back(ErrorCode::spacesWrong);
      }
    }
  }
  std::optional<size_t> CompoundRule::matchesFromStart(
      const SCA& sca, const MString& str, size_t index) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.matchesFromStart(sca, str, index);
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
