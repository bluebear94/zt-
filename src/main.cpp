#include <fstream>
#include <iostream>
#include <optional>
#include <type_traits>

#include "Lexer.h"
#include "Parser.h"
#include "SCA.h"
#include "Token.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input.zt>\n";
    return 1;
  }
  std::fstream fh(argv[1]);
  sca::Lexer lexer(&fh);
  sca::SCA mysca;
  sca::Parser parser(&lexer, &mysca);
  bool res = parser.parse();
  if (!res) return 1;
  std::vector<sca::Error> errors;
  mysca.verify(errors);
  for (const sca::Error& e : errors)
    sca::printError(e);
  if (!errors.empty()) return 1;
  return 0;
}
