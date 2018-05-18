#pragma once

#include <stddef.h>

#include <string>
#include <optional>
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
    boundary,
    slash,
    star,
    bang,
    notEquals,
    kwFeature,
    kwClass,
  };
  struct Token {
    Token() : contents(EndOfFile()) {}
    template<typename T>
    bool is() const { return std::holds_alternative<T>(contents); }
    template<typename T>
    T as() const { return std::get<T>(contents); }
    template<typename T>
    std::optional<T> get() const {
      if (is<T>()) return as<T>();
      return std::nullopt;
    }
    bool isOperator(Operator op) const {
      return is<Operator>() && as<Operator>() == op;
    }
    std::variant<EndOfFile, Operator, std::string, size_t> contents;
    size_t line, col;
  };
}
