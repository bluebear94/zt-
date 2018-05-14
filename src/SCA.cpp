#include "SCA.h"

#include <algorithm>

namespace sca {
  // I hope features with lots of instances aren't that common.
  ErrorCode Feature::getFeatureInstanceByName(
      const std::string& name, size_t& id) {
    auto it = std::find(instanceNames.begin(), instanceNames.end(), name);
    if (it == instanceNames.end()) return ErrorCode::noSuchFeatureInstance;
    id = it - instanceNames.begin();
    return ErrorCode::ok;
  }
  ErrorCode SCA::insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature) {
    size_t oldFeatureCount = features.size();
    auto res = featuresByName.insert(
      std::pair{f.featureName, oldFeatureCount});
    if (!res.second) return ErrorCode::featureExists;
    features.push_back(std::move(f));
    for (size_t ii = 0; ii < phonemesByFeature.size(); ++ii) {
      const std::vector<std::string>& phonemesInInstance =
        phonemesByFeature[ii];
      for (const std::string& phoneme : phonemesInInstance) {
        PhonemeSpec& spec = phonemes[phoneme];
        spec.name = phoneme;
        spec.featureValues.resize(oldFeatureCount + 1);
        spec.featureValues[oldFeatureCount] = ii;
      }
    }
    for (auto& p : phonemes) {
      p.second.featureValues.resize(oldFeatureCount + 1);
    }
    return ErrorCode::ok;
  }
  ErrorCode SCA::getFeatureByName(
      const std::string& name, size_t& id, Feature*& feature) {
    auto it = featuresByName.find(name);
    if (it == featuresByName.end()) return ErrorCode::noSuchFeature;
    id = it->second;
    feature = &(features[it->second]);
    return ErrorCode::ok;
  }
  ErrorCode SCA::getClassByName(
      const std::string& name, size_t& id, CharClass*& cclass) {
    auto it = classesByName.find(name);
    if (it == classesByName.end()) return ErrorCode::noSuchClass;
    id = it->second;
    cclass = &(charClasses[it->second]);
    return ErrorCode::ok;
  }
  ErrorCode SCA::insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes) {
    size_t oldClassCount = charClasses.size();
    auto res = classesByName.insert(
      std::pair{name, oldClassCount});
    if (!res.second) return ErrorCode::classExists;
    for (const std::string& phoneme : myPhonemes) {
      PhonemeSpec& spec = phonemes[phoneme];
      if (spec.charClass == -1) return ErrorCode::phonemeAlreadyHasClass;
    }
    charClasses.emplace_back();
    CharClass& newClass = charClasses.back();
    newClass.name = std::move(name);
    for (const std::string& phoneme : myPhonemes) {
      PhonemeSpec& spec = phonemes[phoneme];
      spec.charClass = oldClassCount;
    }
    return ErrorCode::ok;
  }
}
