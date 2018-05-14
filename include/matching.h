#pragma once

#include <optional>

#include "SCA.h"

namespace sca {
  bool charsMatch(const SCA& sca, const MChar& fr, const MChar& fi);
  std::optional<size_t> matchesFromStart(
    const SCA& sca,
    const SimpleRule& rule, const MString& str, size_t index);
  std::optional<size_t> matchesFromStart(
    const SCA& sca,
    const CompoundRule& rule, const MString& str, size_t index);
  std::optional<size_t> matchesFromStart(
    const SCA& sca,
    const Rule& rule, const MString& str, size_t index);
}
