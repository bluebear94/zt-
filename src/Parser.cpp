#include "Parser.h"

#include <iostream>
#include <string_view>

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
  void Parser::printLineColumn() {
    std::cerr << "  at line " << (peekToken().line + 1) <<
      ", column " << (peekToken().col + 1) << "\n";
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
  #define REQUIRE(x) if (!(x).has_value()) return std::nullopt;
  #define REQUIRE_OPERATOR(x) REQUIRE(parseOperator(x))
  #define CHECK_ERROR_CODE(ec) \
    if (ec != ErrorCode::ok) { \
      printError(ec); \
      printLineColumn(); \
      return std::nullopt; \
    }
  std::optional<std::pair<Feature, PhonemesByFeature>>
  Parser::parseFeature() {
    /*
    feature_def := 'feature' feature_name ['*'] '{' feature_body* '}'
    feature_body := feature_instance ':' phoneme+ ';'
    */
    Feature f;
    f.line = l->getLine();
    f.col = l->getCol();
    REQUIRE_OPERATOR(Operator::kwFeature)
    std::optional<std::string> name = parseString();
    REQUIRE(name)
    const Token& star = peekToken();
    if (star.isOperator(Operator::star)) {
      f.isCore = false;
      getToken();
    } else f.isCore = true;
    REQUIRE_OPERATOR(Operator::lcb)
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
    REQUIRE_OPERATOR(Operator::equals)
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
    std::optional<size_t> n = parseNumber();
    REQUIRE(n)
    if (*n == 0) {
      printError(ErrorCode::explicitLabelZero);
    };
    return n;
  }
  std::optional<CharMatcher::Constraint> Parser::parseMatcherConstraint() {
    // class_constraint := feature_name '=' feature_instance
    std::optional<std::string> fname = parseString();
    REQUIRE(fname)
    size_t id; Feature* feature;
    Error res = sca->getFeatureByName(*fname, id, feature);
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
    Error res = sca->getClassByName(*cname, id, cclass);
    CHECK_ERROR_CODE(res)
    CharMatcher matcher;
    matcher.charClass = id;
    std::optional<size_t> n = parseMatcherIndex();
    REQUIRE(n)
    matcher.index = *n;
    const Token& t = peekToken();
    if (t.isOperator(Operator::pipe)) {
      getToken();
      std::vector<CharMatcher::Constraint> constraints;
      while (true) {
        auto constraint = parseMatcherConstraint();
        REQUIRE(constraint)
        constraints.push_back(*constraint);
        const Token& t2 = peekToken();
        if (!t2.isOperator(Operator::comma)) break;
        getToken();
      }
      matcher.constraints = std::move(constraints);
    } else if (t.isOperator(Operator::slash)) {
      getToken();
      std::vector<const PhonemeSpec*> pses;
      while (true) {
        auto phoneme = parseString();
        REQUIRE(phoneme)
        auto it = sca->findOrInsertPhoneme(*phoneme);
        pses.push_back(&(it->second));
        const Token& t2 = peekToken();
        if (!t2.isOperator(Operator::comma)) break;
        getToken();
      }
      matcher.constraints = std::move(pses);
    }
    REQUIRE_OPERATOR(Operator::rb)
    return matcher;
  }
  bool Parser::parseChar(MString& m, bool allowSpaces) {
    // char := phonemes | char_matcher
    // (or possibly space ['#'])
    size_t oldIndex = index;
    std::optional<std::string> phonemes = parseString();
    if (phonemes.has_value()) {
      // handle phonemes case
      splitIntoPhonemes(*sca, *phonemes, m);
      return true;
    }
    index = oldIndex;
    std::optional<CharMatcher> matcher = parseMatcher();
    if (matcher.has_value()) {
      m.push_back(std::move(*matcher));
      return true;
    }
    if (allowSpaces) {
      index = oldIndex;
      std::optional<Operator> op = parseOperator(Operator::boundary);
      if (op.has_value()) {
        m.push_back(Space());
        return true;
      }
    }
    return false;
  }
  std::optional<MString>
  Parser::parseString(bool allowSpaces) {
    MString m;
    while (true) {
      size_t oldIndex = index;
      bool s = parseChar(m, allowSpaces);
      if (!s) {
        index = oldIndex;
        break;
      }
    }
    return std::move(m);
  }
  bool Parser::parseEnvironment(SimpleRule& r) {
    // env = [['!'] '(' env_string ')']
    const Token* t = &peekToken();
    if (t->isOperator(Operator::bang)) {
      getToken();
      r.inv = true;
      t = &peekToken();
    }
    if (t->isOperator(Operator::lb)) {
      getToken();
      std::optional<MString> lambda = parseString(true);
      if (!lambda.has_value()) return false;
      if (!parseOperator(Operator::placeholder).has_value()) return false;
      std::optional<MString> rho = parseString(true);
      if (!rho.has_value()) return false;
      r.lambda = std::move(*lambda);
      r.rho = std::move(*rho);
      if (!parseOperator(Operator::rb).has_value()) return false;
      return true;
    }
    return false;
  }
  std::optional<std::unique_ptr<SimpleRule>> Parser::parseSimpleRule() {
    // simple_rule := string '->' string env
    std::optional<MString> alpha = parseString(false);
    REQUIRE(alpha)
    REQUIRE_OPERATOR(Operator::arrow)
    std::optional<MString> omega = parseString(false);
    REQUIRE(omega)
    auto r = std::make_unique<SimpleRule>();
    r->alpha = std::move(*alpha);
    r->omega = std::move(*omega);
    size_t oldIndex = index;
    bool res = parseEnvironment(*r);
    if (!res) {
      index = oldIndex;
      r->lambda.clear();
      r->rho.clear();
      r->inv = false;
    }
    return std::move(r);
  }
  std::optional<std::unique_ptr<CompoundRule>> Parser::parseCompoundRule() {
    // compound_rule := '{' (simple_rule ';')* '}'
    REQUIRE_OPERATOR(Operator::lcb);
    auto r = std::make_unique<CompoundRule>();
    while (true) {
      Token t = peekToken();
      if (t.isOperator(Operator::rcb)) {
        getToken();
        return r;
      } else if (t.is<EndOfFile>()) {
        return std::nullopt;
      }
      auto sr = parseSimpleRule();
      REQUIRE(sr)
      r->components.push_back(std::move(**sr));
      REQUIRE_OPERATOR(Operator::semicolon)
    }
  }
  std::optional<std::unique_ptr<Rule>> Parser::parseRule() {
    size_t line = l->getLine(), col = l->getCol();
    size_t oldIndex = index;
    auto sr = parseSimpleRule();
    if (sr) {
      (*sr)->line = line;
      (*sr)->col = col;
      return std::move(sr);
    }
    size_t farthest = index;
    index = oldIndex;
    auto cr = parseCompoundRule();
    if (cr) {
      (*cr)->line = line;
      (*cr)->col = col;
      return std::move(cr);
    }
    index = std::max(farthest, index);
    return std::nullopt;
  }
  std::optional<SoundChange> Parser::parseSoundChange() {
    std::optional<std::unique_ptr<Rule>> r = parseRule();
    REQUIRE(r)
    SoundChange sc;
    sc.rule = std::move(*r);
    const Token* t = &peekToken();
    if (t->isOperator(Operator::slash)) {
      getToken();
      bool atLeastOne = false;
      while (true) {
        const Token& t = peekToken();
        if (!t.is<std::string>()) {
          if (!atLeastOne) return std::nullopt;
          break;
        }
        getToken();
        const std::string& s = t.as<std::string>();
        if (s == "ltr") sc.eo = EvaluationOrder::ltr;
        else if (s == "rtl") sc.eo = EvaluationOrder::rtl;
        else if (s == "once") sc.beh = Behaviour::once;
        else if (s == "loopnsi") sc.beh = Behaviour::loopnsi;
        else if (s == "loopsi") sc.beh = Behaviour::loopsi;
        else {
          std::cerr << s << " is not a valid option\n";
          return std::nullopt;
        }
        atLeastOne = true;
      }
      t = &peekToken();
    }
    if (t->isOperator(Operator::colon)) {
      getToken();
      bool atLeastOne = false;
      while (true) {
        const Token& t = peekToken();
        if (!t.is<std::string>()) {
          if (!atLeastOne) return std::nullopt;
          break;
        }
        getToken();
        const std::string& s = t.as<std::string>();
        sc.poses.insert(s);
        atLeastOne = true;
      }
      t = &peekToken();
    }
    REQUIRE_OPERATOR(Operator::semicolon)
    return std::move(sc);
  }
  std::optional<Error> Parser::parseStatement(size_t& which) {
    // In case of failure, return the longest match
    size_t oldIndex = index;
    auto sc = parseSoundChange();
    if (sc) {
      sca->insertSoundChange(std::move(*sc));
      return ErrorCode::ok;
    }
    size_t indexSC = index;
    index = oldIndex; // backtrack
    auto feature = parseFeature();
    if (feature) {
      Error c = sca->insertFeature(
        std::move(feature->first), feature->second);
      return std::move(c);
    }
    size_t indexFeature = index;
    index = oldIndex; // backtrack
    size_t ccline = l->getLine(), cccol = l->getCol(); // Can't say that name!
    auto charClass = parseCharClass();
    if (charClass) {
      Error c = sca->insertClass(
        std::move(charClass->first), charClass->second,
        ccline, cccol);
      return std::move(c);
    }
    size_t indexCC = index;
    size_t farthest = std::max(indexSC, std::max(indexFeature, indexCC));
    if (farthest == indexSC) which = 0;
    else if (farthest == indexFeature) which = 1;
    else which = 2;
    index = farthest;
    return std::nullopt;
  }
  static const char* things[] = {
    "sound change", "feature definition", "character class definition"
  };
  bool Parser::parse() {
    bool ok = true;
    while (!peekToken().is<EndOfFile>()) {
      size_t which;
      auto res = parseStatement(which);
      if (!res) {
        ok = false;
        std::cerr << "Parse error when trying to parse ";
        std::cerr << things[which];
        std::cerr << ":\n";
        printLineColumn();
        while (true) {
          const Token& t = getToken();
          if (t.isOperator(Operator::semicolon) || t.is<EndOfFile>())
            break;
        }
      }
      else if (*res != ErrorCode::ok) {
        ok = false;
        printError(*res);
        printLineColumn();
      }
    }
    return ok;
  }
}
