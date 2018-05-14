#include "matching.h"

#include <assert.h>

#include <iterator>

#include "Rule.h"
#include "SCA.h"

namespace sca {
  static const PhonemeSpec defaultPS = {
    /* .name = */ "",
    /* .charClass = */ (size_t) -1,
    /* .featureValues = */ std::vector<size_t>(),
  };
  bool charsMatch(
      const SCA& sca, const MChar& fr, const MChar& fi, MatchCapture& mc) {
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
        auto it = mc.find(std::pair(arg.charClass, arg.index));
        if (it == mc.end()) {
          // Remember for later use
          mc[std::pair(arg.charClass, arg.index)] = ps;
        } else {
          // If we already remember this phoneme, does it match it in every
          // feature we remember, other than the ones we tested?
          const PhonemeSpec* rememberedPS = it->second;
          size_t nFeatures = std::max(
            ps->featureValues.size(),
            rememberedPS->featureValues.size());
          for (size_t i = 0; i < nFeatures; ++i) {
            size_t myval = ps->getFeatureValue(i);
            size_t remval = rememberedPS->getFeatureValue(i);
            for (const CharMatcher::Constraint& con : arg.constraints) {
              if (con.feature == i) goto ignore;
            }
            if (myval != remval) return false;
            ignore:;
          }
        }
        return true;
      }
    }, fr);
  }
  MChar applyOmega(const SCA& sca, MChar&& old, const MatchCapture& mc) {
    return std::visit([&](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Space>) {
        return MChar(std::move(arg));
      } else if constexpr (std::is_same_v<T, std::string>) {
        return MChar(std::move(arg));
      } else if constexpr (std::is_same_v<T, CharMatcher>) {
        auto it = mc.find(std::pair(arg.charClass, arg.index));
        assert(it != mc.end()); // this should have been validated before
        PhonemeSpec ps(*(it->second));
        for (const CharMatcher::Constraint& con : arg.constraints) {
          ps.setFeatureValue(con.feature, con.instance);
        }
        auto phrange = sca.getPhonemesBySpec(ps);
        if (phrange.first == phrange.second) abort();
        return MChar(phrange.first->second);
      }
    }, old);
  }
}
