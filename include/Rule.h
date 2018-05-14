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
    size_t getFeatureValue(size_t f) const {
      return (f < featureValues.size()) ? featureValues[f] : 0;
    }
    void setFeatureValue(size_t f, size_t i) {
      if (f >= featureValues.size()) featureValues.resize(f + 1);
      featureValues[f] = i;
    }
    bool hasClass(size_t cc) const { return charClass == cc; }
    bool operator==(const PhonemeSpec& other) const {
      if (charClass != other.charClass) return false;
      size_t nf = std::max(featureValues.size(), other.featureValues.size());
      for (size_t i = 0; i < nf; ++i) {
        if (getFeatureValue(i) != other.getFeatureValue(i)) return false;
      }
      return true;
    }
  };
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
    std::vector<Constraint> constraints;
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
  class Rule {
  public:
    virtual ~Rule() {};
    virtual std::optional<size_t> tryReplace(
      const SCA& sca, MString& str, size_t index) const = 0;
    virtual void verify(std::vector<Error>& errors, const SCA& sca) const {}
  };
  struct SimpleRule : public Rule {
    std::optional<size_t> tryReplace(
      const SCA& sca, MString& str, size_t index) const override;
    void verify(std::vector<Error>& errors, const SCA& sca) const override;
    MString alpha, omega;
    MString lambda, rho;
  };
  struct CompoundRule : public Rule {
    std::optional<size_t> tryReplace(
      const SCA& sca, MString& str, size_t index) const override;
    void verify(std::vector<Error>& errors, const SCA& sca) const override;
    std::vector<SimpleRule> components;
  };
}
