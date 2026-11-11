#!/usr/bin/env python3
"""Generate reference worksheets from a LoreRim research dump (Mutagen TSV tables).

    python3 -I scripts/gen_lorerim_refs.py ~/tmp/research refs/lorerim [--force] [--chain ~/tmp/research2]

--chain points at the R-22 dump (tables/perk-override-chain.tsv, tables/avif-perktree-chain.tsv).
With it, every perk entry gets an "Alternatives" subsection (all other plugin versions of the
same record), and every skill gets a list of perks that some tree version had but the final tree
does not.

Writes perks.md, materials.md and weapons-data.md (the data part of weapons.md) into the output
directory. The worksheets are meant to be filled in by hand afterwards, so existing files are NOT
overwritten without --force.

Everything under "Records" and "Proposal (auto)" is derived mechanically from the records. It is
a reading aid, not a verified description of the game's behaviour.
"""
from __future__ import annotations

import argparse
import collections
import csv
import json
import re
import sys
from pathlib import Path

csv.field_size_limit(10**9)

BASE_PLUGINS = {"Skyrim.esm", "Update.esm", "Dawnguard.esm", "HearthFires.esm", "Dragonborn.esm"}

SKILLS = [  # (AVIF editorID, title, short prefix)
    ("AVOneHanded", "One-Handed", "1H"), ("AVTwoHanded", "Two-Handed", "2H"),
    ("AVMarksman", "Archery", "ARC"), ("AVBlock", "Block", "BLK"),
    ("AVHeavyArmor", "Heavy Armor", "HA"), ("AVLightArmor", "Light Armor", "LA"),
    ("AVSmithing", "Smithing", "SMI"), ("AVAlchemy", "Alchemy", "ALC"),
    ("AVEnchanting", "Enchanting", "ENC"), ("AVAlteration", "Alteration", "ALT"),
    ("AVConjuration", "Conjuration", "CON"), ("AVDestruction", "Destruction", "DES"),
    ("AVMysticism", "Illusion (AVIF `AVMysticism`)", "ILL"), ("AVRestoration", "Restoration", "RES"),
    ("AVLockpicking", "Lockpicking", "LCK"), ("AVPickpocket", "Pickpocket", "PCK"),
    ("AVSneak", "Sneak", "SNK"), ("AVSpeechcraft", "Speech", "SPE"),
    ("AVHealRatePowerMod", "Werebeast tree (AVIF `AVHealRatePowerMod`, Growl) — not a main skill", "WER"),
    ("AVMagickaRateMod", "Vampire Lord tree (AVIF `AVMagickaRateMod`, Sacrilege) — not a main skill", "VAM"),
]

EP_WORDS = {
    "ModAttackDamage": "weapon damage", "ModPowerAttackDamage": "power attack damage",
    "ModIncomingDamage": "damage taken", "ModSpellMagnitude": "spell magnitude",
    "ModSpellDuration": "spell duration", "ModSpellCost": "spell cost",
    "ApplyCombatHitSpell": "on weapon hit, apply", "CalculateMyCriticalHitChance": "critical hit chance",
    "ModIncomingSpellMagnitude": "magnitude of spells received", "ModSneakAttackMult": "sneak attack multiplier",
    "CalculateMyCriticalHitDamage": "critical hit damage", "ModPowerAttackStamina": "power attack stamina cost",
    "ModTemperingHealth": "tempering quality", "ModDetectionSneakSkill": "chance to be detected",
    "ModTargetDamageResistance": "target's armor (penetration)", "ModArmorRating": "armor rating",
    "ModEnchantmentPower": "enchantment power", "ApplyBashingSpell": "on bash, apply",
    "ModIncomingStagger": "stagger received", "ModPickpocketChance": "pickpocket chance",
    "ModBashingDamage": "bash damage", "ModPercentBlocked": "share of damage blocked",
    "Activate": "activation", "ModTargetStagger": "stagger dealt", "ModFallingDamage": "falling damage",
    "CanDualCastSpell": "can dual-cast", "CalculateWeaponDamage": "weapon damage (base calc)",
    "ModPoisonDoseCount": "poison doses", "ModAlchemyEffectiveness": "potion/poison strength",
    "ModSellPrices": "sell prices", "ModBuyPrices": "buy prices", "ModArmorWeight": "armor weight",
    "ApplyWeaponSwingSpell": "on weapon swing, apply", "ModSkillUse": "skill XP",
    "ModCommandedActorLimit": "summon limit", "SetBooleanGraphVariable": "animation flag",
    "ModPotionsCreated": "potions created", "ModTelekinesisDamage": "telekinesis damage",
    "ModSpellRange": "spell range", "ApplyReanimateSpell": "on reanimate, apply",
    "ApplySneakingSpell": "while sneaking, apply", "ModRecoverArrowChance": "arrow recovery chance",
    "ModBowZoom": "bow zoom", "ModAttackSpeed": "attack speed", "ModBlockedStamina": "stamina used to block",
    "ModLockpickSweetSpot": "lockpick sweet spot", "ModLockpickCrimeChance": "lockpicking crime chance",
    "ModLockpickKeyRewardChance": "key reward chance", "ModLockpickIsBreakable": "lockpicks unbreakable",
    "ModShoutOK": "shout", "ModPlayerMagicSlowdown": "magic slowdown", "ModSoulGemRecharge": "soul gem recharge",
    "ModPlayerIntimidation": "intimidation", "ModPersuasion": "persuasion", "ModSpellCastingSoundEvent": "spell sound",
    "ModArmorWeightContainer": "armor weight (container)",
}

ITEM_TYPES = {0: "fists", 1: "sword", 2: "dagger", 3: "war axe", 4: "mace", 5: "greatsword",
              6: "battleaxe/warhammer", 7: "bow", 8: "staff", 9: "spell", 10: "shield", 11: "torch",
              12: "crossbow"}

OPS = {"EqualTo": "=", "NotEqualTo": "≠", "GreaterThan": ">", "GreaterThanOrEqualTo": "≥",
       "LessThan": "<", "LessThanOrEqualTo": "≤"}

PREFIXES_TO_SKIP_IN_EFFECT_CONDS = {"GetIsObjectTypeConditionData", "HasPerkConditionData"}


def J(s: str):
    return json.loads(s) if s and s.strip() else []


def read_tsv(path: Path, quote_none: bool = False):
    csv.field_size_limit(1 << 30)
    with open(path, encoding="utf-8") as f:
        if quote_none:
            return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))
        return list(csv.DictReader(f, delimiter="\t"))


def _is_label(d) -> bool:
    return isinstance(d, dict) and "formKey" in d and "labelSource" in d


def to_old_links(x):
    """R-22 tables write links as {formKey, editorID, labelSource}; perk-trees.tsv as 'EditorID [FK]'."""
    if _is_label(x):
        return f"{x.get('editorID') or ''} [{x['formKey']}]".strip() if x.get("editorID") else x["formKey"]
    if isinstance(x, dict):
        return {k: to_old_links(v) for k, v in x.items()}
    if isinstance(x, list):
        return [to_old_links(v) for v in x]
    return x


def sig_norm(x):
    """Comparison form: links by FormKey only; drop raw master Index next to a resolved Link."""
    if _is_label(x):
        return x["formKey"]
    if isinstance(x, dict):
        has_link = "Link" in x and x["Link"] not in (None, "Null")
        return {k: sig_norm(v) for k, v in x.items() if not (has_link and k == "Index")}
    if isinstance(x, list):
        return [sig_norm(v) for v in x]
    return x


def link_fk(cell: str) -> str:
    if not cell or not cell.strip():
        return ""
    try:
        d = json.loads(cell)
    except ValueError:
        return cell
    return d.get("formKey", "") if isinstance(d, dict) else ""


class Version:
    """One plugin's version of a PERK record (R-22 perk-override-chain.tsv)."""

    FIELDS = ("name", "description", "conditions", "effects", "nextPerk", "flags")

    def __init__(self, r: dict):
        self.formKey, self.editorID, self.plugin = r["formKey"], r["editorID"], r["plugin"]
        self.order = int(r["loadOrderIndex"])
        self.winner = r["isWinner"] == "True"
        self.deleted = r.get("isDeleted") == "True"
        self.nextPerk = link_fk(r["nextPerk"])
        raw = {k: (json.loads(r[k]) if r[k].strip() else []) for k in ("conditions", "effects")}
        err = r.get("decodeErrors", "")
        self.error = "" if err in ("", "0", "[]", "False") else err
        self.sig = {
            "name": r["name"], "description": clean_desc(r["description"]),
            "conditions": json.dumps(sig_norm(raw["conditions"]), sort_keys=True),
            "effects": json.dumps(sig_norm(raw["effects"]), sort_keys=True),
            "nextPerk": self.nextPerk,
            "flags": (r["playable"], r["hidden"], r["trait"], r["numRanks"], r["flags"]),
        }
        # row in the format of perk-trees.tsv, for the existing renderers
        self.row = {"formKey": self.formKey, "editorID": self.editorID, "name": r["name"],
                    "winner": self.plugin, "description": r["description"],
                    "conditions": json.dumps(to_old_links(raw["conditions"])),
                    "effects": json.dumps(to_old_links(raw["effects"])), "error": self.error}

    def key(self):
        return tuple(self.sig[f] if f != "flags" else self.sig[f] for f in self.FIELDS)

    def differs(self, other: "Version") -> list[str]:
        return [f for f in self.FIELDS if self.sig[f] != other.sig[f]]


def link_parts(link) -> tuple[str, str]:
    """'EditorID [FK]' or 'FK' -> (editorID or '', formKey)."""
    if isinstance(link, dict):
        link = link.get("Link", "")
    link = link or ""
    m = re.match(r"^(.*?)\s*\[([0-9A-Fa-f]{6}:[^\]]+)\]", link)
    if m:
        return m.group(1).strip(), m.group(2)
    return "", link


def clean_desc(s: str) -> str:
    s = s.replace("<br>", " ").replace("<BR>", " ").replace("\r", " ").replace("\n", " ")
    return re.sub(r"\s+", " ", s).strip()


def pct(v: float) -> str:
    if v == 0:
        return "×0"
    d = (v - 1) * 100
    return f"{'+' if d > 0 else '−'}{abs(d):.4g}%"


def camel_words(s: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", " ", s).lower()


class Ctx:
    def __init__(self, research: Path):
        t = research / "tables"
        self.perks = read_tsv(t / "perk-trees.tsv")
        self.spells = {r["formKey"]: r for r in read_tsv(t / "spell-effects.tsv")}
        self.mgefs = {r["formKey"]: r for r in read_tsv(t / "magic-effects.tsv")}
        self.perk_by_fk = {r["formKey"]: r for r in self.perks}
        self.handles: dict[str, str] = {}
        self.versions: dict[str, list[Version]] = {}
        self.tree_versions: dict[str, dict[str, set[str]]] = {}  # AVIF -> perk FK -> plugins
        self.tree_final: dict[str, set[str]] = {}
        self.tree_labels: dict[str, str] = {}

    def load_chain(self, research2: Path):
        t = research2 / "tables"
        by = collections.defaultdict(list)
        for r in read_tsv(t / "perk-override-chain.tsv", quote_none=True):
            by[r["formKey"]].append(Version(r))
        for fk, vs in by.items():
            vs.sort(key=lambda v: v.order)
        self.versions = dict(by)
        tv = collections.defaultdict(lambda: collections.defaultdict(set))
        fin = collections.defaultdict(set)
        for r in read_tsv(t / "avif-perktree-chain.tsv", quote_none=True):
            fk = link_fk(r["perk"])
            if not fk or fk == "Null":
                continue
            try:
                self.tree_labels.setdefault(fk, json.loads(r["perk"]).get("editorID") or fk)
            except ValueError:
                pass
            tv[r["editorID"]][fk].add(r["plugin"])
            if r["isWinner"] == "True":
                fin[r["editorID"]].add(fk)
        self.tree_versions = {k: dict(v) for k, v in tv.items()}
        self.tree_final = dict(fin)

    def perk_label(self, link) -> str:
        ed, fk = link_parts(link)
        if fk in self.handles:
            return f"{self.perk_by_fk[fk]['name']} ({self.handles[fk]})"
        return ed or fk

    # ---- conditions --------------------------------------------------------------------------
    def cond(self, c: dict) -> str | None:
        d = c["Data"]
        t = d["$type"]
        op = OPS.get(c.get("CompareOperator"), c.get("CompareOperator"))
        v = c.get("ComparisonValue")
        num = isinstance(v, (int, float))
        vs = f"{v:g}" if num else (link_parts(v)[0] or str(v))
        falsy = (op == "=" and v == 0) or (op == "≠" and v == 1)
        neg = "not " if falsy else ""
        kw = lambda: (link_parts(d.get("Keyword"))[0] or link_parts(d.get("Keyword"))[1])
        if t == "HasPerkConditionData":
            return f"{neg}has perk {self.perk_label(d['Perk'])}"
        if t in ("GetBaseActorValueConditionData", "GetActorValueConditionData"):
            return f"{d['ActorValue']} {op} {vs}"
        if t == "GetActorValuePercentConditionData":
            return f"{d['ActorValue']} {op} {float(v) * 100:g}%" if num else f"{d['ActorValue']} % {op} {vs}"
        if t == "GetLevelConditionData":
            return f"character level {op} {vs}"
        if t in ("HasKeywordConditionData", "WornHasKeywordConditionData", "EPMagic_SpellHasKeywordConditionData",
                 "SpellHasKeywordConditionData", "HasMagicEffectKeywordConditionData",
                 "GetCombatTargetHasKeywordConditionData", "EPAlchemyEffectHasKeywordConditionData"):
            what = {"WornHasKeywordConditionData": "wears", "HasMagicEffectKeywordConditionData": "has effect",
                    "EPMagic_SpellHasKeywordConditionData": "spell", "SpellHasKeywordConditionData": "spell",
                    "EPAlchemyEffectHasKeywordConditionData": "potion effect"}.get(t, "")
            return f"{neg}{(what + ' ') if what else ''}{kw()}"
        if t == "WornApparelHasKeywordCountConditionData":
            return f"wears {op} {vs} pieces of {kw()}"
        if t == "EPMagic_SpellHasSkillConditionData":
            return f"{neg}{d['ActorValue']} spell"
        if t == "IsWeaponSkillTypeConditionData":
            return f"{neg}{d['ActorValue']} weapon"
        if t == "SpellHasCastingPerkConditionData":
            ed = link_parts(d["Perk"])[0]
            m = re.search(r"(Novice|Apprentice|Adept|Expert|Master)", ed or "")
            return f"{neg}{m.group(1).lower() if m else ed} spell" if m or ed else None
        if t == "GetIsIDConditionData":
            return f"{neg}is {link_parts(d['Object'])[0] or link_parts(d['Object'])[1]}"
        if t == "GetEquippedItemTypeConditionData":
            it = ITEM_TYPES.get(int(v), vs) if num else vs
            return f"{d.get('ItemSource', '').lower()} hand {op} {it}"
        if t == "GetIsReferenceConditionData" and link_parts(d.get("Target"))[1] == "000014:Skyrim.esm":
            return f"{neg}is the player"
        if t == "HasMagicEffectConditionData":
            return f"{neg}has effect {link_parts(d['MagicEffect'])[0]}"
        if t == "GetRandomPercentConditionData":
            return f"random {op} {vs}%"
        if t == "GetIsRaceConditionData":
            return f"{neg}race {link_parts(d['Race'])[0] or link_parts(d['Race'])[1]}"
        if t == "IsAttackTypeConditionData":
            return f"{neg}attack {kw()}"
        if t == "GetEquippedConditionData":
            return f"{neg}equipped {link_parts(d['ItemOrList'])[0] or link_parts(d['ItemOrList'])[1]}"
        if t == "GetQuestCompletedConditionData" or t == "GetStageConditionData":
            return f"quest condition ({t[3:-13]})"
        name = camel_words(t.replace("ConditionData", "")).replace("get ", "")
        if v in (0, 1) and op in ("=", "≠"):
            return f"{neg}{name}"
        return f"{name} {op} {vs}"

    def conds(self, conds: list, skip=()) -> str:
        out = []
        for c in conds:
            if c["Data"]["$type"] in skip:
                continue
            s = self.cond(c)
            if s:
                out.append((s, "OR" in str(c.get("Flags", ""))))
        if not out:
            return ""
        txt = ""
        for i, (s, is_or) in enumerate(out):
            txt += s
            if i < len(out) - 1:
                txt += " OR " if is_or else " AND "
        return txt

    def effect_conds(self, e: dict) -> str:
        parts = []
        for tab in e.get("Conditions", []):
            s = self.conds(tab["Conditions"], PREFIXES_TO_SKIP_IN_EFFECT_CONDS)
            if s:
                who = {0: "", 1: "item/spell: ", 2: "target: "}.get(tab.get("RunOnTabIndex"), "")
                parts.append(who + s)
        return "; ".join(parts)

    # ---- spells / MGEF -----------------------------------------------------------------------
    def spell_words(self, link) -> str:
        ed, fk = link_parts(link)
        sp = self.spells.get(fk)
        if not sp:
            return ed or fk
        effs = []
        for ef in J(sp["effects"]):
            med, mfk = link_parts(ef.get("BaseEffect"))
            mg = self.mgefs.get(mfk)
            data = ef.get("Data") or {}
            mag, dur = data.get("Magnitude", 0), data.get("Duration", 0)
            if mg:
                desc = clean_desc(mg.get("description", ""))
                if desc:
                    desc = desc.replace("<mag>", f"{mag:g}").replace("<dur>", f"{dur:g}")
                    desc = re.sub(r"<[^>]+>", "", desc)
                    effs.append(desc)
                else:
                    arch = J(mg.get("archetype", ""))
                    av = arch.get("ActorValue") if isinstance(arch, dict) else None
                    at = arch.get("Type") if isinstance(arch, dict) else ""
                    s = mg["name"] or med
                    if av and av != "None":
                        s += f" ({at} {av} {mag:g})"
                    elif mag:
                        s += f" (magnitude {mag:g})"
                    effs.append(s)
            else:
                effs.append(med or mfk)
        name = sp["name"] or ed
        effs = list(dict.fromkeys(e for e in effs if e))
        return f"*{name}*" + (": " + "; ".join(effs[:4]) if effs else "")

    # ---- effects -----------------------------------------------------------------------------
    def effects(self, r: dict) -> list[str]:
        out = []
        for e in J(r["effects"]):
            t = e["$type"]
            c = self.effect_conds(e)
            cs = f" — when {c}" if c else ""
            if t == "PerkEntryPointModifyValue":
                ep = EP_WORDS.get(e["EntryPoint"], camel_words(e["EntryPoint"]))
                m, v = e["Modification"], e.get("Value", 0)
                val = pct(v) if m == "Multiply" else (f"+{v:g}" if m == "Add" else f"set to {v:g}")
                out.append(f"{ep} {val}{cs}")
            elif t == "PerkEntryPointModifyActorValue":
                ep = EP_WORDS.get(e["EntryPoint"], camel_words(e["EntryPoint"]))
                m, v, av = e["Modification"], e.get("Value", 0), e.get("ActorValue")
                if m == "MultiplyOnePlusAVMult":
                    val = f"+{v * 100:.4g}% per point of {av}"
                elif m == "MultiplyAVMult":
                    val = f"× ({av} × {v:g})"
                else:
                    val = f"+ {av} × {v:g}"
                out.append(f"{ep} {val}{cs}")
            elif t == "PerkAbilityEffect":
                out.append(f"ability {self.spell_words(e['Ability'])}")
            elif t in ("PerkEntryPointSelectSpell", "PerkEntryPointAddActivateChoice"):
                ep = EP_WORDS.get(e["EntryPoint"], camel_words(e["EntryPoint"]))
                sp = e.get("Spell")
                lab = f" [button “{e['ButtonLabel']}”]" if e.get("ButtonLabel") else ""
                sw = self.spell_words(sp) if sp and sp != "Null" else "script"
                out.append(f"{ep} {sw}{lab}{cs}")
            elif t == "PerkEntryPointSelectText":
                out.append(f"{EP_WORDS.get(e['EntryPoint'], e['EntryPoint'])} “{e.get('Text')}”{cs}")
            elif t == "PerkQuestEffect":
                out.append(f"scripted (quest {e.get('Quest')} stage {e.get('Stage')})")
            else:
                out.append(t)
        # dedupe, keep order
        seen, res = set(), []
        for s in out:
            if s not in seen:
                seen.add(s)
                res.append(s)
        return res


# ---- sub-skill proposals ------------------------------------------------------------------------
KW_SUB = [
    (r"WeapTypeDagger|\bdaggers?\b", "dagger"), (r"WeapTypeSword\b|\bswords?\b", "sword"),
    (r"WeapTypeWarAxe|war ?axes?", "war-axe"), (r"WeapTypeMace|\bmaces?\b", "mace"),
    (r"WeapTypeGreatsword|greatswords?", "greatsword"), (r"WeapTypeBattleaxe|battleaxes?", "battleaxe"),
    (r"WeapTypeWarhammer|warhammers?", "warhammer"),
    (r"Quarterstaff|QtrStaff|quarterstaves|battlestaves", "quarterstaff (modded)"),
    (r"REQ_WeaponType_BowHeavy|heavy bows?", "heavy-bow"), (r"REQ_WeaponType_BowLight|light bows?", "light-bow"),
    (r"REQ_WeaponType_CrossbowHeavy|heavy crossbows?", "heavy-crossbow"),
    (r"REQ_WeaponType_CrossbowLight|light crossbows?", "light-crossbow"),
    (r"\bcrossbows?\b", "light-crossbow / heavy-crossbow"), (r"\bbows?\b", "light-bow / heavy-bow"),
    (r"dual[- ]wield|both hands|two weapons|offhand|off-hand", "dual-wield"),
    (r"\bunarmed\b|\bfists?\b|hand to hand", "unarmed (Q-WPN-3)"),
    (r"\bparry|parries|parrying", "parry"), (r"\bbash", "bash"),
    (r"heavy shields?", "heavy-shield"), (r"light shields?", "light-shield"),
    (r"\bshields?\b", "heavy-shield / light-shield"),
    (r"MagicDamageFire|\bfire\b|flames?|burn", "destruction/fire"),
    (r"MagicDamageFrost|\bfrost\b|\bcold\b|\bice\b", "destruction/frost"),
    (r"MagicDamageShock|\bshock\b|lightning|electric", "destruction/shock"),
    (r"Destruction_Arcane|\barcane\b", "destruction/arcane"), (r"Entropic|entropy", "destruction/entropic"),
    (r"\bblood\b|hemoman", "destruction/blood"),
    (r"\bcloaks?\b", "form: cloak"), (r"\brunes?\b", "form: rune"), (r"dual[- ]cast", "form: dual-cast"),
    (r"\bwards?\b", "restoration/ward"), (r"heal|restore health", "restoration/healing"),
    (r"turn undead|\bturning\b", "restoration/turning"), (r"\bsun\b|heliom|solar", "restoration/sun"),
    (r"Restoration_Poison|venom", "restoration/poison"),
    (r"BoundWeapon|BoundArmor|\bbound\b", "conjuration/bound"),
    (r"Necro|undead|reanimat|thrall|zombie", "conjuration/necromancy"),
    (r"daedr|atronach", "conjuration/daedric"), (r"spirit|ghost", "conjuration/spirit"),
    (r"soul trap|souls?\b", "conjuration/soul"),
    (r"Transmut", "alteration/transmutation"), (r"Telekin|kinetic", "alteration/kinetomancy"),
    (r"weather|storm", "alteration/weather"), (r"flesh|mage armor|armor spells", "alteration/protection"),
    (r"paraly", "alteration/paralysis"),
    (r"\bcalm|\bfear|frenzy|courage|rally|command", "illusion/emotional"),
    (r"invisib|muffle|silence|perceptual", "illusion/perceptual"),
    (r"\bshadows?\b", "illusion/shadow"),
    (r"\bstaff|staves|\bwands?\b", "wands-staffs"),
    (r"\bprices?\b|merchant|\bbuy|\bsell|barter|vendors?", "speech-trading"),
    (r"persua|intimidat|\bbribe", "speech-persuasion"),
    (r"\bshouts?\b|thu'?um|words of power", "speech-thuum"),
    (r"\bpoisons?\b", "alchemy/poison"), (r"\bpotions?\b", "alchemy/potion"),
    (r"jewel|\brings?\b|necklace|amulet|circlet", "jewelry"),
]
MATERIAL_WORDS = ["iron", "steel", "leather", "hide", "elven", "orcish", "dwarven", "glass", "ebony",
                  "daedric", "dragon", "stalhrim", "nordic", "quicksilver", "bonemold", "chitin", "silver",
                  "imperial", "advanced", "draconic", "moonstone", "malachite", "corundum"]


def propose(skill: str, r: dict, effects_text: str) -> str:
    # priority: keywords in the effect records > perk name > description text
    found = []
    for text in (r["effects"], r["name"], clean_desc(r["description"])):
        for pat, sub in KW_SUB:
            if re.search(pat, text, re.I) and sub not in found:
                found.append(sub)
        if re.search(r"weapons or fists", text, re.I):  # generic "one-handed weapons or fists" wording
            found = [f for f in found if not f.startswith("unarmed")]
        if text is r["effects"] and "IsWeaponSkillType" in text and all(f.startswith("unarmed") for f in found):
            found = []  # "<skill> weapon OR unarmed": the perk covers the whole skill
        if found:
            break
    if not found and "IsWeaponSkillType" in r["effects"]:
        return "whole skill (all its weapon sub-skills)"
    m = re.findall(r"REQ_ArmorSet_(\w+?)(?:Heavy|Light)?\b", r["effects"])
    found += [f"{x.lower()}-armor" for x in dict.fromkeys(m)][:4]
    if skill == "AVSmithing":
        mats = [w for w in MATERIAL_WORDS if re.search(rf"\b{w}\b", r["name"] + " " + r["description"], re.I)]
        found = [f"smithing material: {', '.join(mats)}"] if mats else found
    if skill in ("AVHeavyArmor", "AVLightArmor"):
        found = [f for f in found if f.endswith("-armor")] or found
    # drop proposals that obviously belong to another skill family
    fam = {
        "AVOneHanded": ("dagger", "sword", "war-axe", "mace", "dual-wield", "unarmed", "with-"),
        "AVTwoHanded": ("greatsword", "battleaxe", "warhammer", "quarterstaff", "unarmed"),
        "AVMarksman": ("bow", "crossbow"), "AVBlock": ("shield", "parry", "bash"),
        "AVDestruction": ("destruction", "form"), "AVRestoration": ("restoration", "form"),
        "AVConjuration": ("conjuration", "form"), "AVAlteration": ("alteration", "form"),
        "AVMysticism": ("illusion", "form"), "AVEnchanting": ("wands-staffs", "jewelry", "enchanting"),
        "AVSpeechcraft": ("speech",), "AVAlchemy": ("alchemy",),
        "AVSmithing": ("smithing", "jewelry"),
    }.get(skill)
    if fam:
        found = [f for f in found if any(k in f for k in fam)]
    return ", ".join(found[:4])


def purchase_level(r: dict, skill_av: str) -> int:
    best = 0
    for c in J(r["conditions"]):
        d = c["Data"]
        if d["$type"] == "GetBaseActorValueConditionData" and c.get("CompareOperator", "").startswith("Greater"):
            try:
                best = max(best, int(c["ComparisonValue"]))
            except (TypeError, ValueError):
                pass
    return best


AV_OF = {"AVOneHanded": "OneHanded", "AVTwoHanded": "TwoHanded", "AVMarksman": "Archery"}


def render_body(ctx: Ctx, r: dict, L: list[str]):
    """Requires / In game / Records lines, shared by main entries and alternatives."""
    req = ctx.conds(J(r["conditions"])) or "—"
    L.append(f"- **Requires:** {req}")
    if "character level ≥ 1" in req and "character level < 0" in req:
        L.append("- ⚠ **Not purchasable in the tree** (contradictory level condition): granted by "
                 "reading the matching book (*Requiem Smithing Books Give Perks*)")
    L.append(f"- **In game:** {clean_desc(r['description']) or '—'}")
    if r.get("error"):
        L.append(f"- **Records:** ⚠ effects partially unreadable by Mutagen ({r['error'][:80]}); check in xEdit")
    effs = ctx.effects(r) if not r.get("error") or r["effects"].strip() not in ("", "[]") else []
    if effs:
        if len(effs) == 1:
            L.append(f"- **Records:** {effs[0]}")
        else:
            L.append("- **Records:**")
            for s in effs[:10]:
                L.append(f"  - {s}")
            if len(effs) > 10:
                L.append(f"  - … {len(effs) - 10} more entries")
    elif not r.get("error"):
        L.append("- **Records:** no effects on the perk itself — other records (spells, scripts, "
                 "recipes, conditions elsewhere) check for this perk")
    return effs


FIELD_WORDS = {"name": "name", "description": "description", "conditions": "requirements",
               "effects": "records", "nextPerk": "next rank", "flags": "flags"}


def render_alternatives(ctx: Ctx, fk: str, L: list[str], h: str = "#####"):
    vs = ctx.versions.get(fk)
    if not vs or len(vs) < 2:
        return
    win = next((v for v in vs if v.winner), vs[-1])
    groups: list[list[Version]] = []  # identical versions are merged, first-occurrence order
    for v in vs:
        if v is win:
            continue
        for g in groups:
            if g[0].key() == v.key():
                g.append(v)
                break
        else:
            groups.append([v])
    same = [g for g in groups if not g[0].differs(win)]
    diff = [g for g in groups if g[0].differs(win)]
    L.append(f"{h} Alternatives\n")
    if same:
        L.append("Same as the winner: " + ", ".join(v.plugin for g in same for v in g) + ".\n")
    if not diff:
        return
    for g in diff:
        v = g[0]
        plugins = " = ".join(x.plugin for x in g)
        L.append(f"{h}# {plugins} · {v.sig['name'] or '(no name)'}\n")
        nxt = ""
        if v.nextPerk and v.nextPerk != "Null":
            if v.nextPerk in ctx.handles:
                lbl = ctx.perk_label("[" + v.nextPerk + "]")
            else:
                nv = ctx.versions.get(v.nextPerk)
                lbl = f"`{nv[-1].editorID}`" if nv else f"`{v.nextPerk}`"
            nxt = f" · next rank: {lbl}"
        ed = f"`{v.editorID}`" if v.editorID != win.editorID else "same EditorID"
        L.append(f"- **Version:** {ed}{nxt} · differs from winner in: "
                 + ", ".join(FIELD_WORDS[f] for f in v.differs(win)))
        if v.deleted:
            L.append("- ⚠ **Deleted** in this plugin")
        render_body(ctx, v.row, L)
        L.append("")


def gen_perks(ctx: Ctx, out: Path):
    by_skill = collections.defaultdict(list)
    for r in ctx.perks:
        by_skill[r["skill"]].append(r)
    # handles
    for av, _, pre in SKILLS:
        rows = by_skill.get(av, [])
        rows.sort(key=lambda r: (r["formKey"].split(":")[1] not in BASE_PLUGINS, purchase_level(r, av), r["name"]))
        for i, r in enumerate(rows, 1):
            ctx.handles[r["formKey"]] = f"{pre}-{i:02d}"
    L = []
    L.append("# LoreRim perk worksheet\n")
    L.append("Generated by `scripts/gen_lorerim_refs.py` from the LoreRim research dump of 2026-11-11 "
             "(profile *Default*, `research-report.md`). Only perks that are nodes of a skill tree are "
             "listed (454 of 2 744 PERK records).\n")
    L.append("""**How to read an entry**

- **Handle** (e.g. `1H-03`) — short name for notes; the real identifier is the **FormKey**
  (`localID:DefiningPlugin.esp`), plus EditorID. *Winner* = plugin whose override is active.
- **Origin** — *base game* = the record is defined in Skyrim.esm / Update / DLC. In LoreRim almost
  all of them are reworked (Requiem renames them `REQ_*`, Ordinator / Big Tweaks / Modern Combat
  Rework override them), so "base game" here means "the vanilla slot", not "vanilla behaviour".
  Whether a vanilla perk was *removed* from a tree cannot be seen from this dump (only tree nodes
  were exported).
- **Requires** — purchase conditions of the record (skill level, other perks, character level).
- **In game** — the description the player sees (`<br>` removed).
- **Records** — what the perk's effects actually do, rendered mechanically from the entry points
  and abilities: `weapon damage +20% — when OneHanded weapon` etc. Conditions on the item/spell
  are prefixed `item/spell:`, on the target `target:`. Scripted perks (quest/script effects)
  cannot be explained from records; they are marked *scripted*.
- **Proposal (auto)** — sub-skill this perk seems to belong to, guessed from keywords and text.
  Treat as a hint.
- **Sub-skill requirements** — for you to fill in: which sub-skill and level (current / floor /
  potential, see design Q-DYN-10) should be required to buy the perk; or "automatic perk of
  sub-skill X at level N" if it should leave the tree.
""")
    if ctx.versions:
        L.append("""- **Alternatives** (from the R-22 dump, `perk-override-chain.tsv`) — every other plugin's
  version of the same PERK record, in load order, from the master to the last loser. Versions
  identical to each other are merged (`A = B`); versions identical to the winner are listed in one
  line. *Differs from winner in* compares name, description, requirements, records, next rank and
  flags. Only the PERK record itself is versioned: abilities and spells inside *Records* are shown
  from their **winning** spell records, so an older version may describe a spell that a later mod
  changed (e.g. Requiem's *Penetrating Strikes* says +5% penetration, its ability shows the
  current 30).
- **Removed from the tree** (per skill) — perks that are nodes in some earlier version of the
  skill's AVIF tree but not in the final one, with the winning PERK version and its alternatives.
""")
    notes = Path(__file__).with_name("perks-notes.md")
    if notes.exists():  # hand-written background section, kept in a separate file
        L.append("\n" + notes.read_text(encoding="utf-8"))
    for av, title, pre in SKILLS:
        rows = by_skill.get(av, [])
        if not rows:
            continue
        L.append(f"\n## {title}\n")
        groups = [("Base-game records (vanilla slots, reworked)", [r for r in rows if r["formKey"].split(":")[1] in BASE_PLUGINS]),
                  ("Mod perks", [r for r in rows if r["formKey"].split(":")[1] not in BASE_PLUGINS])]
        for gname, grows in groups:
            if not grows:
                continue
            L.append(f"\n### {title.split(' (')[0].split(' —')[0]} — {gname}\n")
            for r in grows:
                h = ctx.handles[r["formKey"]]
                origin = "base game" if r["formKey"].split(":")[1] in BASE_PLUGINS else r["formKey"].split(":")[1]
                L.append(f"#### {h} · {r['name']}\n")
                L.append(f"- **ID:** `{r['formKey']}` · `{r['editorID']}` · winner: {r['winner']} · origin: {origin}")
                effs = render_body(ctx, r, L)
                p = propose(av, r, " ".join(effs))
                L.append(f"- **Proposal (auto):** {p or '—'}")
                L.append("- **Sub-skill requirements:**\n")
                render_alternatives(ctx, r["formKey"], L)
        render_removed(ctx, av, title, L)
    (out / "perks.md").write_text("\n".join(L) + "\n", encoding="utf-8")


def render_removed(ctx: Ctx, av: str, title: str, L: list[str]):
    tv = ctx.tree_versions.get(av)
    if not tv:
        return
    final = ctx.tree_final.get(av, set())
    removed = [(fk, sorted(pl)) for fk, pl in tv.items() if fk not in final]
    if not removed:
        return
    elsewhere = {fk: a for a, fs in ctx.tree_final.items() for fk in fs}
    L.append(f"\n### {title.split(' (')[0].split(' —')[0]} — Removed from the tree\n")
    L.append("Perks that are nodes in some earlier version of this AVIF tree, but not in the final "
             "one. The entry shows the winning version of the PERK record.\n")
    for fk, plugins in removed:
        vs = ctx.versions.get(fk, [])
        win = next((v for v in vs if v.winner), vs[-1] if vs else None)
        name = win.sig["name"] if win else ctx.tree_labels.get(fk, fk)
        L.append(f"#### {name or ctx.tree_labels.get(fk, fk)}\n")
        where = elsewhere.get(fk)
        note = f" · now in the tree of `{where}`" if where else ""
        if win:
            L.append(f"- **ID:** `{fk}` · `{win.editorID}` · winner: {win.plugin}{note}")
        else:
            L.append(f"- **ID:** `{fk}` · `{ctx.tree_labels.get(fk, '')}`{note}")
        L.append(f"- **In the tree of:** {', '.join(plugins)}")
        if win:
            render_body(ctx, win.row, L)
            L.append("")
            render_alternatives(ctx, fk, L)
        else:
            L.append("- PERK record not exported\n")


# ---- materials -------------------------------------------------------------------------------------
MAT_PROPOSAL = [  # (regex on editor IDs, smithing sub-skill, tier) — tiers on the placeholder scale of scripts/smithing.toml
    (r"Iron(Banded)?\b|_Iron|TreatAsMaterialIron", "iron", "1"),
    (r"SteelPlate", "steel (plate? Q-SMI-5)", "2"), (r"SkyforgeSteel|WAF_WeapMaterialSkyforge", "steel", "1.5"),
    (r"Steel", "steel", "1.5"), (r"NetchLeather", "leather (netch?)", "1"), (r"Leather", "leather", "1"),
    (r"Hide|Fur\b|_Fur|Wolf|Skaal", "hide / leather", "0.8"), (r"Studded", "leather/iron", "1"),
    (r"Scale", "steel (scaled)", "2"), (r"Chainmail", "iron/steel", "1.2"),
    (r"ImperialLegate|ImperialHeavy|ImperialLight|WeapMaterialImperial|Penitus", "steel (imperial)", "1.5"),
    (r"ElvenGilded", "elven", "3.5"), (r"Elven|Linwe|Summerset|Thalmor", "elven", "3"),
    (r"Orcish", "orcish", "3"), (r"Dwarven|Aetherium", "dwarven", "3"),
    (r"Glass", "glass", "5"), (r"Ebony", "ebony", "6"), (r"Daedric", "daedric", "8"),
    (r"Dragonplate|Dragonscale|Dragonbone", "dragon", "10"), (r"Stalhrim", "stalhrim", "7"),
    (r"Quicksilver|Nordic", "nordic / quicksilver", "4"), (r"Bonemold", "bonemold", "2.5"),
    (r"Chitin", "chitin", "3"), (r"Silver", "silver", "2"), (r"Gold", "gold (jewelry?)", "2"),
    (r"Falmer", "falmer (chaurus)", "3"), (r"Draugr|AncientNord", "ancient nord (draugr)", "2"),
    (r"Dawnguard|Hunter", "steel / leather (dawnguard)", "2"), (r"Vampire", "leather / ? (vampire)", "3"),
    (r"Wood", "wood", "0.5"), (r"Amber", "amber (CC)", "6"), (r"Madness", "madness (CC)", "8"),
    (r"Dark\b|_Dark", "dark (CC)", "?"), (r"Golden", "golden (CC)", "?"),
    (r"Forsworn", "hide / bone (forsworn)", "1"), (r"Guard|Stormcloak|Bear", "iron/steel (faction)", "1.5"),
    (r"Akaviri|Blades", "steel (akaviri)", "4"), (r"Nightingale|ThievesGuild|Shrouded|MoragTong|Linwe", "leather (faction)", "3"),
    (r"Vigilant|SilverHand|SpellKnight|Greybeard|Executioner|Alikr|Cultist|Deathbrand|Blackguard",
     "? (faction set: check COBJ)", "?"),
]


def mat_prop(ed: str) -> tuple[str, str]:
    for pat, sub, tier in MAT_PROPOSAL:
        if re.search(pat, ed, re.I):
            return sub, tier
    return "?", "?"


def gen_materials(research: Path, out: Path):
    t = research / "tables"
    usage = read_tsv(t / "keyword-usage.tsv")
    armors = read_tsv(t / "armor-materials.tsv")
    weapons = read_tsv(t / "weapon-types.tsv")
    pat = re.compile(r"ArmorSet|ArmorMaterial|Armou?rMateri|WeapMaterial|WeaponMaterial|MaterialBonemold|CraftingMaterial|TreatAsMaterial|^REQ_Staff_", re.I)
    samples = collections.defaultdict(collections.Counter)
    types = collections.defaultdict(collections.Counter)
    for rows, kind in ((armors, "ARMO"), (weapons, "WEAP")):
        for r in rows:
            for k in r["keywords"].split("; "):
                ed, fk = link_parts(k)
                if fk and r["name"]:
                    samples[fk][r["name"]] += 1
                    types[fk][r["type"]] += 1
    L = ["# LoreRim materials worksheet\n",
         "Generated by `scripts/gen_lorerim_refs.py` from the research dump of 2026-11-11. One row per "
         "material / armor-set keyword actually used on ARMO or WEAP records. Columns *Proposal* are my "
         "guesses; fill in **Smithing sub-skill** and **tier** (category-XP weight, design §7.4).\n",
         """**Notes**

- Reference keywords by **FormKey** (`localID:Plugin`), not EditorID: Requiem renamed most vanilla
  `ArmorMaterial*` keywords to `REQ_ArmorSet_*` (same FormKey). Both names are shown.
- Requiem armor keywords are **set families** (Guard, Vigilant, Shrouded…), not physical materials.
  For Smithing the real material comes from the recipe (COBJ ingredients). **The dump does not include
  COBJ** (new research task R-15), so mappings of faction sets are guesses.
- `REQ_LEGACY_*` and `REQ_NULL_*` are kept by Requiem for compatibility; do not assume they are
  equivalent to the new families.
- *Records* = number of records with the keyword (not how common the item is in play).
- `REQ_Staff_*` keywords are staff materials (staves are Enchanting / `wands-staffs`, not Smithing).
""",
         "| # | Keyword (current EditorID) | Original EditorID | FormKey | Used on | Records | Examples | Proposal: sub-skill | Proposal: tier | **Smithing sub-skill** | **Tier** |",
         "|---|---|---|---|---|---|---|---|---|---|---|"]
    seen = {}
    for u in usage:
        if u["group"] not in ("Armors", "Weapons"):
            continue
        ed, fk = link_parts(u["keyword"])
        if not fk:
            fk = u["keyword"]
        cur = re.sub(r"\s*\{original:.*?\}", "", ed or "")
        orig = u.get("originalEditorID") or ""
        if not pat.search(cur) and not pat.search(orig):
            continue
        e = seen.setdefault(fk, {"cur": cur, "orig": orig, "groups": set(), "count": 0})
        e["groups"].add(u["group"][:-1])
        e["count"] += int(u["count"])
    rows = sorted(seen.items(), key=lambda kv: -kv[1]["count"])
    for i, (fk, e) in enumerate(rows, 1):
        sub, tier = mat_prop(e["cur"] + " " + e["orig"])
        ex = ", ".join(n for n, _ in samples[fk].most_common(3))
        ty = ", ".join(f"{k}" for k, _ in types[fk].most_common(2))
        orig = e["orig"] if e["orig"] and e["orig"] != e["cur"] else ""
        L.append(f"| {i} | `{e['cur']}` | {('`' + orig + '`') if orig else ''} | `{fk}` | {', '.join(sorted(e['groups']))} ({ty}) "
                 f"| {e['count']} | {ex} | {sub} | {tier} |  |  |")
    (out / "materials.md").write_text("\n".join(L) + "\n", encoding="utf-8")


# ---- weapons ---------------------------------------------------------------------------------------
WEAP_PROPOSAL = [
    (r"WeapTypeDagger|Shortsword", "dagger"), (r"Rapier", "sword (or own: rapier)"),
    (r"Katana2H", "greatsword"), (r"Katana", "sword (or own: katana)"),
    (r"WeapTypeSword\b|FancySword|SteelSword|ElvenBlade|IronSword|Pigsticker", "sword"),
    (r"WarAxe", "war-axe"), (r"WeapTypeMace|Whip|Foul", "mace (whip: own?)"),
    (r"Claw", "unarmed? / dagger (claw)"),
    (r"Greatsword|GiantSword", "greatsword"), (r"Battleaxe|Halberd|Scythe|Warscythe", "battleaxe / polearm"),
    (r"Warhammer", "warhammer"), (r"Quarterstaff|QtrStaff", "quarterstaff (own?)"),
    (r"Pike|Pole2H|Spear2H", "polearm (own?)"), (r"Spear1H|Pole1H|Shortspear", "spear (1H, own?)"),
    (r"WeapTypeSpear", "spear (own?)"), (r"Javelin", "thrown (own?)"),
    (r"BowHeavy", "heavy-bow"), (r"BowLight", "light-bow"), (r"CrossbowHeavy", "heavy-crossbow"),
    (r"CrossbowLight|DwarvenCrossbow", "light-crossbow"), (r"WeapTypeBow", "light/heavy bow (by REQ kw)"),
    (r"WeapTypeWand", "wands-staffs (not a weapon)"), (r"WeapTypeStaff", "wands-staffs (not a weapon)"),
    (r"ThrowingKnife", "thrown (own?)"),
]


def gen_weapons(research: Path, out: Path):
    t = research / "tables"
    weapons = read_tsv(t / "weapon-types.tsv")
    pat = re.compile(r"WeapType|WeaponType|OCF_WeapType|ShWeapType|ThrowingKnife|DwarvenCrossbow", re.I)
    info = {}
    for r in weapons:
        for k in r["keywords"].split("; "):
            ed, fk = link_parts(k)
            if not fk or not pat.search(ed or ""):
                continue
            e = info.setdefault(fk, {"ed": ed, "n": 0, "anim": collections.Counter(), "names": collections.Counter()})
            e["n"] += 1
            e["anim"][f"{r['type']}/{r['skill'] or '—'}"] += 1
            if r["name"]:
                e["names"][r["name"]] += 1
    co = collections.Counter()
    for r in weapons:
        kws = [link_parts(k)[0] for k in r["keywords"].split("; ") if pat.search(link_parts(k)[0] or "")]
        if len(kws) > 1:
            co[" + ".join(sorted(set(kws)))] += 1
    L = ["<!-- generated by scripts/gen_lorerim_refs.py; included into weapons.md -->\n",
         "| # | Type keyword (EditorID) | FormKey | Records | Animation type / skill (top) | Examples | Proposal: sub-skill | **Sub-skill** |",
         "|---|---|---|---|---|---|---|---|"]
    for i, (fk, e) in enumerate(sorted(info.items(), key=lambda kv: -kv[1]["n"]), 1):
        prop = next((s for p, s in WEAP_PROPOSAL if re.search(p, e["ed"], re.I)), "?")
        anim = ", ".join(f"{k} ×{v}" for k, v in e["anim"].most_common(3))
        ex = ", ".join(n for n, _ in e["names"].most_common(3))
        L.append(f"| {i} | `{e['ed']}` | `{fk}` | {e['n']} | {anim} | {ex} | {prop} |  |")
    L.append("\n**Most common combinations of type keywords on one record** (the classifier must pick one):\n")
    L.append("| Combination | Records |\n|---|---|")
    for k, v in co.most_common(25):
        L.append(f"| {k} | {v} |")
    (out / "weapons-data.md").write_text("\n".join(L) + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("research", type=Path)
    ap.add_argument("out", type=Path)
    ap.add_argument("--force", action="store_true", help="overwrite existing worksheets")
    ap.add_argument("--chain", type=Path, help="R-22 dump with perk-override-chain.tsv (adds Alternatives)")
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    targets = ["perks.md", "materials.md", "weapons-data.md"]
    if not a.force and any((a.out / n).exists() for n in targets):
        sys.exit(f"refusing to overwrite worksheets in {a.out} (use --force)")
    ctx = Ctx(a.research)
    if a.chain:
        ctx.load_chain(a.chain)
    gen_perks(ctx, a.out)
    gen_materials(a.research, a.out)
    gen_weapons(a.research, a.out)
    print("written:", ", ".join(str(a.out / n) for n in targets))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
