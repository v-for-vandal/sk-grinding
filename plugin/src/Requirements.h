#pragma once
// Perk requirement formulas (impl/perk-requirements.md): `bash >= 25`,
// `oneOf(heavy-shield, light-shield) >= 50`, `threeOf(1h-*) >= 10 OR twoOf(1h-*) >= 20`,
// `HeavyArmor >= 80 OR (HeavyArmor >= 75 AND speech-intimidation >= 60)`.
// Pure code (no game): parsed at config load, evaluated by Model::updateRequirements.
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace sp {
struct Expr {
  enum class Kind { And, Or, Compare } kind = Kind::Compare;
  std::vector<Expr> kids;  // And / Or
  // Compare: at least `count` of `subs` have current >= threshold; or, when `main` is set,
  // the main skill's level >= threshold.
  int count = 1;
  std::vector<std::string> subs;
  std::string main;
  double threshold = 0;
  std::string text;  // the operand as written, for messages and the panel
};
// Names in a formula: an alias (a line, e.g. `sword-line`), a sub-skill id, a glob over
// sub-skill ids (`1h-*`), or a main skill actor value (`OneHanded`, `HeavyArmor`).
struct NameResolver {
  std::map<std::string, std::vector<std::string>> aliases;
  std::vector<std::string> ids;  // every registered sub-skill id
};
// Throws std::runtime_error with the position on a syntax error or an unknown name.
// `warnings` gets non-fatal notes (a k-of-n term that can never be met).
Expr parseRequirement(const std::string& text, const NameResolver&,
                      std::vector<std::string>& warnings);
struct Values {
  std::function<double(const std::string&)> sub;   // current of a sub-skill
  std::function<double(const std::string&)> main;  // written level of a main skill
};
// `slack` lowers every threshold (hysteresis: a met requirement is kept until it fails at
// threshold − hysteresis).
bool evaluate(const Expr&, const Values&, double slack = 0);
// The formula as a tree for the panel: each comparison with its threshold, the values it
// read and whether it holds.
nlohmann::json explain(const Expr&, const Values&, double slack = 0);
bool isMainSkill(const std::string& name);
bool globMatch(const std::string& pattern, const std::string& id);
}  // namespace sp
