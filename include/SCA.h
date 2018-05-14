#pragma once

#include <stddef.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "errors.h"

namespace sca {
  struct CharClass {
    std::string name;
  };
  struct Feature {
    std::string featureName;
    std::vector<std::string> instanceNames;
    [[nodiscard]] ErrorCode getFeatureInstanceByName(
      const std::string& name, size_t& id);
  };
  using PhonemesByFeature = std::vector<std::vector<std::string>>;
  struct PhonemeSpec {
    std::string name;
    size_t charClass = -1;
    std::vector<size_t> featureValues;
    size_t getFeatureValue(size_t f) const {
      return (f < featureValues.size()) ? featureValues[f] : 0;
    }
  };
  struct CharMatcher {
    struct Constraint {
      size_t feature;
      size_t instance;
    };
    size_t charClass;
    size_t index;
    std::vector<Constraint> constraints;
  };
  class SCA {
  public:
    [[nodiscard]] ErrorCode insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature);
    [[nodiscard]] ErrorCode insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes);
    [[nodiscard]] ErrorCode getFeatureByName(
      const std::string& name, size_t& id, Feature*& feature);
    [[nodiscard]] ErrorCode getClassByName(
      const std::string& name, size_t& id, CharClass*& cclass);
  private:
    std::vector<CharClass> charClasses;
    std::vector<Feature> features;
    std::unordered_map<std::string, size_t> featuresByName;
    std::unordered_map<std::string, size_t> classesByName;
    std::unordered_map<std::string, PhonemeSpec> phonemes;
  };
}
