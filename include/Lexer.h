#pragma once

#include <stddef.h>

#include <iosfwd>
#include <optional>

#include "Token.h"

namespace sca {
  struct Cursor {
    Cursor(std::istream* s) : s(s), line(0), col(0), off(0) {}
    int read() noexcept;
    int peek() noexcept;
    std::istream* s;
    size_t line, col, off;
  };
  Token empty(const Cursor& c) noexcept;
  class Lexer {
  public:
    Lexer(std::istream* in) : cursor(in) {}
    std::optional<Token> getNext() noexcept;
    size_t getLine() const noexcept { return cursor.line; }
    size_t getCol() const noexcept { return cursor.col; }
  private:
    Cursor cursor;
  };
}
