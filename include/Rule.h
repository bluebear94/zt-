#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "errors.h"

namespace sca {
  class SCA;
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
  };
  struct CharMatcher {
    struct Constraint {
      size_t feature;
      // Could probably use unordered_set here, but we don't forsee
      // any constraints that match lots of instances.
      std::vector<size_t> instances;
      Comparison c;
      bool matches(size_t otherInstance) const;
      std::string toString(const SCA& sca) const;
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
  class Rule {
  public:
    virtual ~Rule() {};
    virtual std::optional<size_t> tryReplaceLTR(
      const SCA& sca, MString& str, size_t start) const = 0;
    virtual std::optional<size_t> tryReplaceRTL(
      const SCA& sca, MString& str, size_t start) const = 0;
    virtual void verify(std::vector<Error>& errors, const SCA& sca) const {}
    size_t line = -1, col = -1;
  };
  struct SimpleRule : public Rule {
    std::optional<size_t> tryReplaceLTR(
      const SCA& sca, MString& str, size_t start) const override;
    std::optional<size_t> tryReplaceRTL(
      const SCA& sca, MString& str, size_t start) const override;
    void verify(std::vector<Error>& errors, const SCA& sca) const override;
    MString alpha, omega;
    std::vector<std::pair<MString, MString>> envs;
    bool inv;
  };
  struct CompoundRule : public Rule {
    std::optional<size_t> tryReplaceLTR(
      const SCA& sca, MString& str, size_t start) const override;
    std::optional<size_t> tryReplaceRTL(
      const SCA& sca, MString& str, size_t start) const override;
    void verify(std::vector<Error>& errors, const SCA& sca) const override;
    std::vector<SimpleRule> components;
  };
}
