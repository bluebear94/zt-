#pragma once

#include <stddef.h>

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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
    [[nodiscard]] ErrorCode getFeatureInstanceByName(
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
  struct Space {};
  using MChar = std::variant<std::string, CharMatcher>;
  using MString = std::vector<MChar>;
  struct SimpleRule {
    MString alpha, omega;
    MString lambda, rho;
  };
  struct CompoundRule {
    std::vector<SimpleRule> components;
  };
  using Rule = std::variant<SimpleRule, CompoundRule>;
  struct SoundChange {
    Rule rule;
    EvaluationOrder eo = EvaluationOrder::ltr;
  };
  class SCA {
  public:
    SCA() {}
    [[nodiscard]] ErrorCode insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature);
    [[nodiscard]] ErrorCode insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes);
    [[nodiscard]] ErrorCode getFeatureByName(
      const std::string& name, size_t& id, Feature*& feature);
    [[nodiscard]] ErrorCode getClassByName(
      const std::string& name, size_t& id, CharClass*& cclass);
    [[nodiscard]] ErrorCode getPhonemeByName(
      const std::string& name, PhonemeSpec*& ps);
    [[nodiscard]] ErrorCode getFeatureByName(
      const std::string& name, size_t& id, Feature const*& feature) const;
    [[nodiscard]] ErrorCode getClassByName(
      const std::string& name, size_t& id, CharClass const*& cclass) const;
    [[nodiscard]] ErrorCode getPhonemeByName(
      const std::string& name, PhonemeSpec const*& ps) const;
    void insertSoundChange(SoundChange&& sc) {
      rules.push_back(std::move(sc));
    }
  private:
    std::vector<CharClass> charClasses;
    std::vector<Feature> features;
    std::unordered_map<std::string, size_t> featuresByName;
    std::unordered_map<std::string, size_t> classesByName;
    std::unordered_map<std::string, PhonemeSpec> phonemes;
    std::vector<SoundChange> rules;
  };
}
