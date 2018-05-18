#pragma once

#include <optional>
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
    void setFeatureValue(size_t f, size_t i) {
      if (f >= featureValues.size()) featureValues.resize(f + 1);
      featureValues[f] = i;
    }
    bool hasClass(size_t cc) const { return charClass == cc; }
  };
  struct MChar;
  struct CharMatcher {
    struct Constraint {
      size_t feature;
      size_t instance;
      bool matches(size_t otherInstance) const {
        return instance == otherInstance;
      }
    };
    size_t charClass;
    size_t index;
    std::variant<std::vector<Constraint>, std::vector<const PhonemeSpec*>>
    constraints;
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
  struct MChar {
    MChar() {}
    MChar(std::string&& s) : value(std::move(s)) {}
    MChar(CharMatcher&& c) : value(std::move(c)) {}
    MChar(Space s) : value(s) {}
    MChar(PhonemeSpec&& ps) : value(std::move(ps)) {}
    MChar(const std::string& s) : value(s) {}
    MChar(const CharMatcher& c) : value(c) {}
    MChar(const PhonemeSpec& ps) : value(ps) {}
    std::variant<std::string, CharMatcher, Space, PhonemeSpec> value;
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
  };
  using MString = std::vector<MChar>;
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
    MString lambda, rho;
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
