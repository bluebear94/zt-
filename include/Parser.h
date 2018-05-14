#pragma once

#include <stddef.h>

#include <optional>
#include <utility>
#include <vector>

#include "SCA.h"
#include "Token.h"
#include "errors.h"

namespace sca {
  class Lexer;
  class Parser {
  public:
    Parser(Lexer* l, SCA* sca) : l(l), sca(sca) {}
    std::optional<Error> parseStatement(size_t& which);
    bool parse();
  private:
    std::vector<Token> tokens;
    size_t index = 0;
    Lexer* l;
    SCA* sca;
    const Token& getToken();
    const Token& peekToken();
    std::optional<Operator> parseOperator(Operator op);
    std::optional<std::string> parseString();
    std::optional<size_t> parseNumber();
    std::optional<std::pair<Feature, PhonemesByFeature>> parseFeature();
    std::optional<std::pair<std::string, std::vector<std::string>>>
    parseCharClass();
    std::optional<size_t> parseMatcherIndex();
    std::optional<CharMatcher::Constraint> parseMatcherConstraint();
    std::optional<CharMatcher> parseMatcher();
    std::optional<std::unique_ptr<SimpleRule>> parseSimpleRule();
    std::optional<std::unique_ptr<CompoundRule>> parseCompoundRule();
    std::optional<std::unique_ptr<Rule>> parseRule();
    std::optional<SoundChange> parseSoundChange();
    bool parseChar(MString& m, bool allowSpaces);
    std::optional<MString> parseString(bool allowSpaces);
    void printLineColumn();
  };
}
