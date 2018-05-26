#pragma once

#include <stddef.h>

#include <deque>
#include <optional>
#include <utility>
#include <vector>

#include "Rule.h"
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
    std::deque<std::string> reservePhonemes;
    size_t index = 0;
    Lexer* l;
    SCA* sca;
    SoundChangeOptions defaultOptions;
    const Token& getToken();
    const Token& peekToken();
    std::optional<Operator> parseOperator(Operator op);
    std::optional<Comparison> parseComparison();
    std::optional<std::string> parseString();
    std::optional<size_t> parseNumber();
    std::optional<LuaCode> parseLuaCode();
    std::optional<std::pair<Feature, PhonemesByFeature>> parseFeature();
    std::optional<std::pair<std::string, std::vector<std::string>>>
    parseCharClass();
    std::optional<size_t> parseMatcherIndex();
    std::optional<CharMatcher::Constraint> parseMatcherConstraint();
    std::optional<CharMatcher> parseMatcher();
    bool parseEnvironment(SimpleRule& r);
    std::optional<std::unique_ptr<SimpleRule>> parseSimpleRule();
    std::optional<std::unique_ptr<CompoundRule>> parseCompoundRule();
    std::optional<std::unique_ptr<Rule>> parseRule();
    bool parseSCOptions(SoundChangeOptions& opt);
    std::optional<SoundChange> parseSoundChange();
    bool parseCharSimple(MString& m, bool allowSpaces);
    void parseAlternation(MString& m, bool allowSpaces);
    bool parseChar(MString& m, bool allowSpaces);
    bool parseCharNoRep(MString& m, bool allowSpaces);
    std::optional<MString> parseString(bool allowSpaces);
    void parseStringNoAlt(MString& m, bool allowSpaces);
    std::optional<std::pair<size_t, size_t>> parseRepeaterInner();
    std::optional<std::pair<size_t, size_t>> parseRepeater();
    std::optional<LuaCode> parseGlobalLuaDecl();
    bool parseSoundChangeOptionChange();
    void printLineColumn();
  };
}
