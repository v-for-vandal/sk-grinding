#include "Requirements.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <set>
#include <stdexcept>
namespace sp {
namespace {
// Skill actor values by their CommonLib names (RE::ActorValue), as used in the worksheets.
constexpr std::array<const char*, 18> kMainSkills = {
    "OneHanded",   "TwoHanded", "Archery",     "Block",       "Smithing",   "HeavyArmor",
    "LightArmor",  "Pickpocket", "Lockpicking", "Sneak",      "Alchemy",    "Speech",
    "Alteration",  "Conjuration", "Destruction", "Illusion",  "Restoration", "Enchanting"};
constexpr std::array<const char*, 5> kCounts = {"oneOf", "twoOf", "threeOf", "fourOf",
                                                "fiveOf"};
bool nameChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '/' ||
         c == '*' || c == '.';
}
std::string lower(std::string s) {
  for (auto& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
  return s;
}
struct Parser {
  const std::string& s;
  const NameResolver& names;
  std::vector<std::string>& warnings;
  size_t i = 0;
  [[noreturn]] void fail(const std::string& what) const {
    throw std::runtime_error("requirement \"" + s + "\" at " + std::to_string(i) + ": " + what);
  }
  void skip() {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  }
  bool eat(std::string_view tok) {
    skip();
    if (s.compare(i, tok.size(), tok) != 0) return false;
    i += tok.size();
    return true;
  }
  // AND / OR as whole words, any case.
  bool keyword(const char* word) {
    skip();
    size_t n = std::char_traits<char>::length(word);
    if (i + n > s.size() || lower(s.substr(i, n)) != lower(word)) return false;
    if (i + n < s.size() && nameChar(s[i + n])) return false;
    i += n;
    return true;
  }
  std::string name() {
    skip();
    size_t start = i;
    while (i < s.size() && nameChar(s[i])) ++i;
    if (start == i) fail("name expected");
    return s.substr(start, i - start);
  }
  double number() {
    skip();
    size_t start = i;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) ++i;
    if (start == i) fail("number expected");
    try {
      size_t used = 0;
      double v = std::stod(s.substr(start, i - start), &used);
      if (used != i - start || !std::isfinite(v) || v < 0) fail("bad number");
      return v;
    } catch (const std::logic_error&) {
      fail("bad number");
    }
  }
  // Sub-skill ids a name stands for; empty when it is a main skill.
  std::vector<std::string> expand(const std::string& n) {
    if (auto a = names.aliases.find(n); a != names.aliases.end()) {
      // Alias members are ids or globs (not other aliases).
      std::vector<std::string> out;
      for (auto& m : a->second)
        for (auto& id : expandId(m)) out.push_back(id);
      return out;
    }
    return expandId(n);
  }
  std::vector<std::string> expandId(const std::string& n) {
    if (n.find('*') != std::string::npos) {
      std::vector<std::string> out;
      for (auto& id : names.ids)
        if (globMatch(n, id)) out.push_back(id);
      if (out.empty()) fail("pattern " + n + " matches no sub-skill");
      return out;
    }
    if (std::find(names.ids.begin(), names.ids.end(), n) != names.ids.end()) return {n};
    fail("unknown sub-skill " + n);
  }
  Expr compare() {
    size_t start = i;
    Expr e;
    std::string n = name();
    auto k = std::find(kCounts.begin(), kCounts.end(), n);
    skip();
    if (k != kCounts.end() && i < s.size() && s[i] == '(') {
      ++i;
      e.count = int(k - kCounts.begin()) + 1;
      std::set<std::string> seen;
      do {
        for (auto& id : expand(name()))
          if (seen.insert(id).second) e.subs.push_back(id);
      } while (eat(","));
      if (!eat(")")) fail("')' expected");
      if (int(e.subs.size()) < e.count)
        warnings.push_back("\"" + s + "\": " + n + " over " + std::to_string(e.subs.size()) +
                           " sub-skill(s) can never be met");
    } else if (isMainSkill(n) && !names.aliases.contains(n)) {
      e.main = n;
    } else {
      e.subs = expand(n);
      // A bare alias or glob means "any one of them".
    }
    if (!eat(">=")) fail("'>=' expected");
    e.threshold = number();
    e.text = s.substr(start, i - start);
    while (!e.text.empty() && std::isspace(static_cast<unsigned char>(e.text.front())))
      e.text.erase(e.text.begin());
    return e;
  }
  Expr factor() {
    if (eat("(")) {
      Expr e = expr();
      if (!eat(")")) fail("')' expected");
      return e;
    }
    return compare();
  }
  Expr term() {
    Expr first = factor();
    if (!keyword("AND")) return first;
    Expr e{.kind = Expr::Kind::And};
    e.kids.push_back(std::move(first));
    do e.kids.push_back(factor());
    while (keyword("AND"));
    return e;
  }
  Expr expr() {
    Expr first = term();
    if (!keyword("OR")) return first;
    Expr e{.kind = Expr::Kind::Or};
    e.kids.push_back(std::move(first));
    do e.kids.push_back(term());
    while (keyword("OR"));
    return e;
  }
};
int metCount(const Expr& e, const Values& v, double slack) {
  int n = 0;
  for (auto& id : e.subs) n += v.sub(id) >= e.threshold - slack;
  return n;
}
}  // namespace
bool isMainSkill(const std::string& n) {
  return std::find_if(kMainSkills.begin(), kMainSkills.end(),
                      [&](const char* m) { return n == m; }) != kMainSkills.end();
}
bool globMatch(const std::string& p, const std::string& s) {
  // `*` matches any run of characters (no other wildcards).
  size_t pi = 0, si = 0, star = std::string::npos, mark = 0;
  while (si < s.size()) {
    if (pi < p.size() && p[pi] == '*') {
      star = pi++;
      mark = si;
    } else if (pi < p.size() && p[pi] == s[si]) {
      ++pi;
      ++si;
    } else if (star != std::string::npos) {
      pi = star + 1;
      si = ++mark;
    } else
      return false;
  }
  while (pi < p.size() && p[pi] == '*') ++pi;
  return pi == p.size();
}
Expr parseRequirement(const std::string& text, const NameResolver& names,
                      std::vector<std::string>& warnings) {
  Parser p{text, names, warnings};
  Expr e = p.expr();
  p.skip();
  if (p.i != text.size()) p.fail("unexpected text");
  return e;
}
bool evaluate(const Expr& e, const Values& v, double slack) {
  switch (e.kind) {
    case Expr::Kind::And:
      return std::all_of(e.kids.begin(), e.kids.end(),
                         [&](auto& k) { return evaluate(k, v, slack); });
    case Expr::Kind::Or:
      return std::any_of(e.kids.begin(), e.kids.end(),
                         [&](auto& k) { return evaluate(k, v, slack); });
    case Expr::Kind::Compare:
      if (!e.main.empty()) return v.main(e.main) >= e.threshold - slack;
      return metCount(e, v, slack) >= e.count;
  }
  return false;
}
nlohmann::json explain(const Expr& e, const Values& v, double slack) {
  using nlohmann::json;
  if (e.kind != Expr::Kind::Compare) {
    json out = {{"op", e.kind == Expr::Kind::And ? "and" : "or"},
                {"met", evaluate(e, v, slack)},
                {"terms", json::array()}};
    for (auto& k : e.kids) out["terms"].push_back(explain(k, v, slack));
    return out;
  }
  json out = {{"text", e.text}, {"threshold", e.threshold}, {"met", evaluate(e, v, slack)}};
  if (!e.main.empty()) {
    out["main"] = e.main;
    out["value"] = v.main(e.main);
    return out;
  }
  out["count"] = e.count;
  out["values"] = json::object();
  for (auto& id : e.subs) out["values"][id] = v.sub(id);
  return out;
}
}  // namespace sp
