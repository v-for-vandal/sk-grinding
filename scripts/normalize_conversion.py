#!/usr/bin/env python3
"""Normalize sub-skill -> main-skill XP conversion (design-v2 §4.4).

Model
-----
* XP cost of one level L -> L+1 (main skill and sub-skills, each with its own parameters):
      cost(L) = improve_mult * L**exponent + improve_offset
* Conversion of sub-skill XP into main XP, as a function of the sub-skill's *peak* level L:
      g(L) = k * rise(L) * fall(L)
      rise(L) = smoothstep(clamp(L / T1, 0, 1)) = x^2 (3 - 2x)   (0 at L = 0, 1 from L = T1)
      fall(L) = (1 + exp(-T/w)) / (1 + exp((L - T)/w))          (normalized logistic, fall(0) = 1)
  T1 (rise) is global ([conversion] rise_T1, default 25) and can be overridden per sub-skill.
  T, w (fall) are per sub-skill; T is where the falloff has halved (unchanged meaning).
  Main XP is produced only by XP that raises the sub-skill's peak (design §4.4), so the total
  main XP a sub-skill can contribute while going from level a to b is
      k * I(a, b),  I(a, b) = sum_{L=a}^{b-1} cost_sub(L) * gn(L + 0.5)
  where gn is g with k = 1.
* Normalization: sub-skills are grouped (e.g. "material", "category", "jewelry"). All
  sub-skills in a group share one k. Each group gets a main-XP budget M_g. Group k is solved so
  that the reference path (listed sub-skills trained 0 -> path level) yields exactly M_g:
      k_g = M_g / sum_{i in path, group(i) = g} I_i(0, path_level)
  Budgets are given either as an absolute main-level range ("main_from"/"main_to") or as a share
  of what remains of N = mainXP(start_level -> target_level) after the absolute budgets.

Usage
-----
    python3 scripts/normalize_conversion.py scripts/smithing.toml
    python3 scripts/normalize_conversion.py scripts/smithing.toml --levels 25 50 75 100 150 200

Only the Python standard library is needed (Python 3.11+ for tomllib).
"""
from __future__ import annotations

import argparse
import math
import sys
import tomllib
from dataclasses import dataclass, field

MAX_LEVEL = 500  # technical hardCap (design §5.1); "lifetime" totals are computed up to it


@dataclass
class Curve:
    improve_mult: float
    improve_offset: float
    exponent: float

    def cost(self, level: int) -> float:
        return self.improve_mult * level ** self.exponent + self.improve_offset

    def xp_between(self, a: int, b: int) -> float:
        return sum(self.cost(L) for L in range(a, b))

    def level_from_xp(self, start: int, xp: float) -> float:
        """Fractional level reached from `start` after spending `xp`."""
        L = start
        while True:
            c = self.cost(L)
            if xp < c:
                return L + xp / c
            xp -= c
            L += 1
            if L > MAX_LEVEL:
                return float(L)


@dataclass
class SubSkill:
    name: str
    group: str
    T: float
    w: float
    T1: float
    curve: Curve
    k: float = 0.0
    tier: float | None = None
    _cache: dict = field(default_factory=dict, repr=False)

    def rise(self, level: float) -> float:
        if self.T1 <= 0:
            return 1.0
        x = min(max(level / self.T1, 0.0), 1.0)
        return x * x * (3.0 - 2.0 * x)

    def fall(self, level: float) -> float:
        num = 1.0 + math.exp(-self.T / self.w)
        x = (level - self.T) / self.w
        if x > 700:  # avoid overflow; contribution is ~0 anyway
            return 0.0
        return num / (1.0 + math.exp(x))

    def gn(self, level: float) -> float:
        return self.rise(level) * self.fall(level)

    def peak_gn(self) -> float:
        return max(self.gn(L / 4) for L in range(0, 4 * MAX_LEVEL))

    def g(self, level: float) -> float:
        return self.k * self.gn(level)

    def integral(self, a: int, b: int) -> float:
        key = (a, b)
        if key not in self._cache:
            self._cache[key] = sum(self.curve.cost(L) * self.gn(L + 0.5) for L in range(a, b))
        return self._cache[key]

    def main_xp(self, a: int, b: int) -> float:
        return self.k * self.integral(a, b)


def load_curve(d: dict, fallback: Curve | None = None) -> Curve:
    if fallback is None:
        return Curve(float(d["improve_mult"]), float(d["improve_offset"]), float(d["exponent"]))
    return Curve(
        float(d.get("improve_mult", fallback.improve_mult)),
        float(d.get("improve_offset", fallback.improve_offset)),
        float(d.get("exponent", fallback.exponent)),
    )


def fmt(x: float, nd: int = 1) -> str:
    if abs(x) >= 1e5:
        return f"{x:,.0f}"
    if abs(x) >= 100:
        return f"{x:,.{nd}f}"
    return f"{x:.{max(nd, 3)}g}" if abs(x) < 1 else f"{x:.{nd + 1}f}"


def table(headers: list[str], rows: list[list[str]]) -> str:
    out = ["| " + " | ".join(headers) + " |", "|" + "|".join("---" for _ in headers) + "|"]
    out += ["| " + " | ".join(r) + " |" for r in rows]
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("config", help="TOML config, see scripts/smithing.toml")
    ap.add_argument("--levels", type=int, nargs="*", default=[10, 25, 50, 75, 100, 125, 150, 200],
                    help="sub-skill levels for the per-level tables")
    args = ap.parse_args()

    with open(args.config, "rb") as f:
        cfg = tomllib.load(f)

    main_cfg = cfg["main"]
    main_curve = load_curve(main_cfg)
    start, target = int(main_cfg["start_level"]), int(main_cfg["target_level"])
    sub_curve_default = load_curve(cfg["sub_curve"])

    rise_T1 = float(cfg.get("conversion", {}).get("rise_T1", 25))
    subs: dict[str, SubSkill] = {}
    for s in cfg["subskill"]:
        subs[s["name"]] = SubSkill(
            name=s["name"], group=s["group"], T=float(s["T"]), w=float(s["w"]),
            T1=float(s.get("T1", rise_T1)),
            curve=load_curve(s.get("curve", {}), sub_curve_default), tier=s.get("tier"),
        )

    path_cfg = cfg["reference_path"]
    path_level = int(path_cfg["level"])
    path = list(path_cfg["members"])
    for m in path:
        if m not in subs:
            sys.exit(f"reference_path member '{m}' is not a defined subskill")

    # ---- budgets ---------------------------------------------------------------------------
    N = main_curve.xp_between(start, target)
    budgets: dict[str, float] = {}
    shares: dict[str, float] = {}
    for g, b in cfg["budget"].items():
        if "main_from" in b:
            budgets[g] = main_curve.xp_between(int(b["main_from"]), int(b["main_to"]))
        else:
            shares[g] = float(b["share"])
    remaining = N - sum(budgets.values())
    if remaining < 0:
        sys.exit("absolute budgets exceed N")
    share_sum = sum(shares.values())
    for g, sh in shares.items():
        budgets[g] = remaining * sh / share_sum

    # ---- solve k per group -------------------------------------------------------------------
    for g, M in budgets.items():
        members = [subs[m] for m in path if subs[m].group == g]
        if not members:
            sys.exit(f"budget group '{g}' has no members in the reference path")
        I = sum(s.integral(0, path_level) for s in members)
        k = M / I
        for s in subs.values():
            if s.group == g:
                s.k = k
    for s in subs.values():
        if s.group not in budgets:
            sys.exit(f"subskill '{s.name}' is in group '{s.group}', which has no budget")

    # ---- report ------------------------------------------------------------------------------
    print(f"# Conversion normalization: {args.config}\n")
    print(f"Main curve: cost(L) = {main_curve.improve_mult}·L^{main_curve.exponent} + "
          f"{main_curve.improve_offset}")
    print(f"N = main XP {start}→{target} = {fmt(N)}\n")

    print("## Group budgets and k\n")
    rows = []
    for g, M in budgets.items():
        k = next(s.k for s in subs.values() if s.group == g)
        rows.append([g, fmt(M), f"{100 * M / N:.1f}%", f"{k:.5g}"])
    print(table(["group", "budget M (main XP)", "share of N", "k"], rows) + "\n")

    print(f"## Sub-skills (path level {path_level})\n")
    rows = []
    for s in subs.values():
        to_path = s.main_xp(0, path_level)
        life = s.main_xp(0, MAX_LEVEL)
        alone = main_curve.level_from_xp(start, to_path)
        rows.append([
            s.name + (" *" if s.name in path else ""), s.group,
            "" if s.tier is None else f"{s.tier:g}", f"{s.T1:g}", f"{s.T:g}", f"{s.w:g}",
            f"{s.peak_gn():.2f}", f"{s.k:.5g}",
            fmt(s.curve.xp_between(0, path_level)), fmt(to_path), f"{start}→{alone:.1f}",
            fmt(life), f"{100 * to_path / life:.0f}%",
        ])
    print(table(["sub-skill (* = path)", "group", "tier", "T1", "T", "w", "peak gn", "k",
                 f"sub XP 0→{path_level}", f"main XP 0→{path_level}",
                 f"main level, alone", "lifetime main XP", f"% of lifetime by {path_level}"], rows))
    print()

    low = [x.name for x in subs.values() if x.peak_gn() < 0.9]
    if low:
        print("**Warning:** peak of gn < 0.9 (T1 too close to T): " + ", ".join(low) + "\n")

    print("## g(L): conversion factor by peak level\n")
    rows = [[s.name] + [f"{s.g(L):.3g}" for L in [0] + args.levels] for s in subs.values()]
    print(table(["sub-skill"] + [f"L={L}" for L in [0] + args.levels], rows) + "\n")

    print("## Cumulative share of lifetime main XP by peak level\n")
    rows = []
    for s in subs.values():
        life = s.main_xp(0, MAX_LEVEL)
        rows.append([s.name] + [f"{100 * s.main_xp(0, L) / life:.0f}%" for L in args.levels])
    print(table(["sub-skill"] + [f"L={L}" for L in args.levels], rows) + "\n")

    print("## Reference path check\n")
    rows = []
    for L in sorted(set([path_level] + args.levels)):
        xp = sum(subs[m].main_xp(0, L) for m in path)
        rows.append([str(L), fmt(xp), f"{main_curve.level_from_xp(start, xp):.1f}"])
    print(f"Path: {', '.join(path)}. All path members trained 0→L at once:\n")
    print(table(["L", "main XP", f"main level (from {start})"], rows) + "\n")

    every = sum(s.main_xp(0, path_level) for s in subs.values())
    print(f"All {len(subs)} defined sub-skills trained 0→{path_level}: main level "
          f"{main_curve.level_from_xp(start, every):.1f}\n")

    # ---- optional crafting pace estimate -----------------------------------------------------
    craft = cfg.get("crafting")
    if craft:
        print("## Crafting pace (material units needed)\n")
        base_mat = float(craft["xp_per_unit_material"])
        base_cat = float(craft["xp_per_unit_category"])
        daily = float(craft.get("daily_bucket_raw_xp", 0))
        mats = [s for s in subs.values() if s.group == craft.get("material_group", "material")]
        cats = [s for s in subs.values() if s.group == craft.get("category_group", "category")]
        rows = []
        for m in mats:
            need = m.curve.xp_between(0, path_level)
            units = need / base_mat
            row = [m.name, f"{m.tier:g}" if m.tier is not None else "", f"{units:,.0f}"]
            if cats and m.tier is not None:
                cneed = cats[0].curve.xp_between(0, path_level)
                row.append(f"{cneed / (base_cat * m.tier):,.0f}")
            else:
                row.append("")
            if daily:
                # one craft event feeds both sub-skills; bucket counts raw XP of the event
                per_unit = base_mat + (base_cat * m.tier if m.tier else 0)
                row.append(f"{need / base_mat * per_unit / daily:.1f}")
            rows.append(row)
        headers = ["material", "tier", f"units: material 0→{path_level}",
                   f"units: one category 0→{path_level} with this material"]
        if daily:
            headers.append(f"active days for material 0→{path_level} (bucket {daily:g})")
        print(table(headers, rows) + "\n")
        print("Raw XP per craft: material sub-skill = xp_per_unit_material × units; "
              "category sub-skill = xp_per_unit_category × tier × units. "
              "Zone multipliers, trainer bonus and DR are ignored.\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
