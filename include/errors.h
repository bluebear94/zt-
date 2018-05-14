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
  };
  struct Error {
    ErrorCode ec;
    std::string details;
    Error(ErrorCode ec) : ec(ec) {}
    Error(ErrorCode ec, std::string&& details) :
      ec(ec), details(std::move(details)) {}
    bool operator==(const Error& e) { return ec == e.ec; }
    bool operator==(const ErrorCode e) { return ec == e; }
    bool operator!=(const Error& e) { return ec != e.ec; }
    bool operator!=(const ErrorCode e) { return ec != e; }
  };
  void printError(const Error& err);
  const char* stringError(ErrorCode ec);
  inline Error operator%(ErrorCode ec, const std::string& s) {
    return Error(ec, std::string(s));
  }
}
