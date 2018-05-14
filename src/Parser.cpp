#include "Parser.h"

#include <iostream>
#include <string_view>

#include "Lexer.h"
#include "utf8.h"

namespace sca {
  static void splitIntoPhonemes(
      const SCA& sca, const std::string_view s, MString& phonemes) {
    size_t ei = s.length();
    if (ei == 0) return;
    const PhonemeSpec* ps = nullptr;
    // Go from the whole string and chop off the last character until
    // we get a match
    std::string st(s);
    while (ei >= 1) {
      ErrorCode res = sca.getPhonemeByName(st, ps);
      if (res == ErrorCode::ok) break;
      st.pop_back();
      --ei;
    }
    if (ps != nullptr) {
      phonemes.push_back(std::move(st));
      splitIntoPhonemes(sca, s.substr(ei), phonemes);
    } else {
      // No match found; just take the first codepoint
      UTF8Iterator<const std::string_view> it(s);
      ++it;
      size_t pos = it.position();
      phonemes.push_back(std::string(s.substr(0, pos)));
      splitIntoPhonemes(sca, s.substr(pos), phonemes);
    }
  }
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
  bool Parser::parseChar(bool allowSpace, MString& m) {
    // char := phonemes | char_matcher
    // (or possibly space ['#'])
    size_t oldIndex = index;
    std::optional<std::string> phonemes = parseString();
    if (phonemes) {
      // handle phonemes case
      splitIntoPhonemes(*sca, *phonemes, m);
      return true;
    }
    index = oldIndex;
    std::optional<CharMatcher> matcher = parseMatcher();
    if (matcher) {
      m.push_back(std::move(*matcher));
    }
    if (allowSpace) {
      if (!parseOperator(Operator::hash)) return false;
      m.push_back(Space());
    }
    return false;
  }
  std::optional<MString> Parser::parseString(bool allowSpace) {
    MString m;
    bool atLeastOne = false;
    while (true) {
      size_t oldIndex = index;
      bool s = parseChar(allowSpace, m);
      if (!s) {
        index = oldIndex;
        if (atLeastOne) break;
        return std::nullopt;
      }
    }
    return std::move(m);
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
