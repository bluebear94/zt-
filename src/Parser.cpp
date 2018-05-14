#include "Parser.h"

#include <iostream>

#include "Lexer.h"

namespace sca {
  const Token& Parser::peekToken() {
    while (index >= tokens.size()) {
      std::optional<sca::Token> thuh = l->getNext();
      if (thuh) {
        tokens.push_back(*thuh);
      } else {
        std::cerr << "Lexer error at line " <<
          (l->getLine() + 1) << " column " <<
          (l->getCol() + 1) << "\n";
      }
    }
    return tokens[index];
  }
  const Token& Parser::getToken() {
    const Token& res = peekToken();
    ++index;
    return res;
  }
  std::optional<Operator> Parser::parseOperator(Operator op) {
    const Token& t = getToken();
    if (t.isOperator(op)) return op;
    return std::nullopt;
  }
  std::optional<std::string> Parser::parseString() {
    const Token& t = getToken();
    return t.get<std::string>();
  }
  std::optional<size_t> Parser::parseNumber() {
    const Token& t = getToken();
    return t.get<size_t>();
  }
  #define REQUIRE(x) if (!(x)) return std::nullopt;
  #define REQUIRE_OPERATOR(x) REQUIRE(parseOperator(x))
  #define CHECK_ERROR_CODE(ec) \
    if (ec != ErrorCode::ok) { \
      printError(ec); \
      return std::nullopt; \
    }
  std::optional<std::pair<Feature, PhonemesByFeature>>
  Parser::parseFeature() {
    /*
    feature_def := 'feature' feature_name '{' feature_body* '}'
    feature_body := feature_instance ':' phoneme+ ';'
    */
    REQUIRE_OPERATOR(Operator::kwFeature)
    std::optional<std::string> name = parseString();
    REQUIRE(name)
    REQUIRE_OPERATOR(Operator::lcb)
    Feature f;
    f.featureName = std::move(*name);
    PhonemesByFeature pbf;
    while (true) {
      // Stop when we see a '}'
      const Token& t = peekToken();
      if (t.is<Operator>() && t.as<Operator>() == Operator::rcb) {
        getToken();
        break;
      } else if (t.is<EndOfFile>()) return std::nullopt;
      // Otherwise, parse a line
      std::optional<std::string> iname = parseString();
      REQUIRE(iname)
      f.instanceNames.push_back(std::move(*iname));
      REQUIRE_OPERATOR(Operator::colon)
      // Now for the phoneme names
      pbf.emplace_back();
      auto& pbfi = pbf.back();
      bool atLeastOne = false;
      while (true) {
        // Stop when we see a ';'
        const Token& t = peekToken();
        if (t.isOperator(Operator::semicolon)) {
          getToken();
          if (atLeastOne) break;
          return std::nullopt;
        } else if (t.is<EndOfFile>()) return std::nullopt;
        std::optional<std::string> phoneme = parseString();
        REQUIRE(phoneme)
        pbfi.push_back(std::move(*phoneme));
        atLeastOne = true;
      }
    }
    return std::pair(std::move(f), std::move(pbf));
  }
  std::optional<std::pair<std::string, std::vector<std::string>>>
  Parser::parseCharClass() {
    // class_def := 'class' class '=' phoneme+ ';'
    REQUIRE_OPERATOR(Operator::kwClass)
    std::optional<std::string> name = parseString();
    REQUIRE(name);
    std::vector<std::string> phonemes;
    bool atLeastOne = false;
    while (true) {
      // Stop when we see a ';'
      const Token& t = peekToken();
      if (t.isOperator(Operator::semicolon)) {
        getToken();
        if (atLeastOne) break;
        return std::nullopt;
      } else if (t.is<EndOfFile>()) return std::nullopt;
      std::optional<std::string> phoneme = parseString();
      REQUIRE(phoneme)
      phonemes.push_back(std::move(*phoneme));
      atLeastOne = true;
    }
    return std::pair(std::move(*name), std::move(phonemes));
  }
  std::optional<size_t> Parser::parseMatcherIndex() {
    size_t oldIndex = index;
    if (!parseOperator(Operator::colon)) {
      index = oldIndex;
      return 0;
    }
    return parseNumber();
  }
  std::optional<CharMatcher::Constraint> Parser::parseMatcherConstraint() {
    // class_constraint := feature_name '=' feature_instance
    std::optional<std::string> fname = parseString();
    REQUIRE(fname)
    size_t id; Feature* feature;
    ErrorCode res = sca->getFeatureByName(*fname, id, feature);
    CHECK_ERROR_CODE(res);
    REQUIRE_OPERATOR(Operator::equals)
    std::optional<std::string> iname = parseString();
    REQUIRE(iname)
    size_t iid;
    res = feature->getFeatureInstanceByName(*iname, iid);
    CHECK_ERROR_CODE(res);
    return CharMatcher::Constraint{ id, iid };
  }
  std::optional<CharMatcher> Parser::parseMatcher() {
    // char_matcher := '$(' class [':' int] ['|' class_constraints] ')'
    REQUIRE_OPERATOR(Operator::dlb);
    std::optional<std::string> cname = parseString();
    REQUIRE(cname)
    size_t id;
    CharClass* cclass;
    ErrorCode res = sca->getClassByName(*cname, id, cclass);
    CHECK_ERROR_CODE(res)
    CharMatcher matcher;
    matcher.charClass = id;
    std::optional<size_t> n = parseMatcherIndex();
    REQUIRE(n)
    matcher.index = *n;
    const Token& t = peekToken();
    if (t.isOperator(Operator::pipe)) {
      getToken();
      while (true) {
        auto constraint = parseMatcherConstraint();
        REQUIRE(constraint)
        matcher.constraints.push_back(*constraint);
        const Token& t2 = peekToken();
        if (!t2.isOperator(Operator::comma)) break;
        getToken();
      }
    }
    return matcher;
  }
  std::optional<ErrorCode> Parser::parseStatement() {
    size_t oldIndex = index;
    auto feature = parseFeature();
    if (feature) {
      ErrorCode c = sca->insertFeature(
        std::move(feature->first), feature->second);
      return c;
    }
    index = oldIndex; // backtrack
    auto charClass = parseCharClass();
    if (charClass) {
      ErrorCode c = sca->insertClass(
        std::move(charClass->first), charClass->second);
      return c;
    }
    return std::nullopt;
  }
}
