#include <fstream>
#include <iostream>
#include <optional>
#include <type_traits>

#include <boost/filesystem.hpp>

#include "Lexer.h"
#include "Parser.h"
#include "Rule.h"
#include "SCA.h"
#include "Token.h"

namespace fs = boost::filesystem;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <input.zt> <words.txt>\n";
    return 1;
  }
  if (!fs::exists(argv[1]) || fs::is_directory(argv[1])) {
    std::cerr << "File " << argv[1] << " doesn't exist or is a directory\n";
    return 1;
  }
  if (!fs::exists(argv[2]) || fs::is_directory(argv[2])) {
    std::cerr << "File " << argv[2] << " doesn't exist or is a directory\n";
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
  mysca.reversePhonemeMap();
  std::fstream wfh(argv[2]);
  std::string line;
  while (!wfh.eof()) {
    std::getline(wfh, line);
    if (line.empty()) continue;
    std::cout << line << " -> ";
    size_t i = line.find("#");
    std::string pos;
    if (i != std::string::npos) {
      pos = line.substr(i + 1);
      line.resize(i);
    }
    std::cout << mysca.apply(line, pos) << "\n";
  }
  return 0;
}
