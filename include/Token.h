#pragma once

#include <stddef.h>

#include <string>
#include <variant>

namespace sca {
  struct EndOfFile {};
  enum class Operator {
    arrow,
    lb,
    rb,
    lsb,
    rsb,
    dlb,
    lcb,
    rcb,
    comma,
    equals,
    colon,
    semicolon,
    pipe,
    placeholder,
  };
  struct Token {
    Token() : contents(EndOfFile()) {}
    std::variant<EndOfFile, Operator, std::string, size_t> contents;
    size_t line, col;
  };
}
