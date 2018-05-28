#include "matching.h"

#include <assert.h>
#include <stdlib.h>

#include <iostream>
#include <iterator>

#include "Rule.h"
#include "SCA.h"

namespace sca {
#if 0
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
        return MChar(*ps);
      } else {
        return c;
      }
    }, c.value);
  }
#endif
  bool charsMatch(
      const SCA& sca,
      const MChar& fr, const PhonemeSpec& fi,
      MatchCapture& mc) {
    //bool llmatch = fr2.is<std::string>() && fi2.is<std::string>();
    //MChar fr = llmatch ? fr2 : decay(sca, fr2);
    //MChar fi = llmatch ? fi2 : decay(sca, fi2);
    return std::visit([&](const auto& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Space>) {
        return false; // spaces can't occur in words, at least not right now
      } else if constexpr (std::is_same_v<T, std::string>) {
        // Both the phoneme spec and name should be equal;
        if (fi.name != arg) return false;
        const PhonemeSpec* ps;
        Error res = sca.getPhonemeByName(arg, ps);
        if (!res.ok()) {
          // Not found; accept anyway
          // (e. g. if 'x' is not enumerated, then we want 'x' to match 'x')
          return true;
        }
        return arePhonemeSpecsEqual(sca, fi, *ps);
      } else if constexpr (std::is_same_v<T, PhonemeSpec>) {
        return arePhonemeSpecsEqual(sca, fi, arg);
      } else if constexpr (std::is_same_v<T, CharMatcher>) {
        /*const PhonemeSpec* ps;
        bool owned;
        if (fi.is<std::string>()) {
          const std::string ph = fi.as<std::string>();
          Error err = sca.getPhonemeByName(ph, ps);
          if (err != ErrorCode::ok) ps = &defaultPS;
          owned = false;
        } else if (fi.is<PhonemeSpec>()) {
          ps = new PhonemeSpec(std::move(fi.as<PhonemeSpec>()));
          owned = true;
        }*/
        // Get properties of fi
        // Do the classes match (or this one takes any class)?
        if (arg.charClass != -1 && !fi.hasClass(arg.charClass))
          return false;
        // Does this phoneme satisfy our constraints?
        return std::visit([&](const auto& cons) -> bool {
          using U = std::decay_t<decltype(cons)>;
          using Constraint = CharMatcher::Constraint;
          size_t i = -1;
          if constexpr (std::is_same_v<U, std::vector<Constraint>>) {
            // Should match all constraints.
            for (const CharMatcher::Constraint& con : cons) {
              if (!con.matches(fi.getFeatureValue(con.feature, sca), mc, sca))
                return false;
            }
          } else {
            // Should be one of the phonemes enumerated.
            for (i = 0; i < cons.size(); ++i) {
              const PhonemeSpec& ps2 = *(cons[i]);
              if (arePhonemeSpecsEqual(sca, fi, ps2) && fi.name == ps2.name) break;
            }
            if (i == cons.size()) return false;
          }
          auto itres = mc.try_emplace(
            std::pair(arg.charClass, arg.index),
            &fi, false, i);
          if (!itres.second) { // Already there; query current.
            auto it = itres.first;
            if constexpr (std::is_same_v<U, std::vector<Constraint>>) {
              // If we already remember this phoneme, does it match it in every
              // feature we remember, other than the ones we tested?
              const PhonemeSpec* rememberedPS = it->second.ps;
              size_t nFeatures = std::max(
                fi.featureValues.size(),
                rememberedPS->featureValues.size());
              for (size_t i = 0; i < nFeatures; ++i) {
                size_t myval = fi.getFeatureValue(i, sca);
                size_t remval = rememberedPS->getFeatureValue(i, sca);
                  for (const CharMatcher::Constraint& con : cons) {
                    if (con.feature == i) goto ignore;
                  }
                if (myval != remval) return false;
                ignore:;
              }
            } else {
              // If we have a matcher, then does the remembered index
              // correspond?
              size_t rememberedIndex = it->second.index;
              assert(rememberedIndex != -1); // should be caught by validator
              if (rememberedIndex != i) return false;
            }
          }
          return true;
        }, arg.constraints);
      } else {
        std::cerr << "charsMatch: Type is not single character\n";
        abort();
        return false;
      }
    }, fr.value);
  }
  PUnique<const PhonemeSpec> applyOmega(
      const SCA& sca, const MChar& old, const MatchCapture& mc) {
    return std::visit([&](auto&& arg) -> PUnique<const PhonemeSpec> {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, CharMatcher>) {
        auto it = mc.find(std::pair(arg.charClass, arg.index));
        assert(it != mc.end()); // this should have been validated before
        if (arg.hasConstraints()) {
          auto ps = makePOwner<PhonemeSpec>(*(it->second.ps));
          for (const CharMatcher::Constraint& con : arg.getConstraints()) {
            assert(con.c == Comparison::eq);
            assert(con.instances.size() == 1);
            ps->setFeatureValue(con.feature, con.evaluate(0, mc, sca), sca);
          }
          auto phrange = sca.getPhonemesBySpec(*ps);
          if (phrange.first == phrange.second) {
            // Return an anonymous phoneme spec
            return makeConst(std::move(ps));
          }
          // Find the first phoneme that matches the name, or else return the first in
          // the range
          auto it = std::find_if(phrange.first, phrange.second, [&](const auto& p) {
            return p.first.name == ps->name;
          });
          if (it == phrange.second)
            return makePObserver(phrange.first->first);
          else
            return makePObserver(it->first);
        } else {
          size_t index = it->second.index;
          assert(index != -1);
          const auto& name = arg.getEnumeration()[index]->name;
          const PhonemeSpec* ps;
          Error res = sca.getPhonemeByName(name, ps);
          assert(res.ok());
          return makePObserver(*ps);
        }
      } else if constexpr (std::is_same_v<T, std::string>) {
        const PhonemeSpec* ps;
        Error res = sca.getPhonemeByName(arg, ps);
        if (!res.ok()) {
          auto ps2 = makePOwner<PhonemeSpec>();
          ps2->name = std::move(arg);
          return makeConst(std::move(ps2));
        }
        return makePObserver(*ps);
      } else {
        std::cerr << "applyOmega: invalid type for `old`";
        abort();
      }
    }, old.value);
  }
}
