"""Generate independent C++ normalization fixtures using the existing Python model."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import json
import tomllib
from normalize_conversion import SubSkill, load_curve

root = Path(__file__).resolve().parent.parent
result = {}
for path in sorted((root / "config/skills").glob("*.toml")):
    cfg = tomllib.loads(path.read_text(encoding="utf-8"))
    main = cfg["main"]
    curve = load_curve(main)
    default = load_curve(cfg["sub_curve"])
    subs = [SubSkill(s["name"], s["group"], s["T"], s["w"],
                    s.get("T1", cfg["conversion"]["rise_T1"]),
                    load_curve(s.get("curve", {}), default),
                    fixed_k=s.get("k")) for s in cfg["subskill"]]
    n = curve.xp_between(main["start_level"], main["target_level"])
    budget = {g: curve.xp_between(b["main_from"], b["main_to"])
              for g, b in cfg["budget"].items() if "main_from" in b}
    remaining = n - sum(budget.values())
    total_share = sum(b.get("share", 0) for b in cfg["budget"].values())
    for g, b in cfg["budget"].items():
        if "share" in b:
            budget[g] = remaining * b["share"] / total_share
    ks = {}
    for g, amount in budget.items():
        integral = sum(s.integral(0, cfg["reference_path"]["level"]) for s in subs
                       if s.group == g and s.name in cfg["reference_path"]["members"]
                       and s.fixed_k is None)
        ks[g] = amount / integral
    result[main["actor_value"]] = {"N": n, "k": ks}
(root / "plugin/tests/normalization.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print("Generated normalization fixtures from scripts/normalize_conversion.py for", len(result), "skills")
