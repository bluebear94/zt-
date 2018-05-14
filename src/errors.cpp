#include "errors.h"

#include <iostream>

namespace sca {
  static const char* errorCodes[] = {
    "OK",
    "No such feature",
    "No such instance in the feature",
    "Feature already exists",
    "No such character class",
    "Character class already exists",
    "Phoneme already has character class",
    "No such phoneme",
  };
  const char* stringError(ErrorCode ec) {
    int n = (int) ec;
    if (n >= 0 && n < (sizeof(errorCodes) / sizeof(*errorCodes)))
      return errorCodes[n];
    return "Unknown error";
  }
  void printError(ErrorCode ec) {
    std::cerr << "SCA error: " << stringError(ec) <<
      " (#" << (int) ec << ")\n";
  }
}
