#pragma once

namespace sca {
  enum class ErrorCode {
    ok,
    noSuchFeature,
    noSuchFeatureInstance,
    featureExists,
    noSuchClass,
    classExists,
    phonemeAlreadyHasClass,
  };
  void printError(ErrorCode ec);
  const char* stringError(ErrorCode ec);
}
