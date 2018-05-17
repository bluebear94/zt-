#include "SCA.h"

#include <algorithm>

#include "utf8.h"

namespace sca {
  void splitIntoPhonemes(
      const SCA& sca, const std::string_view s, MString& phonemes) {
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
      auto it = st.begin();
      while (it != st.end()) {
        auto res = rule->tryReplaceLTR(sca, st, it);
        if (res.has_value() && beh == Behaviour::once) break;
        if (beh == Behaviour::loopnsi) it = *res;
        else ++it;
      }
    } else {
      auto it = st.rbegin();
      while (it != st.rend()) {
        auto res = rule->tryReplaceRTL(sca, st, it);
        if (res.has_value() && beh == Behaviour::once) break;
        if (beh == Behaviour::loopnsi) it = *res;
        else ++it;
      }
    }
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
      sc.rule->verify(errors, *this);
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
