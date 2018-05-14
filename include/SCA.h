#pragma once

#include <limits.h>
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
    [[nodiscard]] Error getFeatureInstanceByName(
      const std::string& name, size_t& id) const;
  };
  using PhonemesByFeature = std::vector<std::vector<std::string>>;
  struct PSHash {
    size_t operator()(const PhonemeSpec& ps) const {
      size_t x = std::hash<size_t>()(ps.charClass);
      constexpr size_t bits = CHAR_BIT * sizeof(size_t);
      int rot = 0;
      for (size_t fv : ps.featureValues) {
        // use raw to avoid issues on 0
        x ^= (fv << rot) | (fv >> (bits - rot));
        rot = (rot + 1) % bits;
      }
      return x;
    }
  };
  struct SoundChange {
    std::unique_ptr<Rule> rule;
    EvaluationOrder eo = EvaluationOrder::ltr;
    Behaviour beh = Behaviour::once;
    void apply(const SCA& sca, MString& st) const;
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
    const CharClass& getClassByID(size_t id) const { return charClasses[id]; }
    void insertSoundChange(SoundChange&& sc) {
      rules.push_back(std::move(sc));
    }
    void reversePhonemeMap();
    auto getPhonemesBySpec(const PhonemeSpec& ps) const {
      return phonemesReverse.equal_range(ps);
    }
    void verify(std::vector<Error>& errors) const;
    std::string apply(const std::string_view& st) const;
  private:
    std::vector<CharClass> charClasses;
    std::vector<Feature> features;
    std::unordered_map<std::string, size_t> featuresByName;
    std::unordered_map<std::string, size_t> classesByName;
    std::unordered_map<std::string, PhonemeSpec> phonemes;
    std::vector<SoundChange> rules;
    std::unordered_multimap<
      PhonemeSpec, std::string, PSHash> phonemesReverse;
  };
  void splitIntoPhonemes(
    const SCA& sca, const std::string_view s, MString& phonemes);
}
