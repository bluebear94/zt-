#include "matching.h"

#include <assert.h>

#include <iterator>

#include "SCA.h"

namespace sca {
  static const PhonemeSpec defaultPS = {
    /* .name = */ "",
    /* .charClass = */ (size_t) -1,
    /* .featureValues = */ std::vector<size_t>(),
  };
  bool charsMatch(const SCA& sca, const MChar& fr, const MChar& fi) {
    return std::visit([&](const auto& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Space>) {
        return std::holds_alternative<Space>(fi);
      } else if constexpr (std::is_same_v<T, std::string>) {
        return std::holds_alternative<std::string>(fi) &&
          std::get<std::string>(fi) == arg;
      } else if constexpr (std::is_same_v<T, CharMatcher>) {
        if (!std::holds_alternative<std::string>(fi)) return false;
        const std::string& ph = std::get<std::string>(fi);
        // Get properties of fi
        const PhonemeSpec* ps;
        Error err = sca.getPhonemeByName(ph, ps);
        if (err != ErrorCode::ok) ps = &defaultPS;
        // Do the classes match (or this one takes any class)?
        if (arg.charClass != -1 && !ps->hasClass(arg.charClass))
          return false;
        // Does this phoneme satisfy our constraints?
        for (const CharMatcher::Constraint& con : arg.constraints) {
          if (!con.matches(ps->getFeatureValue(con.feature)))
            return false;
        }
        return true;
      }
    }, fr);
  }

  std::optional<size_t> matchesFromStart(
      const SCA& sca,
      const SimpleRule& rule, const MString& str, size_t index) {
    auto start = str.cbegin() + index;
    auto end = start;
    // Check if text itself matches
    for (const MChar& ac : rule.alpha) {
      if (end == str.cend()) return std::nullopt;
      if (!charsMatch(sca, ac, *end)) return std::nullopt;
      ++end;
    }
    // Check for context
    auto lr = rule.lambda.crbegin();
    auto ll = rule.lambda.crend();
    auto cmp = std::reverse_iterator(start);
    for (auto it = lr; it != ll; ++it) {
      if (cmp == str.crend()) return std::nullopt;
      if (!charsMatch(sca, *it, *cmp)) return std::nullopt;
      ++cmp;
    }
    auto cmp2 = end;
    for (const MChar& rc : rule.rho) {
      if (cmp2 == str.cend()) return std::nullopt;
      if (!charsMatch(sca, rc, *cmp2)) return std::nullopt;
      ++cmp2;
    }
    return end - start;
  }
  std::optional<size_t> matchesFromStart(
      const SCA& sca,
      const CompoundRule& rule, const MString& str, size_t index) {
    for (const SimpleRule& sr : rule.components) {
      auto res = matchesFromStart(sca, sr, str, index);
      if (res.has_value()) return res;
    }
    return std::nullopt;
  }
  std::optional<size_t> matchesFromStart(
      const SCA& sca,
      const Rule& rule, const MString& str, size_t index) {
    return std::visit([=](const auto& arg) {
      return matchesFromStart(sca, arg, str, index);
    }, rule);
  }
}
