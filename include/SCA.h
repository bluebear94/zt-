#pragma once

#include <stddef.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Rule.h"
#include "errors.h"

namespace sca {
  enum class EvaluationOrder {
    ltr,
    rtl,
  };
  struct CharClass {
    std::string name;
  };
  struct Feature {
    std::string featureName;
    std::vector<std::string> instanceNames;
    [[nodiscard]] Error getFeatureInstanceByName(
      const std::string& name, size_t& id) const;
  };
  using PhonemesByFeature = std::vector<std::vector<std::string>>;
  struct PhonemeSpec {
    std::string name;
    size_t charClass = -1;
    std::vector<size_t> featureValues;
    size_t getFeatureValue(size_t f) const {
      return (f < featureValues.size()) ? featureValues[f] : 0;
    }
    bool hasClass(size_t cc) const { return charClass == cc; }
  };
  struct SoundChange {
    std::unique_ptr<Rule> rule;
    EvaluationOrder eo = EvaluationOrder::ltr;
  };
  class SCA {
  public:
    SCA() {}
    [[nodiscard]] Error insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature);
    [[nodiscard]] Error insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes);
    [[nodiscard]] Error getFeatureByName(
      const std::string& name, size_t& id, Feature*& feature);
    [[nodiscard]] Error getClassByName(
      const std::string& name, size_t& id, CharClass*& cclass);
    [[nodiscard]] Error getPhonemeByName(
      const std::string& name, PhonemeSpec*& ps);
    [[nodiscard]] Error getFeatureByName(
      const std::string& name, size_t& id, Feature const*& feature) const;
    [[nodiscard]] Error getClassByName(
      const std::string& name, size_t& id, CharClass const*& cclass) const;
    [[nodiscard]] Error getPhonemeByName(
      const std::string& name, PhonemeSpec const*& ps) const;
    void insertSoundChange(SoundChange&& sc) {
      rules.push_back(std::move(sc));
    }
    void verify(std::vector<Error>& errors) const;
  private:
    std::vector<CharClass> charClasses;
    std::vector<Feature> features;
    std::unordered_map<std::string, size_t> featuresByName;
    std::unordered_map<std::string, size_t> classesByName;
    std::unordered_map<std::string, PhonemeSpec> phonemes;
    std::vector<SoundChange> rules;
  };
}
