#include "errors.h"

#include <iostream>
#include <sstream>

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
    "Constraint in ω specifies multiple instances of a feature",
    "Alternation or repetition found in ω",
    "Ordered constraint operator on unordered feature",
    "Dependent constraint was not previously defined",
  };
  const char* stringError(ErrorCode ec) {
    int n = (int) ec;
    if (n >= 0 && n < (sizeof(errorCodes) / sizeof(*errorCodes)))
      return errorCodes[n];
    return "Unknown error";
  }
  static void printError(const Error& err, std::ostream& fh) {
    fh << "SCA error: " << stringError(err.ec) <<
      " (#" << (int) err.ec << ")";
    if (!err.details.empty()) fh << ": " << err.details;
    if (err.line != -1 && err.col != -1) {
      fh << " at line " << (err.line + 1) <<
        ", column " << (err.col + 1);
    }
    fh << "\n";
  }
  void printError(const Error& err) {
    printError(err, std::cerr);
  }
  std::string errorAsString(const Error& err) {
    std::stringstream ss;
    printError(err, ss);
    return ss.str();
  }
}
