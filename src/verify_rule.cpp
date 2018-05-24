#include "Rule.h"

#include <algorithm>
#include <deque>
#include <unordered_set>

#include "SCA.h"

namespace sca {
  using P = std::pair<size_t, size_t>;
  using MatchSet = std::unordered_set<
      P, PHash<std::size_t, std::size_t>>;
  struct DependentConstraintVerifyContext {
    // Effectively a stack, but `deque` is used to allow iterating through
    // the stack in hasMatcher. By convention, `back` is the top.
    std::deque<MatchSet> seen;
    bool hasLabelledMatchers = false, hasUnlabelledMatchers = false;
    size_t line, col;
    std::unordered_map<
      P, size_t, PHash<size_t, size_t>>
    enumCount;
  };
  static void checkString(
    const MString& st, bool write,
    std::vector<Error>& errors, const SCA& sca,
    DependentConstraintVerifyContext& ctx);
  void SimpleRule::verify(std::vector<Error>& errors, const SCA& sca) const {
    // A rule should not have both unlabelled and labelled matchers.
    // Unlabelled matchers across different categories are fine.
    DependentConstraintVerifyContext ctx;
    ctx.line = line;
    ctx.col = col;
    ctx.seen.emplace_back();
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
  static bool hasMatcher(
      const DependentConstraintVerifyContext& ctx,
      const P& p) {
    // Return true if found.
    return std::any_of(ctx.seen.begin(), ctx.seen.end(), [&p](const auto& s) {
      return s.find(p) != s.end();
    });
  }
  static void verifyEnumCount(
    const CharMatcher& m, bool write,
    std::vector<Error>& errors, const SCA& sca,
    const std::string& asString,
    DependentConstraintVerifyContext& ctx
  ) {
    P p(m.charClass, m.index);
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
      if (!m.hasConstraints()) {
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
  }
  static void checkString(
    const MString& st, bool write,
    std::vector<Error>& errors, const SCA& sca,
    DependentConstraintVerifyContext& ctx
  ) {
    MatchSet& top = ctx.seen.back(); // shouldn't be invalidated
    for (const MChar& ch : st) {
      if (!write && !ch.isSingleCharacter()) {
        errors.push_back(Error(ErrorCode::nonSingleCharInOmega)
          .at(ctx.line, ctx.col));
      }
      std::visit([&](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, CharMatcher>) {
          // ========== CharMatcher ======================================
          std::string asString = arg.toString(sca);
          P p(arg.charClass, arg.index);
          if (write) top.insert(p);
          else { // in omega; reject any not previously defined
            auto it = top.find(p);
            if (it == top.end()) {
              errors.push_back((ErrorCode::undefinedMatcher
                % asString)
                .at(ctx.line, ctx.col));
            }
          }
          // Check constraints
          if (arg.hasConstraints()) {
            for (const CharMatcher::Constraint& con : arg.getConstraints()) {
              std::string cstring = con.toString(sca);
              // Dependent instances previously defined?
              for (const auto& inst : con.instances) {
                if (!std::holds_alternative<P>(inst)) continue;
                if (!hasMatcher(ctx, std::get<P>(inst))) {
                  errors.push_back((ErrorCode::undefinedDependentConstraint
                    % cstring)
                    .at(ctx.line, ctx.col)); // Yikes, that indentation
                }
              }
              // If in omega, follows certain rules?
              // * can't set a non-core feature
              // * can't use a comparison other than `==`
              // * can't set multiple instances of a feature
              //   (though this could be a future extension?)
              if (!write) {
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
              // Uses only = and != for unordered features?
              const Feature& f = sca.getFeatureByID(con.feature);
              if (!f.ordered &&
                  con.c != Comparison::eq && con.c != Comparison::ne) {
                errors.push_back(
                  (ErrorCode::orderedConstraintUnorderedFeature % cstring)
                  .at(ctx.line, ctx.col));
              }
            }
          }
          // Check miscellaneous things:
          // Labelled and unlabelled matchers mixed?
          bool unlabelled = arg.index == 0;
          if (unlabelled ? ctx.hasLabelledMatchers : ctx.hasUnlabelledMatchers)
            errors.push_back(Error(ErrorCode::mixedMatchers)
              .at(ctx.line, ctx.col));
          if (unlabelled) ctx.hasUnlabelledMatchers = true;
          else ctx.hasLabelledMatchers = true;
          // Enum count correct?
          verifyEnumCount(arg, write, errors, sca, asString, ctx);
          // ====== End CharMatcher ======================================
        } else if constexpr (std::is_same_v<T, Alternation>) {
          // ========== Alternation ======================================
          // For each option, temporarily create a new stack entry
          // used for any new matchers seen within that option.
          // Consolidate the matchers found in all options and consider
          // those defined outside the alternation.
          if (arg.options.empty()) return;
          ctx.seen.emplace_back();
          checkString(
            arg.options[0], write, errors, sca, ctx);
          auto common = std::move(ctx.seen.back());
          ctx.seen.pop_back();
          for (size_t i = 1; i < arg.options.size(); ++i) {
            ctx.seen.emplace_back();
            checkString(
              arg.options[i], write, errors, sca, ctx);
            MatchSet intersect;
            for (const auto& p : common) {
              if (ctx.seen.back().find(p) != ctx.seen.back().end())
                intersect.insert(p);
            }
            common = std::move(intersect);
            ctx.seen.pop_back();
          }
          for (const auto& p : common) {
            top.insert(p);
          }
          // ====== End Alternation ======================================
        } else if constexpr (std::is_same_v<T, Repeat>) {
          // ========== Repeat ===========================================
          checkString(arg.s, write, errors, sca, ctx);
          // ====== End Repeat ===========================================
        }
        // Other cases need not be considered.
      }, ch.value);
    }
  }
}
