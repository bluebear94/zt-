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
  static MChar decay(const SCA& sca, const MChar& c) {
    return std::visit([&](const auto& arg) -> MChar {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, std::string>) {
        const PhonemeSpec* ps;
        Error err = sca.getPhonemeByName(arg, ps);
        if (err != ErrorCode::ok) return c;
        return *ps;
      } else {
        return c;
      }
    }, c.value);
  }
  bool charsMatch(
      const SCA& sca, const MChar& fr2, const MChar& fi2, MatchCapture& mc) {
    bool llmatch = fr2.is<std::string>() && fi2.is<std::string>();
    MChar fr = llmatch ? fr2 : decay(sca, fr2);
    MChar fi = llmatch ? fi2 : decay(sca, fi2);
    return std::visit([&](const auto& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Space>) {
        return fi.is<Space>();
      } else if constexpr (std::is_same_v<T, std::string>) {
        return fi.isT(arg);
      } else if constexpr (std::is_same_v<T, PhonemeSpec>) {
        return fi.is<PhonemeSpec>() &&
          arePhonemeSpecsEqual(sca, fi.as<PhonemeSpec>(), arg);
      } else if constexpr (std::is_same_v<T, CharMatcher>) {
        const PhonemeSpec* ps;
        bool owned;
        if (fi.is<std::string>()) {
          const std::string ph = fi.as<std::string>();
          Error err = sca.getPhonemeByName(ph, ps);
          if (err != ErrorCode::ok) ps = &defaultPS;
          owned = false;
        } else if (fi.is<PhonemeSpec>()) {
          ps = new PhonemeSpec(std::move(fi.as<PhonemeSpec>()));
          owned = true;
        }
        // Get properties of fi
        // Do the classes match (or this one takes any class)?
        if (arg.charClass != -1 && !ps->hasClass(arg.charClass))
          return false;
        // Does this phoneme satisfy our constraints?
        for (const CharMatcher::Constraint& con : arg.constraints) {
          if (!con.matches(ps->getFeatureValue(con.feature)))
            return false;
        }
        auto itres = mc.try_emplace(
          std::pair(arg.charClass, arg.index),
          ps, owned);
        if (!itres.second) { // Already there; query current.
          auto it = itres.first;
          // If we already remember this phoneme, does it match it in every
          // feature we remember, other than the ones we tested?
          const PhonemeSpec* rememberedPS = it->second.ps;
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
    }, fr.value);
  }
  MChar applyOmega(const SCA& sca, MChar&& old, const MatchCapture& mc) {
    return std::visit([&](auto&& arg) -> MChar {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, CharMatcher>) {
        auto it = mc.find(std::pair(arg.charClass, arg.index));
        assert(it != mc.end()); // this should have been validated before
        PhonemeSpec ps(*(it->second.ps));
        for (const CharMatcher::Constraint& con : arg.constraints) {
          ps.setFeatureValue(con.feature, con.instance);
        }
        auto phrange = sca.getPhonemesBySpec(ps);
        if (phrange.first == phrange.second) {
          // Return an anonymous phoneme spec
          return ps;
        }
        return phrange.first->second;
      } else {
        return std::move(arg);
      }
    }, old.value);
  }
}
