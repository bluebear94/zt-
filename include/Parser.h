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
    std::optional<ErrorCode> parseStatement();
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
    std::optional<SimpleRule> parseSimpleRule();
    std::optional<CompoundRule> parseCompoundRule();
    std::optional<Rule> parseRule();
    std::optional<SoundChange> parseSoundChange();
    bool parseChar(MString& m);
    std::optional<MString> parseString(bool allowEmpty);
  };
}
