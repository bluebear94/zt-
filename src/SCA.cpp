#include "SCA.h"

#include <algorithm>

#include "sca_lua.h"
#include "utf8.h"

namespace sca {
  template<typename T>
  void splitIntoPhonemes(
      const SCA& sca, const std::string_view s,
      T& phonemes) {
    size_t ei = s.length();
    if (ei == 0) return;
    const PhonemeSpec* ps = nullptr;
    // Go from the whole string and chop off the last character until
    // we get a match
    std::string st(s);
    while (ei >= 1) {
      Error res = sca.getPhonemeByName(st, ps);
      if (res == ErrorCode::ok) break;
      st.pop_back();
      --ei;
    }
    if (ps != nullptr) {
      phonemes.push_back(std::move(st));
      splitIntoPhonemes(sca, s.substr(ei), phonemes);
    } else {
      // No match found; just take the first codepoint
      UTF8Iterator<const std::string_view> it(s);
      ++it;
      size_t pos = it.position();
      phonemes.push_back(std::string(s.substr(0, pos)));
      splitIntoPhonemes(sca, s.substr(pos), phonemes);
    }
  }
  void splitIntoPhonemes(
      const SCA& sca, const std::string_view s,
      std::vector<MChar>& phonemes) {
    splitIntoPhonemes<std::vector<MChar>>(sca, s, phonemes);
  }
  void splitIntoPhonemes(
      const SCA& sca, const std::string_view s,
      std::deque<std::string>& phonemes) {
    splitIntoPhonemes<std::deque<std::string>>(sca, s, phonemes);
  }
  // I hope features with lots of instances aren't that common.
  Error Feature::getFeatureInstanceByName(
      const std::string& name, size_t& id) const {
    auto it = std::find(instanceNames.begin(), instanceNames.end(), name);
    if (it == instanceNames.end())
      return ErrorCode::noSuchFeatureInstance % name;
    id = it - instanceNames.begin();
    return ErrorCode::ok;
  }
  void SoundChange::apply(
      const SCA& sca, MString& st, const std::string& pos) const {
    if (!poses.empty() && poses.count(pos) == 0) return;
    if (eo == EvaluationOrder::ltr) {
      size_t i = 0;
      // `<=` is intentional. We allow matching one character past the end
      // to allow epenthesis rules such as the following:
      // -> i (t _ ~);
      while (i <= st.size()) {
        auto res = rule->tryReplaceLTR(sca, st, i);
        if (res.has_value() && beh == Behaviour::once) break;
        if (beh == Behaviour::loopnsi && res.has_value()) i += *res;
        else ++i;
      }
    } else {
      size_t i = 0;
      while (i <= st.size()) {
        auto res = rule->tryReplaceRTL(sca, st, i);
        if (res.has_value() && beh == Behaviour::once) break;
        if (beh == Behaviour::loopnsi && res.has_value()) i += *res;
        else ++i;
      }
    }
  }
  SCA::SCA() :
      phonemesReverse(16, PSHash{this}, PSEqual{this}),
      luaState(luaL_newstate(), &lua_close) {
    luaL_openlibs(luaState.get());
  }
  Error SCA::insertFeature(
      Feature&& f, const PhonemesByFeature& phonemesByFeature) {
    size_t oldFeatureCount = features.size();
    auto res = featuresByName.insert(
      std::pair{f.featureName, oldFeatureCount});
    if (!res.second) {
      const Feature& old = features[res.first->second];
      return (ErrorCode::featureExists % f.featureName).at(old.line, old.col);
    }
    features.push_back(std::move(f));
    for (size_t ii = 0; ii < phonemesByFeature.size(); ++ii) {
      const std::vector<std::string>& phonemesInInstance =
        phonemesByFeature[ii];
      for (const std::string& phoneme : phonemesInInstance) {
        PhonemeSpec& spec = phonemes[phoneme];
        spec.name = phoneme;
        spec.setFeatureValue(oldFeatureCount, ii, *this);
      }
    }
    return ErrorCode::ok;
  }
  Error SCA::insertClass(
      std::string&& name, const std::vector<std::string>& myPhonemes,
      size_t line, size_t col) {
    size_t oldClassCount = charClasses.size();
    auto res = classesByName.insert(
      std::pair{name, oldClassCount});
    if (!res.second) {
      const CharClass& old = charClasses[res.first->second];
      return (ErrorCode::classExists % name).at(old.line, old.col);
    }
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
    newClass.line = line;
    newClass.col = col;
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
  void SCA::verify(std::vector<Error>& errors) const {
    for (const SoundChange& sc : rules) {
      sc.rule->verify(errors, *this, sc);
    }
  }
  void SCA::reversePhonemeMap() {
    for (const auto& p : phonemes) {
      phonemesReverse.insert(std::pair(p.second, p.first));
    }
  }
  std::string SCA::apply(
      const std::string_view& st, const std::string& pos) const {
    MString ms;
    splitIntoPhonemes(*this, st, ms);
    for (const SoundChange& r : rules)
      r.apply(*this, ms, pos);
    std::string s;
    for (const MChar& mc : ms) {
      if (mc.is<std::string>())
        s += mc.as<std::string>();
      else if (mc.is<PhonemeSpec>()) {
        const auto& spec = mc.as<PhonemeSpec>();
        s += "[phoneme/";
        if (spec.charClass == -1) s += '*';
        else s += charClasses[spec.charClass].name;
        s += ':';
        bool first = true;
        for (size_t i = 0; i < features.size(); ++i) {
          if (!features[i].isCore) continue;
          if (!first) s += ',';
          size_t k = spec.getFeatureValue(i, *this);
          s += features[i].featureName;
          s += '=';
          s += features[i].instanceNames[k];
          first = false;
        }
        s += "]";
      }
      else s += "#";
    }
    return s;
  }
  void SCA::addGlobalLuaCode(const LuaCode& lc) {
    globalLuaCode += lc.code;
  }
  std::string SCA::executeGlobalLuaCode() {
    if (globalLuaCode.empty()) return "";
    sca::lua::init(luaState.get());
    sca::lua::pushSCA(luaState.get(), *this);
    lua_setglobal(luaState.get(), "sca");
    int stat = luaL_loadbufferx(
      luaState.get(),
      globalLuaCode.c_str(), globalLuaCode.size(),
      "<global code>", "t");
    if (stat != LUA_OK) goto rek;
    stat = lua_pcall(luaState.get(), 0, 0, 0);
    if (stat != LUA_OK) goto rek;
    return "";
    rek:
    std::string s = lua_tostring(luaState.get(), -1);
    lua_pop(luaState.get(), -1);
    return s;
  }
}
