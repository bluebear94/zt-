#pragma once

#include <limits.h>
#include <stddef.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "Rule.h"
#include "errors.h"

namespace sca {
  enum class EvaluationOrder {
    ltr,
    rtl,
  };
  enum class Behaviour {
    once,
    loopnsi,
    loopsi,
  };
  struct CharClass {
    std::string name;
  };
  struct Feature {
    std::string featureName;
    std::vector<std::string> instanceNames;
    size_t def = 0;
    bool isCore;
    [[nodiscard]] Error getFeatureInstanceByName(
      const std::string& name, size_t& id) const;
  };
  using PhonemesByFeature = std::vector<std::vector<std::string>>;
  bool arePhonemeSpecsEqual(
    const SCA& sca, const PhonemeSpec& a, const PhonemeSpec& b);
  struct PSEqual {
    size_t operator()(const PhonemeSpec& a, const PhonemeSpec& b) const {
      return arePhonemeSpecsEqual(*sca, a, b);
    }
    const SCA* sca;
  };
  struct PSHash {
    size_t operator()(const PhonemeSpec& ps) const;
    const SCA* sca;
  };
  struct SoundChange {
    std::unique_ptr<Rule> rule;
    EvaluationOrder eo = EvaluationOrder::ltr;
    Behaviour beh = Behaviour::once;
    std::unordered_set<std::string> poses;
    void apply(const SCA& sca, MString& st, const std::string& pos) const;
  };
  class SCA {
  public:
    SCA() : phonemesReverse(16, PSHash{this}, PSEqual{this}) {}
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
    const CharClass& getClassByID(size_t id) const { return charClasses[id]; }
    const Feature& getFeatureByID(size_t id) const { return features[id]; }
    void insertSoundChange(SoundChange&& sc) {
      rules.push_back(std::move(sc));
    }
    void reversePhonemeMap();
    auto getPhonemesBySpec(const PhonemeSpec& ps) const {
      return phonemesReverse.equal_range(ps);
    }
    void verify(std::vector<Error>& errors) const;
    std::string apply(
      const std::string_view& st, const std::string& pos) const;
  private:
    std::vector<CharClass> charClasses;
    std::vector<Feature> features;
    std::unordered_map<std::string, size_t> featuresByName;
    std::unordered_map<std::string, size_t> classesByName;
    std::unordered_map<std::string, PhonemeSpec> phonemes;
    std::vector<SoundChange> rules;
    std::unordered_multimap<
      PhonemeSpec, std::string, PSHash, PSEqual> phonemesReverse;
  };
  void splitIntoPhonemes(
    const SCA& sca, const std::string_view s, MString& phonemes);
}
