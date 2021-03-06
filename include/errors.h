#pragma once

#include <string>

namespace sca {
  enum class ErrorCode {
    ok,
    noSuchFeature,
    noSuchFeatureInstance,
    featureExists,
    noSuchClass,
    classExists,
    phonemeAlreadyHasClass,
    noSuchPhoneme,
    explicitLabelZero,
    mixedMatchers,
    spacesWrong,
    undefinedMatcher,
    nonCoreFeatureSet,
    enumCharCountMismatch,
    enumToNonEnum,
    invalidOperatorOmega,
    multipleInstancesOmega,
    nonSingleCharInOmega,
    orderedConstraintUnorderedFeature,
    undefinedDependentConstraint,
  };
  struct Error {
    ErrorCode ec;
    std::string details;
    size_t line = -1, col = -1;
    Error(ErrorCode ec) : ec(ec) {}
    Error(ErrorCode ec, std::string&& details) :
      ec(ec), details(std::move(details)) {}
    bool ok() const { return ec == ErrorCode::ok; }
    bool operator==(const Error& e) const { return ec == e.ec; }
    bool operator==(const ErrorCode e) const { return ec == e; }
    bool operator!=(const Error& e) const { return ec != e.ec; }
    bool operator!=(const ErrorCode e) const { return ec != e; }
    Error& at(size_t line, size_t col) {
      this->line = line;
      this->col = col;
      return *this;
    }
  };
  void printError(const Error& err);
  std::string errorAsString(const Error& err);
  const char* stringError(ErrorCode ec);
  inline Error operator%(ErrorCode ec, const std::string& s) {
    return Error(ec, std::string(s));
  }
}
