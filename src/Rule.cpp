#include "Rule.h"

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <iostream>

#include "SCA.h"
#include "iterutils.h"
#include "matching.h"
#include "sca_lua.h"

/*
  For the implementation of the SimpleRule::verify and CompoundRule::verify
  methods, see verify_rule.cpp.
*/

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
    if (charClass == -1)
      return "*:" + std::to_string(index);
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
  template<typename CFwd, typename WFwd>
  static std::optional<WFwd> matchesMChar(
    WFwd istart, WFwd iend,
    const MChar& ruleChar,
    const SCA& sca,
    MatchCapture& mc
  ) {
    if (ruleChar.isSingleCharacter()) {
      if (istart == iend) return std::nullopt;
      bool match = charsMatch(sca, ruleChar, **istart, mc);
      if (match) return istart + 1;
      else return std::nullopt;
    }
    // Does not necessarily match one and only one.
    return std::visit([&](const auto& arg) -> std::optional<WFwd> {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Alternation>) {
        // Try each option in succession and pick the first one that works
        for (const MString& opt : arg.options) {
          // Back up the MatchCapture, in case this option fails
          MatchCapture mcback(mc);
          auto it = matchesPattern(
            istart, iend,
            IRev<CFwd>::cbegin(opt), IRev<CFwd>::cend(opt),
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
        WFwd it = istart;
        while (true) {
          if (it >= iend) break; // Passed the end; can't match anymore
          if (nCopies > arg.max) break; // Can't match more copies
          auto matchEnd = matchesPattern(
            it, iend,
            IRev<CFwd>::cbegin(arg.s), IRev<CFwd>::cend(arg.s),
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
  template<typename CFwd, typename WFwd
  > // templated to handle both fwd and rev cases
  static std::optional<WFwd> matchesPattern(
    WFwd istart, // where to start looking
    WFwd iend, // end of string (stop looking when you reach here)
    CFwd rstart, // iterator to start of rule
    CFwd rend, // iterator to end of rule
    const SCA& sca,
    MatchCapture& mc
  ) {
    WFwd iit = istart;
    CFwd rit = rstart;
    while (true) {
      if (rit == rend) // all chars matched
        return iit;
      if (iit == iend) { // end of string but unmatched chars
        if (rit->template is<Space>()) return iit;
        return std::nullopt;
      }
      const auto& inRule = *rit;
      auto matchEnd = matchesMChar<CFwd, WFwd>(iit, iend, inRule, sca, mc);
      if (!matchEnd.has_value()) return std::nullopt;
      ++rit;
      iit = *matchEnd;
    }
  }
  template<typename Fwd, typename CFwd, typename WFwd>
  static std::optional<WFwd> matchesRule(
    // v text start / search start / text end
    WFwd istart, WFwd ipoint, WFwd iend,
    CFwd astart, CFwd aend, // alpha
    const std::vector<std::pair<MString, MString>>& envs, // envs
    bool envInverted, // Match if environment is NOT matched (vs matched)?
    const SCA& sca,
    MatchCapture& mc
  ) {
    auto amatch = matchesPattern(ipoint, iend, astart, aend, sca, mc);
    if (!amatch) return std::nullopt;
    WFwd ipend = *amatch;
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
      const SCA& sca, WString& str, size_t start) const {
    MatchCapture mc;
    auto istart = str.begin() + start;
    auto match = matchesRule<MSI, MSCI, WString::iterator>(
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
    bool gammaMatches = evaluate(sca.getLuaState(), str, start, start + s);
    if (!gammaMatches) return std::nullopt;
    // Now replace subrange
    WString omegaApp;
    for (const MChar& oc : omega)
      omegaApp.push_back(applyOmega(sca, oc, mc));
    replaceSubrange(
      str, istart, end, omegaApp.begin(), omegaApp.end());
    return s;
  }
  std::optional<size_t> SimpleRule::tryReplaceRTL(
      const SCA& sca, WString& str, size_t start) const {
    MatchCapture mc;
    auto istart = str.rbegin() + start;
    auto match = matchesRule<MSRI, MSRCI, WString::reverse_iterator>(
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
    size_t eifwd = str.size() - 1 - start;
    bool gammaMatches = evaluate(
      sca.getLuaState(), str,
      eifwd - s,
      eifwd);
    if (!gammaMatches) return std::nullopt;
    // Now replace subrange
    WString omegaApp;
    for (const MChar& oc : omega)
      omegaApp.push_back(applyOmega(sca, oc, mc));
    replaceSubrange(
      str, end.base(), istart.base(), omegaApp.begin(), omegaApp.end());
    return s;
  }
  std::optional<size_t> CompoundRule::tryReplaceLTR(
      const SCA& sca, WString& str, size_t start) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplaceLTR(sca, str, start);
      if (res.has_value()) return *res;
    }
    return std::nullopt;
  }
  std::optional<size_t> CompoundRule::tryReplaceRTL(
      const SCA& sca, WString& str, size_t start) const {
    for (const SimpleRule& sr : components) {
      auto res = sr.tryReplaceRTL(sca, str, start);
      if (res.has_value()) return *res;
    }
    return std::nullopt;
  }
  bool SimpleRule::setGamma(lua_State* luaState, const std::string_view& s) {
    char* buffer = new char[s.length() + 7];
    memcpy(buffer, "return ", 7);
    memcpy(buffer + 7, s.data(), s.length());
    int stat = luaL_loadbuffer(luaState, buffer, s.length() + 7, "<Γ>");
    delete[] buffer;
    if (stat != LUA_OK) return false;
    gammaref = luaL_ref(luaState, LUA_REGISTRYINDEX);
    return true;
  }
  bool SimpleRule::evaluate(lua_State* luaState,
      const WString& word, size_t mstart, size_t mend) const {
    if (gammaref == LUA_NOREF) return true;
    // Create M
    lua_newtable(luaState);
    lua_pushinteger(luaState, mstart + 1);
    lua_setfield(luaState, -2, "s");
    lua_pushinteger(luaState, mend + 1);
    lua_setfield(luaState, -2, "e");
    lua_pushinteger(luaState, mend - mstart);
    lua_setfield(luaState, -2, "n");
    lua_setglobal(luaState, "M");
    // Create W
    lua_newtable(luaState);
    for (size_t i = 0; i < word.size(); ++i) {
      // pray that no one modifies the word
      sca::lua::pushPhonemeSpec(luaState, (PhonemeSpec&) *(word[i]));
      lua_seti(luaState, -2, i + 1);
    }
    lua_setglobal(luaState, "W");
    lua_geti(luaState, LUA_REGISTRYINDEX, gammaref);
    int stat = lua_pcall(luaState, 0, 1, 0);
    if (stat != LUA_OK) {
      std::cerr << "Fatal error when evaluating a Γ:\n";
      std::cerr << lua_tostring(luaState, -1) << "\n";
      abort();
    }
    return lua_toboolean(luaState, -1);
  }
}
