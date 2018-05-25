#include "PHash.h"

#include <functional>
#include <utility>

#include "Rule.h"
#include "SCA.h"

namespace sca {
  size_t PSHash::operator()(const PhonemeSpec& ps) const {
    size_t x = std::hash<size_t>()(ps.charClass);
    constexpr size_t bits = CHAR_BIT * sizeof(size_t);
    int rot = 0;
    for (size_t i = 0; i < ps.featureValues.size(); ++i) {
      size_t fv = ps.featureValues[i];
      fv -= sca->getFeatureByID(i).def;
      // use raw to avoid issues on 0
      if (sca->getFeatureByID(i).isCore)
        x ^= (fv << rot) | (fv >> (bits - rot));
      rot = (rot + 1) % bits;
    }
    return x;
  }
  bool arePhonemeSpecsEqual(
      const SCA& sca, const PhonemeSpec& a, const PhonemeSpec& b) {
    if (a.charClass != b.charClass) return false;
    size_t nf = std::max(a.featureValues.size(), b.featureValues.size());
    for (size_t i = 0; i < nf; ++i) {
      if (!sca.getFeatureByID(i).isCore) continue;
      if (a.getFeatureValue(i, sca) != b.getFeatureValue(i, sca))
        return false;
    }
    return true;
  }
}
