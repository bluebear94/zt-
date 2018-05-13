#include <fstream>
#include <iostream>
#include <optional>
#include <type_traits>

#include "Lexer.h"
#include "Token.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input.zt>\n";
    return 1;
  }
  std::fstream fh(argv[1]);
  sca::Lexer lexer(&fh);
  bool done = false;
  while (!done) {
    std::optional<sca::Token> thuh = lexer.getNext();
    if (thuh) {
      sca::Token& t = *thuh;
      std::visit([&done](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, sca::EndOfFile>) {
          std::cout << "(end of file)\n";
          done = true;
        } else if constexpr (std::is_same_v<T, sca::Operator>) {
          std::cout << "Operator #" << (int) arg << "\n";
        } else if constexpr (std::is_same_v<T, std::string>) {
          std::cout << "String: " << arg << "\n";
        } else if constexpr (std::is_same_v<T, size_t>) {
          std::cout << "Number: " << arg << "\n";
        } else {
          std::cout << "(unknown type)\n";
        }
      }, t.contents);
    } else {
      std::cerr << "Lexer error at line " <<
        (lexer.getLine() + 1) << " column " <<
        (lexer.getCol() + 1) << "\n";
    }
  }
  return 0;
}
