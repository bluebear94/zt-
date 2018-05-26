#pragma once

#include <limits.h>
#include <stddef.h>

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <lua.hpp>

#include "PHash.h"
#include "Rule.h"
#include "Token.h"
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
    size_t line, col;
  };
  struct Feature {
    std::string featureName;
    std::vector<std::string> instanceNames;
    size_t def = 0;
    size_t line, col;
    bool isCore;
    bool ordered = false;
    [[nodiscard]] Error getFeatureInstanceByName(
      const std::string& name, size_t& id) const;
  };
  using PhonemesByFeature = std::vector<std::vector<std::string>>;
  struct SoundChangeOptions {
    EvaluationOrder eo = EvaluationOrder::ltr;
    Behaviour beh = Behaviour::once;
  };
  struct SoundChange {
    std::unique_ptr<Rule> rule;
    SoundChangeOptions opt;
    std::unordered_set<std::string> poses;
    void apply(const SCA& sca, WString& st, const std::string& pos) const;
  };
  class SCA {
  public:
    SCA();
    [[nodiscard]] Error insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature);
    [[nodiscard]] Error insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes,
      size_t line, size_t col);
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
    const CharClass* getClassByIDOrNull(size_t id) const {
      return (0 <= id && id < charClasses.size())
        ? &(charClasses[id]) : nullptr;
    }
    const Feature* getFeatureByIDOrNull(size_t id) const {
      return (0 <= id && id < features.size())
        ? &(features[id]) : nullptr;
    }
    CharClass* getClassByIDOrNull(size_t id) {
      return (0 <= id && id < charClasses.size())
        ? &(charClasses[id]) : nullptr;
    }
    Feature* getFeatureByIDOrNull(size_t id) {
      return (0 <= id && id < features.size())
        ? &(features[id]) : nullptr;
    }
    void insertSoundChange(SoundChange&& sc) {
      rules.push_back(std::move(sc));
    }
    void reversePhonemeMap();
    auto getPhonemesBySpec(const PhonemeSpec& ps) const {
      return phonemesReverse.equal_range(ps);
    }
    auto findOrInsertPhoneme(const std::string& name) {
      auto res = phonemes.try_emplace(name);
      if (res.second) {
        res.first->second.name = name;
      }
      return res.first;
    }
    void verify(std::vector<Error>& errors) const;
    std::string apply(
      const std::string_view& st, const std::string& pos) const;
    void addGlobalLuaCode(const LuaCode& lc);
    std::string executeGlobalLuaCode();
    std::string wStringToString(const WString& ws) const;
    lua_State* getLuaState() const { return luaState.get(); }
  private:
    std::vector<CharClass> charClasses;
    std::vector<Feature> features;
    std::unordered_map<std::string, size_t> featuresByName;
    std::unordered_map<std::string, size_t> classesByName;
    std::unordered_map<std::string, PhonemeSpec> phonemes;
    std::vector<SoundChange> rules;
    std::unordered_multimap<
      PhonemeSpec, std::string, PSHash, PSEqual> phonemesReverse;
    mutable std::unique_ptr<lua_State, decltype(&lua_close)>
      luaState;
    std::string globalLuaCode;
  };
  void splitIntoPhonemes(
    const SCA& sca, const std::string_view s,
    MString& phonemes);
  void splitIntoPhonemes(
    const SCA& sca, const std::string_view s,
    std::deque<std::string>& phonemes);
}
