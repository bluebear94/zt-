#include "Parser.h"

#include <algorithm>
#include <iostream>
#include <limits>
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
    if (!reservePhonemes.empty()) return std::nullopt;
    const Token& t = getToken();
    if (t.isOperator(op)) return op;
    return std::nullopt;
  }
  std::optional<Comparison> Parser::parseComparison() {
    const Token& t = getToken();
    if (!t.is<Operator>()) return std::nullopt;
    switch (t.as<Operator>()) {
      case Operator::equals: return Comparison::eq;
      case Operator::notEquals: return Comparison::ne;
      case Operator::lt: return Comparison::lt;
      case Operator::gt: return Comparison::gt;
      case Operator::le: return Comparison::le;
      case Operator::ge: return Comparison::ge;
      default: return std::nullopt;
    }
  }
  std::optional<std::string> Parser::parseString() {
    if (!reservePhonemes.empty()) return std::nullopt;
    const Token& t = getToken();
    return t.get<std::string>();
  }
  std::optional<size_t> Parser::parseNumber() {
    if (!reservePhonemes.empty()) return std::nullopt;
    const Token& t = getToken();
    return t.get<size_t>();
  }
  std::optional<LuaCode> Parser::parseLuaCode() {
    if (!reservePhonemes.empty()) return std::nullopt;
    const Token& t = getToken();
    return t.get<LuaCode>();
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
    const Token& ordered = peekToken();
    if (ordered.isOperator(Operator::kwOrdered)) {
      f.ordered = true;
      getToken();
    } else f.ordered = false;
    REQUIRE_OPERATOR(Operator::lcb)
    f.featureName = std::move(*name);
    PhonemesByFeature pbf;
    size_t definst = -1;
    size_t i = 0;
    while (true) {
      // Stop when we see a '}'
      const Token& t = peekToken();
      if (t.is<Operator>() && t.as<Operator>() == Operator::rcb) {
        getToken();
        f.def = (definst == -1) ? 0 : definst;
        break;
      } else if (t.is<EndOfFile>()) return std::nullopt;
      // Otherwise, parse a line
      std::optional<std::string> iname = parseString();
      REQUIRE(iname)
      f.instanceNames.push_back(std::move(*iname));
      if (const Token& t = peekToken(); t.isOperator(Operator::star)) {
        getToken();
        if (definst == -1) definst = i;
        else return std::nullopt;
      }
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
      ++i;
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
    std::optional<Comparison> cmp = parseComparison();
    REQUIRE(cmp)
    CharMatcher::Constraint c;
    c.feature = id;
    c.c = *cmp;
    bool atLeastOne = false;
    // Read feature values
    while (true) {
      const Token& t = peekToken();
      if (!t.is<std::string>()) {
        if (atLeastOne) return std::move(c);
        return std::nullopt;
      }
      getToken();
      const Token& t2 = peekToken();
      if (t2.isOperator(Operator::colon)) {
        getToken();
        size_t cid; CharClass* cl;
        res = sca->getClassByName(t.as<std::string>(), cid, cl);
        CHECK_ERROR_CODE(res);
        auto n = parseNumber();
        REQUIRE(n)
        c.instances.push_back(std::pair(cid, *n));
      } else {
        size_t iid;
        res = feature->getFeatureInstanceByName(t.as<std::string>(), iid);
        CHECK_ERROR_CODE(res);
        c.instances.push_back(iid);
      }
      atLeastOne = true;
    }
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
  bool Parser::parseCharSimple(MString& m, bool allowSpaces) {
    // char := phonemes | char_matcher
    // (or possibly space ['#'])
    if (!reservePhonemes.empty()) {
      m.push_back(std::move(reservePhonemes.front()));
      reservePhonemes.pop_front();
      return true;
    }
    size_t oldIndex = index;
    std::optional<std::string> phonemes = parseString();
    if (phonemes.has_value()) {
      // handle phonemes case
      splitIntoPhonemes(*sca, *phonemes, reservePhonemes);
      if (reservePhonemes.empty()) return false;
      m.push_back(std::move(reservePhonemes.front()));
      reservePhonemes.pop_front();
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
  std::optional<std::pair<size_t, size_t>> Parser::parseRepeaterInner() {
    constexpr size_t SIZE_T_MAX = std::numeric_limits<size_t>::max();
    std::optional<size_t> m = parseNumber();
    REQUIRE(m)
    const Token& t = peekToken();
    if (!t.isOperator(Operator::comma)) return std::pair(*m, *m);
    getToken();
    const Token& t2 = peekToken();
    if (!t2.is<size_t>()) return std::pair(*m, SIZE_T_MAX);
    size_t n = t2.as<size_t>();
    getToken();
    return std::pair(*m, n);
  }
  std::optional<std::pair<size_t, size_t>> Parser::parseRepeater() {
    constexpr size_t SIZE_T_MAX = std::numeric_limits<size_t>::max();
    size_t oldIndex = index;
    const Token& t = getToken();
    if (!t.is<Operator>()) goto rek;
    switch (t.as<Operator>()) {
      case Operator::star: return std::pair(0, SIZE_T_MAX);
      case Operator::plus: return std::pair(1, SIZE_T_MAX);
      case Operator::question: return std::pair(0, 1);
      case Operator::lcb: {
        auto res = parseRepeaterInner();
        if (!res.has_value()) goto rek;
        const Token& t2 = getToken();
        if (!t2.isOperator(Operator::rcb)) goto rek;
        return res;
      }
      default: {}
    }
    rek:
    index = oldIndex;
    return std::nullopt;
  }
  void Parser::parseAlternation(MString& m, bool allowSpaces) {
    std::vector<MString> opts;
    while (true) {
      MString m2;
      parseStringNoAlt(m2, allowSpaces);
      opts.push_back(std::move(m2));
      const Token& t = peekToken();
      if (!t.isOperator(Operator::pipe)) {
        if (opts.size() == 1) { // Needed beccause ω can't output alternations
          for (auto&& e : opts[0]) m.push_back(std::move(e));
        } else {
          Alternation a = {std::move(opts)};
          m.push_back(std::move(a));
        }
        return;
      }
      getToken();
    }
  }
  bool Parser::parseCharNoRep(MString& m, bool allowSpaces) {
    const Token& t = peekToken();
    if (t.isOperator(Operator::lsb)) {
      getToken();
      parseAlternation(m, allowSpaces);
      if (!parseOperator(Operator::rsb)) return false;
      return true;
    }
    return parseCharSimple(m, allowSpaces);
  }
  bool Parser::parseChar(MString& m, bool allowSpaces) {
    size_t os = m.size();
    bool res = parseCharNoRep(m, allowSpaces);
    if (!res) return false;
    size_t charsRead = m.size() - os;
    auto repeater = parseRepeater();
    if (!repeater.has_value()) return true;
    std::vector<MChar> s(charsRead);
    std::move(m.begin() + os, m.end(), s.begin());
    m.resize(os + 1);
    Repeat r = { std::move(s), repeater->first, repeater->second };
    m.back() = std::move(r);
    return true;
  }
  void Parser::parseStringNoAlt(MString& m, bool allowSpaces) {
    while (true) {
      size_t oldIndex = index;
      bool s = parseChar(m, allowSpaces);
      if (!s) {
        index = oldIndex;
        break;
      }
    }
  }
  std::optional<MString>
  Parser::parseString(bool allowSpaces) {
    MString m;
    parseAlternation(m, allowSpaces);
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
    if (!t->isOperator(Operator::lb)) return false;
    getToken();
    while (true) {
      std::optional<MString> lambda = parseString(true);
      if (!lambda.has_value()) return false;
      if (!parseOperator(Operator::placeholder).has_value()) return false;
      std::optional<MString> rho = parseString(true);
      if (!rho.has_value()) return false;
      r.envs.push_back({std::move(*lambda), std::move(*rho)});
      t = &getToken();
      if (t->isOperator(Operator::rb)) return true;
      if (!t->isOperator(Operator::envOr)) return false;
    }
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
      r->envs.clear();
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
  std::optional<LuaCode> Parser::parseGlobalLuaDecl() {
    REQUIRE_OPERATOR(Operator::kwExecuteOnce);
    return parseLuaCode();
  }
  std::optional<Error> Parser::parseStatement(size_t& which) {
    // In case of failure, return the longest match
    size_t oldIndex = index;
    auto sc = parseSoundChange();
    if (sc.has_value()) {
      sca->insertSoundChange(std::move(*sc));
      return ErrorCode::ok;
    }
    size_t indexSC = index;
    index = oldIndex; // backtrack
    auto feature = parseFeature();
    if (feature.has_value()) {
      Error c = sca->insertFeature(
        std::move(feature->first), feature->second);
      return std::move(c);
    }
    size_t indexFeature = index;
    index = oldIndex; // backtrack
    size_t ccline = l->getLine(), cccol = l->getCol(); // Can't say that name!
    auto charClass = parseCharClass();
    if (charClass.has_value()) {
      Error c = sca->insertClass(
        std::move(charClass->first), charClass->second,
        ccline, cccol);
      return std::move(c);
    }
    size_t indexCC = index;
    index = oldIndex; // backtrack
    auto glDecl = parseGlobalLuaDecl();
    if (glDecl.has_value()) {
      sca->addGlobalLuaCode(*glDecl);
      return ErrorCode::ok;
    }
    size_t indexGLC = index;
    size_t farthest = std::max({indexSC, indexFeature, indexCC, indexGLC});
    // std::max(indexSC, std::max(indexFeature, indexCC));
    if (farthest == indexSC) which = 0;
    else if (farthest == indexFeature) which = 1;
    else if (farthest == indexCC) which = 2;
    else which = 3;
    index = farthest;
    return std::nullopt;
  }
  static const char* things[] = {
    "sound change", "feature definition", "character class definition",
    "global Lua code",
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
