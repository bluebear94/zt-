#include "SCA.h"

#include <algorithm>

namespace sca {
  // I hope features with lots of instances aren't that common.
  Error Feature::getFeatureInstanceByName(
      const std::string& name, size_t& id) const {
    auto it = std::find(instanceNames.begin(), instanceNames.end(), name);
    if (it == instanceNames.end())
      return ErrorCode::noSuchFeatureInstance % name;
    id = it - instanceNames.begin();
    return ErrorCode::ok;
  }
  Error SCA::insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature) {
    size_t oldFeatureCount = features.size();
    auto res = featuresByName.insert(
      std::pair{f.featureName, oldFeatureCount});
    if (!res.second) return ErrorCode::featureExists % f.featureName;
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
  Error SCA::insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes) {
    size_t oldClassCount = charClasses.size();
    auto res = classesByName.insert(
      std::pair{name, oldClassCount});
    if (!res.second) return ErrorCode::classExists % name;
    for (const std::string& phoneme : myPhonemes) {
      PhonemeSpec& spec = phonemes[phoneme];
      if (spec.charClass != -1)
        return ErrorCode::phonemeAlreadyHasClass %
          (phoneme + " is in " + charClasses[spec.charClass].name +
            "; tried to insert it in " + name);
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
  Error SCA::getFeatureByName(
      const std::string& name, size_t& id, Feature const*& feature) const {
    auto it = featuresByName.find(name);
    if (it == featuresByName.end())
      return ErrorCode::noSuchFeature % name;
    id = it->second;
    feature = &(features[it->second]);
    return ErrorCode::ok;
  }
  Error SCA::getClassByName(
      const std::string& name, size_t& id, CharClass const*& cclass) const {
    auto it = classesByName.find(name);
    if (it == classesByName.end())
      return ErrorCode::noSuchClass % name;
    id = it->second;
    cclass = &(charClasses[it->second]);
    return ErrorCode::ok;
  }
  Error SCA::getPhonemeByName(
      const std::string& name, PhonemeSpec const*& ps) const {
    auto it = phonemes.find(name);
    if (it == phonemes.end())
      return ErrorCode::noSuchPhoneme % name;
    ps = &(it->second);
    return ErrorCode::ok;
  }
  Error SCA::getFeatureByName(
      const std::string& name, size_t& id, Feature*& feature) {
    return getFeatureByName(name, id, (const Feature*&) feature);
  }
  Error SCA::getClassByName(
      const std::string& name, size_t& id, CharClass*& cclass) {
    return getClassByName(name, id, (const CharClass*&) cclass);
  }
  Error SCA::getPhonemeByName(
      const std::string& name, PhonemeSpec*& ps) {
    return getPhonemeByName(name, (const PhonemeSpec*&) ps);
  }
  static void verifyRule(const SimpleRule& s, std::vector<Error>& errors) {
    // A rule should not have both unlabelled and labelled matchers.
    // Unlabelled matchers across different categories are fine.
    bool hasLabelledMatchers = false;
    bool hasUnlabelledMatchers = false;
    auto checkString = [&](const MString& st) {
      for (const MChar& c : st) {
        if (std::holds_alternative<CharMatcher>(c)) {
          const CharMatcher& m = std::get<CharMatcher>(c);
          bool unlabelled = m.index == 0;
          if (unlabelled ? hasLabelledMatchers : hasUnlabelledMatchers)
            errors.push_back(ErrorCode::mixedMatchers);
          (unlabelled ? hasUnlabelledMatchers : hasLabelledMatchers) = true;
        }
      }
    };
    checkString(s.alpha);
    checkString(s.omega);
    checkString(s.lambda);
    checkString(s.rho);
    for (size_t i = 1; i < s.lambda.size(); ++i) {
      if (std::holds_alternative<Space>(s.lambda[i])) {
        errors.push_back(ErrorCode::spacesWrong);
      }
    }
    for (size_t i = 0; i < s.rho.size() - 1; ++i) {
      if (std::holds_alternative<Space>(s.rho[i])) {
        errors.push_back(ErrorCode::spacesWrong);
      }
    }
  }
  static void verifyRule(const CompoundRule& c, std::vector<Error>& errors) {
    for (const SimpleRule& s : c.components) {
      verifyRule(s, errors);
    }
  }
  static void verifyRule(const Rule& r, std::vector<Error>& errors) {
    std::visit([&](const auto& arg) { verifyRule(arg, errors); }, r);
  }
  void SCA::verify(std::vector<Error>& errors) const {
    for (const SoundChange& sc : rules) {
      verifyRule(sc.rule, errors);
    }
  }
}
