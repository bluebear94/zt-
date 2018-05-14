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
}
