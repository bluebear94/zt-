#include "Lexer.h"

#include <ctype.h>

#include <iostream>

namespace sca {
  int Cursor::read() noexcept {
    s->clear();
    s->seekg(off);
    int c = s->get();
    if (c == '\n') {
      ++line;
      col = 0;
    } else if (c != std::char_traits<char>::eof()) {
      ++col;
    }
    off = s->tellg();
    return c;
  }
  int Cursor::peek() noexcept {
    s->clear();
    s->seekg(off);
    return s->peek();
  }
  Token empty(const Cursor& c) noexcept {
    Token t;
    t.line = c.line;
    t.col = c.col;
    return t;
  }
  inline bool isStringChar(int c) {
    return isalpha(c) || (unsigned char) c >= 0x80;
  }
  std::optional<Token> Lexer::getNext() noexcept {
    #define RETURN_OP(x) \
      do { \
        t.contents = x; \
        return t; \
      } while (0)
    int c;
    do {
      c = cursor.read();
      while (c == '#') {
        // Comment; read the first character after the newline
        do {
          c = cursor.read();
        } while (c != '\n' && c != std::char_traits<char>::eof());
        c = cursor.read();
      }
    } while (isspace(c));
    Token t = empty(cursor);
    switch (c) {
      case std::char_traits<char>::eof(): {
        return t; // Already EOF
      }
      case '(': RETURN_OP(Operator::lb);
      case ')': RETURN_OP(Operator::rb);
      case '[': RETURN_OP(Operator::lsb);
      case ']': RETURN_OP(Operator::rsb);
      case '{': RETURN_OP(Operator::lcb);
      case '}': RETURN_OP(Operator::rcb);
      case ',': RETURN_OP(Operator::comma);
      case '=': RETURN_OP(Operator::equals);
      case ':': RETURN_OP(Operator::colon);
      case ';': RETURN_OP(Operator::semicolon);
      case '_': RETURN_OP(Operator::placeholder);
      case '~': RETURN_OP(Operator::boundary);
      case '/': RETURN_OP(Operator::slash);
      case '*': RETURN_OP(Operator::star);
      case '+': RETURN_OP(Operator::plus);
      case '?': RETURN_OP(Operator::question);
      case '|': {
        Cursor temp = cursor;
        int d = cursor.read();
        if (d == '|') {
          RETURN_OP(Operator::envOr);
        }
        cursor = temp;
        RETURN_OP(Operator::pipe);
      }
      case '!': {
        Cursor temp = cursor;
        int d = cursor.read();
        if (d == '=') {
          RETURN_OP(Operator::notEquals);
        }
        cursor = temp;
        RETURN_OP(Operator::bang);
      }
      case '$': {
        Cursor temp = cursor;
        int d = cursor.read();
        switch (d) {
          case '(': RETURN_OP(Operator::dlb);
          default: {
            cursor = temp;
            return std::nullopt;
          }
        }
      }
      case '-': {
        Cursor temp = cursor;
        int d = cursor.read();
        switch (d) {
          case '>': RETURN_OP(Operator::arrow);
          default: {
            cursor = temp;
            return std::nullopt;
          }
        }
      }
      case 0x22: {
        std::string s;
        while (true) {
          int d = cursor.read();
          if (d == std::char_traits<char>::eof()) return std::nullopt;
          if (d == 0x22) {
            t.contents = std::move(s);
            return t;
          } else if (d == '\\') {
            int e = cursor.read();
            switch (e) {
              case std::char_traits<char>::eof(): return std::nullopt;
              case 0x22: s += (char) 0x22; break;
              case '\\': s += '\\'; break;
              case 'n': s += '\n'; break;
            }
          } else {
            s += (char) d;
          }
        }
      }
      default: {
        if (isStringChar(c)) {
          // Alphabetic: treat this as a string
          std::string s{(char) c};
          int d;
          while (true) {
            d = cursor.peek();
            if (isStringChar(d)) {
              s += (char) d;
              (void) cursor.read();
            } else break;
          }
          if (s == "feature") t.contents = Operator::kwFeature;
          else if (s == "class") t.contents = Operator::kwClass;
          else if (s == "NOT") t.contents = Operator::bang;
          else t.contents = std::move(s);
          return t;
        } else if (isdigit(c)) {
          size_t n = c - '0';
          int d;
          while (true) {
            d = cursor.peek();
            if (isdigit(d)) {
              n *= 10;
              n += c - '0';
              (void) cursor.read();
            } else break;
          }
          t.contents = n;
          return t;
        }
      }
    }
    return std::nullopt;
  }
}
