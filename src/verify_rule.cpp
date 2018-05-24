#include "Rule.h"

#include <unordered_set>

#include "SCA.h"

namespace sca {
  using MatchSet = std::unordered_set<
      std::pair<std::size_t, std::size_t>,
      PHash<std::size_t, std::size_t>>;
  static MatchSet getAllMatchers(const MString& s);
  static MatchSet getCommonMatchers(const Alternation& alt) {
    if (alt.options.empty()) return MatchSet(); // nothing in common
    MatchSet common = getAllMatchers(alt.options[0]);
    for (size_t i = 1; i < alt.options.size(); ++i) {
      auto there = getAllMatchers(alt.options[i]);
      MatchSet intersect;
      for (const auto& p : common) {
        if (there.find(p) != there.end()) intersect.insert(p);
      }
      common = std::move(intersect);
    }
    return common;
  }
  static MatchSet getAllMatchers(const MString& s) {
    MatchSet common;
    for (const MChar& ch : s) {
      std::visit([&](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, CharMatcher>) {
          common.insert(std::pair(arg.charClass, arg.index));
        } else if constexpr (std::is_same_v<T, Alternation>) {
          auto there = getCommonMatchers(arg);
          for (const auto& p : there) common.insert(p);
        } else if constexpr (std::is_same_v<T, Repeat>) {
          auto there = getAllMatchers(arg.s);
          for (const auto& p : there) common.insert(p);
        }
      }, ch.value);
    }
    return common;
  }
  struct SimpleRuleVerifyContext {
    SimpleRuleVerifyContext() :
      hasLabelledMatchers(false), hasUnlabelledMatchers(false) {}
    size_t line, col;
    MatchSet defined;
    bool hasLabelledMatchers, hasUnlabelledMatchers;
    std::unordered_map<
      std::pair<size_t, size_t>, size_t, PHash<size_t, size_t>>
    enumCount;
  };
  static void checkString(
    const MString& st, bool write,
    std::vector<Error>& errors, const SCA& sca,
    SimpleRuleVerifyContext& ctx
  ) {
    if (write) {
      auto used = getAllMatchers(st);
      // Record defined symbols
      for (const auto& p : used) ctx.defined.insert(p);
    }
    for (const MChar& c : st) {
      if (!write && !c.isSingleCharacter()) {
        errors.push_back(Error(ErrorCode::nonSingleCharInOmega)
          .at(ctx.line, ctx.col));
      }
      if (!c.is<CharMatcher>()) continue;
      const CharMatcher& m = c.as<CharMatcher>();
      std::string asString = m.toString(sca);
      auto p = std::pair(m.charClass, m.index);
      bool unlabelled = m.index == 0;
      if (unlabelled ? ctx.hasLabelledMatchers : ctx.hasUnlabelledMatchers)
        errors.push_back(Error(ErrorCode::mixedMatchers)
          .at(ctx.line, ctx.col));
      if (unlabelled) ctx.hasUnlabelledMatchers = true;
      else ctx.hasLabelledMatchers = true;
      auto verifyEnumCount = [&](auto iter) {
        size_t count = iter->second;
        if (count == -1) {
          errors.push_back((ErrorCode::enumToNonEnum
            % asString)
            .at(ctx.line, ctx.col));
        } else if (count != m.getEnumeration().size()) {
          errors.push_back((ErrorCode::enumCharCountMismatch
            % asString)
            .at(ctx.line, ctx.col));
        }
      };
      if (write) {
        if (m.hasConstraints()) {
          ctx.enumCount.try_emplace(
            p, -1);
          // If already there, no need to check
        } else {
          auto res = ctx.enumCount.try_emplace(
            p, m.getEnumeration().size());
          // If already there, then check
          if (!res.second) {
            verifyEnumCount(res.first);
          }
        }
      } else {
        // Reject if not already defined...
        if (ctx.defined.count(p) == 0) {
          errors.push_back((ErrorCode::undefinedMatcher
            % asString)
            .at(ctx.line, ctx.col));
        }
        // ... or tries to set a non-core feature,
        // or tries to use a comparison other than `==`
        // or tries to set multiple instances of a feature
        //   (though this could be a future extension?)
        if (m.hasConstraints()) {
          for (const CharMatcher::Constraint& con : m.getConstraints()) {
            std::string cstring = con.toString(sca);
            const auto& f = sca.getFeatureByID(con.feature);
            if (!f.isCore) {
              errors.push_back((ErrorCode::nonCoreFeatureSet
                % cstring)
                .at(ctx.line, ctx.col));
            }
            if (con.c != Comparison::eq) {
              errors.push_back((ErrorCode::invalidOperatorOmega
                % cstring)
                .at(ctx.line, ctx.col));
            }
            if (con.instances.size() != 1) {
              errors.push_back((ErrorCode::multipleInstancesOmega
                % cstring)
                .at(ctx.line, ctx.col));
            }
          }
        } else {
          // ... or mismatches enum char count of a previous matcher
          auto it = ctx.enumCount.find(p);
          if (it == ctx.enumCount.end()) {
          errors.push_back((ErrorCode::undefinedMatcher
            % asString)
            .at(ctx.line, ctx.col));
          }
          verifyEnumCount(it);
        }
      }
      if (m.hasConstraints()) {
        for (const CharMatcher::Constraint& con : m.getConstraints()) {
          std::string cstring = con.toString(sca);
          const Feature& f = sca.getFeatureByID(con.feature);
          if (!f.ordered &&
              con.c != Comparison::eq && con.c != Comparison::ne) {
            errors.push_back(
              (ErrorCode::orderedConstraintUnorderedFeature % cstring)
              .at(ctx.line, ctx.col));
          }
        }
      }
    }
  }
  void SimpleRule::verify(std::vector<Error>& errors, const SCA& sca) const {
    // A rule should not have both unlabelled and labelled matchers.
    // Unlabelled matchers across different categories are fine.
    SimpleRuleVerifyContext ctx;
    ctx.line = line;
    ctx.col = col;
    checkString(alpha, true, errors, sca, ctx);
    for (const auto& p : envs) {
      const auto& lambda = p.first;
      const auto& rho = p.second;
      checkString(lambda, true, errors, sca, ctx);
      checkString(rho, true, errors, sca, ctx);
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
    checkString(omega, false, errors, sca, ctx);
  }
  void CompoundRule::verify(std::vector<Error>& errors, const SCA& sca) const {
    for (const SimpleRule& s : components) {
      s.verify(errors, sca);
    }
  }
}