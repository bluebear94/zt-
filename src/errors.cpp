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
    "Explicit index cannot be zero",
    "Use of both unlabelled and labelled matchers in a simple rule",
    "Space character is not first in λ or last in ρ",
    "Matcher in ω that was not defined in α, λ or ρ",
    "Non-core feature changed in matcher in ω",
    "Character count mismatch to a previous enumerating matcher",
    "Enumerating matcher refers to previous non-enumerating matcher",
    "Constraint operator other than `==` in ω",
  };
  const char* stringError(ErrorCode ec) {
    int n = (int) ec;
    if (n >= 0 && n < (sizeof(errorCodes) / sizeof(*errorCodes)))
      return errorCodes[n];
    return "Unknown error";
  }
  void printError(const Error& err) {
    std::cerr << "SCA error: " << stringError(err.ec) <<
      " (#" << (int) err.ec << ")";
    if (!err.details.empty()) std::cerr << ": " << err.details;
    if (err.line != -1 && err.col != -1) {
      std::cerr << " at line " << (err.line + 1) <<
        ", column " << (err.col + 1);
    }
    std::cerr << "\n";
  }
}
