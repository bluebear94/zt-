#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <lua.hpp>

#include "PHash.h"
#include "PUnique.h"
#include "errors.h"

namespace sca {
  class SCA;
  struct SoundChange;
  class MatchResult;
  using MatchCapture = std::unordered_map<
    std::pair<size_t, size_t>,
    MatchResult,
    PHash<size_t, size_t>>;
  struct PhonemeSpec {
    std::string name;
    size_t charClass = -1;
    std::vector<size_t> featureValues;
    size_t getFeatureValue(size_t f, const SCA& sca) const;
    void setFeatureValue(size_t f, size_t i, const SCA& sca);
    bool hasClass(size_t cc) const { return charClass == cc; }
  };
  struct MChar;
  using MString = std::vector<MChar>;
  enum class Comparison {
    eq,
    ne,
    lt,
    gt,
    le,
    ge,
  };
  struct CharMatcher {
    struct Constraint {
      using IV = std::variant<size_t, std::pair<size_t, size_t>>;
      size_t feature;
      // Could probably use unordered_set here, but we don't forsee
      // any constraints that match lots of instances.
      std::vector<IV> instances;
      Comparison c;
      bool matches(
        size_t otherInstance, const MatchCapture& mc, const SCA& sca) const;
      std::string toString(const SCA& sca) const;
      size_t evaluate(size_t i, const MatchCapture& mc, const SCA& sca) const;
    };
    size_t charClass;
    size_t index;
    std::variant<std::vector<Constraint>, std::vector<const PhonemeSpec*>>
    constraints;
    std::string toString(const SCA& sca) const;
    bool hasConstraints() const {
      return std::holds_alternative<std::vector<Constraint>>(constraints);
    }
    std::vector<Constraint>& getConstraints() {
      return std::get<std::vector<Constraint>>(constraints);
    }
    std::vector<const PhonemeSpec*>& getEnumeration() {
      return std::get<std::vector<const PhonemeSpec*>>(constraints);
    }
    const std::vector<Constraint>& getConstraints() const {
      return std::get<std::vector<Constraint>>(constraints);
    }
    const std::vector<const PhonemeSpec*>& getEnumeration() const {
      return std::get<std::vector<const PhonemeSpec*>>(constraints);
    }
  };
  struct Space {};
  struct Alternation {
    std::vector<MString> options;
  };
  struct Repeat {
    MString s;
    size_t min, max;
  };
  struct MChar {
    MChar() {}
    MChar(std::string&& s) : value(std::move(s)) {}
    MChar(CharMatcher&& c) : value(std::move(c)) {}
    MChar(Space s) : value(s) {}
    MChar(PhonemeSpec&& ps) : value(std::move(ps)) {}
    MChar(Alternation&& a) : value(std::move(a)) {}
    MChar(Repeat&& r) : value(std::move(r)) {}
    explicit MChar(const std::string& s) : value(s) {}
    explicit MChar(const CharMatcher& c) : value(c) {}
    explicit MChar(const PhonemeSpec& ps) : value(ps) {}
    explicit MChar(const Alternation& a) : value(a) {}
    explicit MChar(const Repeat& r) : value(r) {}
    std::variant<
      std::string,
      CharMatcher,
      Space,
      PhonemeSpec,
      Alternation,
      Repeat
    > value;
    template<typename T>
    bool is() const { return std::holds_alternative<T>(value); }
    template<typename T>
    const T& as() const { return std::get<T>(value); }
    template<typename T>
    T&& as() { return std::move(std::get<T>(value)); }
    template<typename T>
    bool isT(const T& s) const {
      return is<T>() && as<T>() == s;
    }
    bool isSingleCharacter() const {
      return is<std::string>() || is<CharMatcher>() || is<Space>() ||
        is<PhonemeSpec>();
    }
  };
  using MSI = typename MString::iterator;
  using MSRI = typename MString::reverse_iterator;
  using MSCI = typename MString::const_iterator;
  using MSRCI = typename MString::const_reverse_iterator;
  using WString = std::vector<PUnique<const PhonemeSpec>>;
  class Rule {
  public:
    virtual ~Rule() {};
    virtual std::optional<size_t> tryReplaceLTR(
      const SCA& sca, WString& str, size_t start) const = 0;
    virtual std::optional<size_t> tryReplaceRTL(
      const SCA& sca, WString& str, size_t start) const = 0;
    virtual void verify(
      std::vector<Error>& errors, const SCA& sca, const SoundChange& sc) const
      = 0;
    size_t line = -1, col = -1;
  };
  struct SimpleRule : public Rule {
    std::optional<size_t> tryReplaceLTR(
      const SCA& sca, WString& str, size_t start) const override;
    std::optional<size_t> tryReplaceRTL(
      const SCA& sca, WString& str, size_t start) const override;
    void verify(
      std::vector<Error>& errors,
      const SCA& sca,
      const SoundChange& sc) const override;
    MString alpha, omega;
    std::vector<std::pair<MString, MString>> envs;
    int gammaref = LUA_NOREF;
    bool inv;
    bool setGamma(lua_State* luaState, const std::string_view& s);
  private:
    bool evaluate(lua_State* luaState,
      const WString& word, size_t mstart, size_t mend) const;
  };
  struct CompoundRule : public Rule {
    std::optional<size_t> tryReplaceLTR(
      const SCA& sca, WString& str, size_t start) const override;
    std::optional<size_t> tryReplaceRTL(
      const SCA& sca, WString& str, size_t start) const override;
    void verify(
      std::vector<Error>& errors,
      const SCA& sca,
      const SoundChange& sc) const override;
    std::vector<SimpleRule> components;
  };
}
