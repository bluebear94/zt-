#pragma once

#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "errors.h"

namespace sca {
  class SCA;
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
  using MChar = std::variant<std::string, CharMatcher, Space>;
  using MString = std::vector<MChar>;
  class Rule {
  public:
    virtual ~Rule() {};
    virtual std::optional<size_t> tryReplace(
      const SCA& sca, MString& str, size_t index) const = 0;
    virtual void verify(std::vector<Error>& errors) const {}
  };
  struct SimpleRule : public Rule {
    std::optional<size_t> tryReplace(
      const SCA& sca, MString& str, size_t index) const override;
    void verify(std::vector<Error>& errors) const override;
    MString alpha, omega;
    MString lambda, rho;
  };
  struct CompoundRule : public Rule {
    std::optional<size_t> tryReplace(
      const SCA& sca, MString& str, size_t index) const override;
    void verify(std::vector<Error>& errors) const override;
    std::vector<SimpleRule> components;
  };
}
