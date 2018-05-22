#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>

#include <boost/filesystem.hpp>

#include "Lexer.h"
#include "Parser.h"
#include "Rule.h"
#include "SCA.h"
#include "Token.h"

namespace fs = boost::filesystem;

const char* defaultFormat = "%a%?p[#]%p -> %o";

const char* usage = R".(Usage:
  %s [-f <formatter>] <script.zt> [words.txt]

  * <script.zt>: a path to a ztš script to apply to the words
  * <words.txt>: a path to a file of newline-separated words, or stdin
      if omitted. If a '#' is found on a line, the substring after it
      will be passed as the part of speech, while the actual word is
      truncated before the '#'.
  * -f, --format <formatter>: a format string for the output:
    * %a: the input word, without the part of speech
    * %o: the output word
    * %p: the part of speech
    * %?*[...]: prints the string inside the square brackets if a predicate
      is true (given by whatever * is):
      * p: part of speech present
      * P: part of speech absent
  
See README.md for documentation on the ztš language.
).";

struct Config {
  const char* script = nullptr;
  const char* words = nullptr;
  const char* format = defaultFormat;
};

void parse(Config& c, int argc, char** argv) {
  char** w = argv + 1;
  unsigned pos = 0;
  while (*w != nullptr) {
    unsigned mode = 0;
    char* arg = *(w++);
    if (arg[0] == '-') {
      switch (arg[1]) {
        case '-': {
          if (strcmp(arg + 2, "format") == 0) mode = 1;
          else mode = -1;
          break;
        }
        case 'f': mode = 1; break;
        default: mode = -1; break;
      }
    }
    if (mode == 1) {
      char* fmter = *(w++);
      if (fmter == nullptr) mode = -1;
      else {
        c.format = fmter;
      }
    } else if (mode == 0) {
      switch (pos++) {
        case 0: c.script = arg; break;
        case 1: c.words = arg; break;
        default: mode = -1;
      }
    }
    if (mode == -1) {
      fprintf(stderr, usage, argv[0]);
      exit(1);
    }
  }
}

std::string format(
    const char* pattern,
    const char* a, const char* o,
    const char* p) {
  std::string res;
  const char* w = pattern;
  while (*w != '\0') {
    if (*w != '%') res += *w;
    else switch (char opt = *(++w)) {
      case 'a': res += a; break;
      case 'o': res += o; break;
      case 'p': res += p; break;
      case '?': {
        char c = *(++w);
        bool predKnown = true;
        bool pred = false;
        switch (c) {
          case 'p': pred = (*p) != '\0'; break;
          case 'P': pred = (*p) == '\0'; break;
          default: predKnown = false;
        }
        if (predKnown && *(++w) == '[') {
          while (*(++w) != ']') {
            if (*w == '\0') {
              std::cerr << "Unclosed conditional " << *w << "\n";
            }
            if (pred) res += *w;
          }
        } else {
          std::cerr << "Unknown condition " << c << "\n";
          exit(-1);
        }
        break;
      }
      case '\0':
      std::cerr << "% at end of formatter\n";
      exit(1);
      default:
      std::cerr << "Unknown option " << opt << "\n";
      exit(1);
    }
    ++w;
  }
  return res;
}

int main(int argc, char** argv) {
  Config c;
  parse(c, argc, argv);
  if (!fs::exists(c.script) || fs::is_directory(c.script)) {
    std::cerr << "File " << c.script << " doesn't exist or is a directory\n";
    return 1;
  }
  if (c.words != nullptr &&
      (!fs::exists(c.words) || fs::is_directory(c.words))) {
    std::cerr << "File " << c.words << " doesn't exist or is a directory\n";
    return 1;
  }
  std::fstream fh(c.script);
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
  std::istream* wfh = (c.words != nullptr) ?
    new std::fstream(argv[2]) : &(std::cin);
  std::string line;
  while (!wfh->eof()) {
    std::getline(*wfh, line);
    if (line.empty()) continue;
    size_t i = line.find("#");
    std::string pos;
    if (i != std::string::npos) {
      pos = line.substr(i + 1);
      line.resize(i);
    }
    std::string output = mysca.apply(line, pos);
    std::cout
      << format(c.format, line.c_str(), output.c_str(), pos.c_str())
      << "\n";
  }
  if (c.words != nullptr) delete wfh;
  return 0;
}
