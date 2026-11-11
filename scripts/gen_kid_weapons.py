#!/usr/bin/env python3
"""Generate the KID config that gives every weapon exactly one weapon-class keyword.

    python3 -I scripts/gen_kid_weapons.py config/kid/SPO_WeaponClass_KID.ini
    python3 -I scripts/gen_kid_weapons.py config/kid/SPO_WeaponClass_KID.ini --check ~/tmp/research2

Design: skills/weapons/design.md, refs/lorerim/weapons.md, refs/frameworks.md §2.

* Only keywords (and one specific form, the Unarmed weapon) are used, never animation types.
* Every source keyword is referenced by FormID~Plugin, never by EditorID: WeapTypeSpear,
  WeapTypeClaw, WeapTypeRapier... exist as different keywords in several plugins.
* Classes are tried in CLASSES order; the first match wins. KID has no "first match", so each
  class line excludes (NOT) every class listed before it. KID sorts keywords topologically by these
  filters, so the classes are distributed in this order, after OCF's own KID files (our filters
  use OCF keywords).
* One line per (class, source plugin): if a plugin is missing, KID drops only the lines that refer
  to it (verify on the LoreRim load order, see --check and the KID log).

--check simulates the rules on the static WEAP dump (weapon-types.tsv). It does NOT see keywords
that OCF and other KID files add at runtime, so the result is a lower bound (R-19).
"""
from __future__ import annotations

import argparse
import collections
import csv
import re
import sys
from pathlib import Path

# ---- source keywords: name -> "FormID~Plugin" (from keyword-aliases.tsv, R-22 dump) -------------
K = {
    # vanilla
    "WeapTypeSword": "0x01E711~Skyrim.esm", "WeapTypeWarAxe": "0x01E712~Skyrim.esm",
    "WeapTypeDagger": "0x01E713~Skyrim.esm", "WeapTypeMace": "0x01E714~Skyrim.esm",
    "WeapTypeBow": "0x01E715~Skyrim.esm", "WeapTypeStaff": "0x01E716~Skyrim.esm",
    "WeapTypeWarhammer": "0x06D930~Skyrim.esm", "WeapTypeGreatsword": "0x06D931~Skyrim.esm",
    "WeapTypeBattleaxe": "0x06D932~Skyrim.esm", "WeapDwarvenCrossbow": "0x015992~Dawnguard.esm",
    # Requiem
    "REQ_BowHeavy": "0x9F9914~Requiem.esp", "REQ_BowLight": "0x9F9915~Requiem.esp",
    "REQ_CrossbowHeavy": "0x899DBE~Requiem.esp", "REQ_CrossbowLight": "0x9F7179~Requiem.esp",
    "REQ_Quarterstaff": "0xADDF81~Requiem.esp",
    "REQ_UnarmedClaw": "0x000A85~Requiem - Weapons and Armor Redone.esp",  # WeapTypeClaw on 'Unarmed'
    # Wand Keywords
    "WeapTypeWand": "0x000800~Wand Keywords.esp",
    # Animated Armoury (NewArmoury.esp)
    "NA_Pike": "0x0E457E~NewArmoury.esp", "NA_Spear": "0x0E457F~NewArmoury.esp",
    "NA_Halberd": "0x0E4580~NewArmoury.esp", "NA_QtrStaff": "0x0E4581~NewArmoury.esp",
    "NA_Rapier": "0x000801~NewArmoury.esp", "NA_Claw": "0x19AAB4~NewArmoury.esp",
    "NA_Cestus": "0x19AAB3~NewArmoury.esp", "NA_Whip": "0x31BD77~NewArmoury.esp",
    "NA_Katana": "0x344733~NewArmoury.esp",
    # Skyrim Spear Mechanic
    "SSM_Spear": "0x000D62~SkyrimSpearMechanic.esp", "SSM_Javelin": "0x00D4C9~SkyrimSpearMechanic.esp",
    "SSM_Pike": "0x0DA156~SkyrimSpearMechanic.esp", "SSM_Halberd": "0x0DA157~SkyrimSpearMechanic.esp",
    "SSM_Rapier": "0x0DA158~SkyrimSpearMechanic.esp", "SSM_Quarterstaff": "0x0DA159~SkyrimSpearMechanic.esp",
    "SSM_Claw": "0x0DA15A~SkyrimSpearMechanic.esp", "SSM_Whip": "0x0DA15B~SkyrimSpearMechanic.esp",
    "SSM_Katana": "0x0DA15C~SkyrimSpearMechanic.esp", "SSM_Scythe": "0x0DA15D~SkyrimSpearMechanic.esp",
    "SSM_Gun": "0x0DA15E~SkyrimSpearMechanic.esp",
    # Smooth Weapon.esm (no static WEAP uses them in LoreRim; kept in case they are distributed)
    "SW_Rapier": "0x000800~Smooth Weapon.esm", "SW_Pike": "0x000801~Smooth Weapon.esm",
    "SW_Halberd": "0x000803~Smooth Weapon.esm", "SW_QtrStaff": "0x000804~Smooth Weapon.esm",
    "SW_Claw": "0x000805~Smooth Weapon.esm", "SW_Scythe": "0x000806~Smooth Weapon.esm",
    "SW_Katana": "0x000807~Smooth Weapon.esm", "SW_Whip": "0x000809~Smooth Weapon.esm",
    "SW_Spear": "0x00080A~Smooth Weapon.esm", "SW_Javelin": "0x00080B~Smooth Weapon.esm",
    "SW_Tdagger": "0x00080C~Smooth Weapon.esm", "SW_Lance": "0x00080D~Smooth Weapon.esm",
    "SW_Gun": "0x000810~Smooth Weapon.esm", "SW_Pistol": "0x00082A~Smooth Weapon.esm",
    "SW_Rifle": "0x00082B~Smooth Weapon.esm", "SW_Minigun": "0x00082C~Smooth Weapon.esm",
    # Noct's Lost Relics
    "NOCT_Spear": "0x000801~Noct_LostRelicsOfTamriel.esp", "NOCT_Quarterstaff": "0x000802~Noct_LostRelicsOfTamriel.esp",
    # CL's Rapier, CC Saints & Seducers
    "CL_Rapier": "0x000800~CL's Rapier.esp",
    "CC_Pickaxe": "0x59F31E~Skyrim Extended Cut - Saints and Seducers.esp",
    # Saints & Seducers types injected into Update.esm (winner: PrvtI_SaintsSeducersArmory.esp)
    "Sh_Shortsword": "0x003285~Update.esm", "Sh_LongMace": "0x003286~Update.esm",
    "Sh_Spear": "0x003287~Update.esm", "Sh_Glaive": "0x003288~Update.esm",
    "Sh_Halberd": "0x003289~Update.esm", "Sh_Shortspear": "0x00328C~Update.esm",
    "Sh_QuarterStaff": "0x00328D~Update.esm",
}
OCF = {  # OCF keywords: OCF.esp, older ones injected into Update.esm
    "WeapCasting": "0x880", "WeapRanged": "0x87F", "WeapUnarmed": "0x82D",
    "WeapSmallBladed1H": "0x887", "WeapBladed1H": "0x884", "WeapEdged1H": "0x886",
    "WeapBlunt1H": "0x885", "WeapPole1H": "0x824",
    "WeapBladed2H": "0x881", "WeapEdged2H": "0x883", "WeapBlunt2H": "0x882", "WeapPole2H": "0x842",
    "Staff1H": "0x863", "Wand1H": "0x888", "StaffBlank": "0x862", "WandBlank": "0x889",
    "Bow": "0x84B", "Bow2H": "0x84C", "Bowblade2H": "0x84D", "Greatbow2H": "0x84E",
    "Longbow2H": "0x84F", "Shortbow2H": "0x850",
    "Crossbow": "0x851", "Crossbow1H": "0x852", "Crossbow2H": "0x853",
    "Gun1H": "0x854", "Gun1H_Basic": "0x855", "Gun1H_Energy": "0x856", "Gun1H_Flamethrower": "0x857",
    "Gun1H_Launcher": "0x858", "Gun1H_Gravity": "0x859", "Gun1H_Shockthrower": "0x85A",
    "Gun1H_Shotgun": "0x85B", "Gun2H": "0x85C", "Gun2H_Basic": "0x85D", "Gun2H_Energy": "0x85E",
    "Gun2H_Launcher": "0x85F", "Gun2H_Shotgun": "0x860", "Blowgun2H": "0x84A", "Slingshot2H": "0x861",
    "Warscythe1H": "0x828", "Warscythe2H": "0x845", "Sickle1H": "0x817",
    "Crescent1H": "0x813", "Crescent2H": "0x839",
    "Halberd1H": "0x827", "Halberd2H": "0x844", "Glaive1H": "0x826", "Glaive2H": "0x843",
    "Lance1H": "0x829", "Lance2H": "0x846", "Pike1H": "0x82A", "Pike2H": "0x847",
    "Spear1H": "0x82B", "Spear2H": "0x848", "Trident1H": "0x82C", "Trident2H": "0x849",
    "Pole1H_Swing": "0x825",
    "Quarterstaff1H": "0x823", "Bladestaff2H": "0x83E",
    "Greatsword2H": "0x867", "Katana2H": "0x82E", "MassiveSword2H": "0x832", "Rapier2H": "0x833",
    "Saber2H": "0x834", "Scimitar2H": "0x835", "CurvedSword2H": "0x865",
    "Lightsaber2H": "0x82F", "Lightsaber2H_1Blade": "0x830", "Lightsaber2H_2Blade": "0x831",
    "Warhammer2H": "0x841", "MassiveHammer2H": "0x87C", "Mace2H": "0x840", "Club2H": "0x83F",
    "Flail2H": "0x87E", "Warpick2H": "0x83C",
    "Battleaxe2H": "0x837", "MassiveAxe2H": "0x87B", "Cleaver2H": "0x838", "Woodaxe2H": "0x83D",
    "Pickaxe2H": "0x83A",
    "Shortsword1H": "0x83B", "Wakizashi1H": "0x822",
    "Dagger1H": "0x802", "Tanto1H": "0x808", "Kunai1H": "0x803", "Sai1H": "0x805", "Shiv1H": "0x806",
    "RevDagger1H": "0x804", "HuntingKnife1H": "0x80A", "ToolKnife1H": "0x809",
    "Sword1H": "0x810", "Saber1H": "0x80E", "Scimitar1H": "0x80F", "CurvedSword1H": "0x864",
    "Lightsaber1H": "0x80B", "Lightsaber1H_1Blade": "0x80C", "Lightsaber1H_2Blade": "0x80D",
    "WarAxe1H": "0x818", "Hatchet1H": "0x815", "WoodHatchet1H": "0x81B", "Woodaxe1H": "0x81A",
    "Pickaxe1H": "0x816", "Cleaver1H": "0x812", "Warpick1H": "0x819",
    "Mace1H": "0x821", "Club1H": "0x81E", "Hammer1H": "0x820", "Flail1H": "0x87D",
    "Cestus1H": "0x81D",
}
for n, lid in OCF.items():
    K["OCF_" + n] = f"{lid}~OCF.esp"
K.update({  # OCF keywords injected into Update.esm
    "OCF_Claw1H": "0x777776~Update.esm", "OCF_Whip1H": "0x777778~Update.esm",
    "OCF_Katana1H": "0x777779~Update.esm", "OCF_Rapier1H": "0x777774~Update.esm",
    "OCF_Javelin1H": "0x777771~Update.esm", "OCF_Scythe2H": "0x777777~Update.esm",
    "OCF_Gun": "0x77777A~Update.esm", "OCF_Quarterstaff2H": "0x777775~Update.esm",
    "OCF_Pole1H_Thrust": "0xCD0100~Update.esm", "OCF_Pole2H_Swing": "0x777773~Update.esm",
    "OCF_Pole2H_Thrust": "0x777772~Update.esm",
})
FORMS = {
    "Unarmed": "0x0001F4~Skyrim.esm",  # the engine's Unarmed weapon record
    # Per-form fixes: weapons whose type keyword contradicts what they are (all NPC/creature
    # weapons, found by --check: class hand != trained skill). A form rule sits in the right
    # class and inherits its NOT list, so it wins over the later, wrong class.
    "GiantClubVolendrung": "0x00080B~The Cursed Tribe - Quest Expansion.esp",  # WeapTypeSword
    "SeaGiantHammerClub": "0x00FB2B~mihailseagiants.esp",
    "SeaGiantBattleaxeClub": "0x00FB2C~mihailseagiants.esp",
    "SeaGiantGreatswordClub": "0x00FB2D~mihailseagiants.esp",
    "FrostGiantClub": "0x012D14~Dawnguard.esm",                  # WeapTypeMace
    "BenthicLurkerWeapon": "0x01E112~Dragonborn.esm",
    "HarbingerBattleaxe": "0x00083B~LoreRim - Magink Bosses.esp",  # WeapTypeWarAxe
    "UndeathStaffOfWorms": "0x0144BB~Undeath.esp",                 # WeapTypeSword on a staff
}

TWO_H_VANILLA = ["WeapTypeGreatsword", "WeapTypeBattleaxe", "WeapTypeWarhammer"]

# ---- classes in priority order -------------------------------------------------------------------
# Each class: (keyword EditorID, sub-skill id, [rules]). A rule is one of:
#   ("any", [names...], [extra NOT names...])   MATCH any of the names
#   ("all", [names...], [extra NOT names...])   ALL of the names
# Rules are split per source plugin automatically for "any".
C = []


def cls(kw, sub, *rules):
    C.append((kw, sub, list(rules)))


def any_(*names, no=()):
    return ("any", list(names), list(no))


def all_(*names, no=()):
    return ("all", list(names), list(no))


# not a weapon: staves and wands feed wands-staffs (skills/enchanting/design.md, wands and staves)
cls("SPO_WeapClass_Casting", "wands-staffs",
    any_("WeapTypeStaff", "WeapTypeWand", "OCF_WeapCasting", "OCF_Staff1H", "OCF_Wand1H",
         "OCF_StaffBlank", "OCF_WandBlank", "FORM:UndeathStaffOfWorms"))
# archery: crossbows before bows (crossbows also carry WeapTypeBow)
cls("SPO_WeapClass_Archery_HeavyCrossbow", "archery-heavy-crossbow", any_("REQ_CrossbowHeavy"))
cls("SPO_WeapClass_Archery_LightCrossbow", "archery-light-crossbow",
    any_("REQ_CrossbowLight", "WeapDwarvenCrossbow", "OCF_Crossbow", "OCF_Crossbow1H", "OCF_Crossbow2H"))
cls("SPO_WeapClass_Archery_HeavyBow", "archery-heavy-bow", any_("REQ_BowHeavy"))
cls("SPO_WeapClass_Archery_LightBow", "archery-light-bow",
    any_("REQ_BowLight", "WeapTypeBow", "OCF_Bow", "OCF_Bow2H", "OCF_Bowblade2H", "OCF_Greatbow2H",
         "OCF_Longbow2H", "OCF_Shortbow2H"))
cls("SPO_WeapClass_Archery_Other", "archery-other",
    any_("SSM_Gun", "SW_Gun", "SW_Pistol", "SW_Rifle", "SW_Minigun", "OCF_Gun", "OCF_Gun1H",
         "OCF_Gun1H_Basic", "OCF_Gun1H_Energy", "OCF_Gun1H_Flamethrower", "OCF_Gun1H_Launcher",
         "OCF_Gun1H_Gravity", "OCF_Gun1H_Shockthrower", "OCF_Gun1H_Shotgun", "OCF_Gun2H",
         "OCF_Gun2H_Basic", "OCF_Gun2H_Energy", "OCF_Gun2H_Launcher", "OCF_Gun2H_Shotgun",
         "OCF_Blowgun2H", "OCF_Slingshot2H", "OCF_WeapRanged"))
# scythes: 2H first; a bare WeapTypeScythe is 2H only together with a vanilla 2H type
cls("SPO_WeapClass_2H_Scythe", "2h-scythe",
    any_("OCF_Scythe2H", "OCF_Warscythe2H", "OCF_Crescent2H"),
    *[all_(s, v) for s in ("SSM_Scythe", "SW_Scythe") for v in TWO_H_VANILLA])
cls("SPO_WeapClass_1H_Scythe", "1h-scythe",
    any_("OCF_Warscythe1H", "OCF_Sickle1H", "OCF_Crescent1H", "SSM_Scythe", "SW_Scythe"))
# polearms: 2H (halberd, pike, glaive, lance, 2H spear, trident); never if OCF says one-handed
cls("SPO_WeapClass_2H_Polearm", "2h-polearm",
    any_("OCF_WeapPole2H", "OCF_Pole2H_Swing", "OCF_Pole2H_Thrust", "OCF_Halberd2H", "OCF_Glaive2H",
         "OCF_Lance2H", "OCF_Pike2H", "OCF_Spear2H", "OCF_Trident2H"),
    any_("NA_Pike", "NA_Halberd", "SSM_Pike", "SSM_Halberd", "SW_Pike", "SW_Halberd", "SW_Lance",
         "NOCT_Spear", "Sh_Halberd", "Sh_Glaive", "Sh_Spear", no=("OCF_WeapPole1H",)))
# 1H spears and other one-handed polearms, javelins
cls("SPO_WeapClass_1H_Spear", "1h-spear",
    any_("OCF_WeapPole1H", "OCF_Pole1H_Thrust", "OCF_Pole1H_Swing", "OCF_Javelin1H", "OCF_Spear1H",
         "OCF_Pike1H", "OCF_Halberd1H", "OCF_Glaive1H", "OCF_Lance1H", "OCF_Trident1H"),
    any_("SSM_Spear", "NA_Spear", "SW_Spear", "SSM_Javelin", "SW_Javelin", "Sh_Shortspear"))
# quarterstaves (they also carry WeapTypeWarhammer); a 1H quarterstaff goes to 1h-other
cls("SPO_WeapClass_2H_Quarterstaff", "2h-quarterstaff",
    any_("OCF_Quarterstaff2H", "OCF_Bladestaff2H"),
    any_("REQ_Quarterstaff", "NA_QtrStaff", "SW_QtrStaff", "SSM_Quarterstaff", "NOCT_Quarterstaff",
         "Sh_QuarterStaff", no=("OCF_Quarterstaff1H",)))
cls("SPO_WeapClass_2H_Sword", "2h-sword",
    any_("WeapTypeGreatsword", "FORM:SeaGiantGreatswordClub", "OCF_Greatsword2H", "OCF_Katana2H", "OCF_MassiveSword2H",
         "OCF_Rapier2H", "OCF_Saber2H", "OCF_Scimitar2H", "OCF_CurvedSword2H", "OCF_Lightsaber2H",
         "OCF_Lightsaber2H_1Blade", "OCF_Lightsaber2H_2Blade"))
cls("SPO_WeapClass_2H_Warhammer", "2h-warhammer",
    any_("WeapTypeWarhammer", "FORM:GiantClubVolendrung", "FORM:SeaGiantHammerClub",
         "FORM:FrostGiantClub", "FORM:BenthicLurkerWeapon", "OCF_Warhammer2H", "OCF_MassiveHammer2H", "OCF_Mace2H", "OCF_Club2H",
         "OCF_Flail2H", "OCF_Warpick2H"))
cls("SPO_WeapClass_2H_Battleaxe", "2h-battleaxe",
    any_("WeapTypeBattleaxe", "FORM:SeaGiantBattleaxeClub", "FORM:HarbingerBattleaxe", "OCF_Battleaxe2H", "OCF_MassiveAxe2H", "OCF_Cleaver2H",
         "OCF_Woodaxe2H", "OCF_Pickaxe2H"))
cls("SPO_WeapClass_2H_Other", "2h-other",
    any_("OCF_WeapBladed2H", "OCF_WeapEdged2H", "OCF_WeapBlunt2H", "OCF_WeapPole2H"))
cls("SPO_WeapClass_1H_ShortSword", "1h-short-sword",
    any_("OCF_Shortsword1H", "OCF_Wakizashi1H", "Sh_Shortsword"))
# daggers and claws; the Requiem 'Unarmed' claw keyword is NOT a claw weapon
cls("SPO_WeapClass_1H_Dagger", "1h-dagger",
    any_("WeapTypeDagger", "OCF_Dagger1H", "OCF_Claw1H", "OCF_Tanto1H", "OCF_Kunai1H", "OCF_Sai1H",
         "OCF_Shiv1H", "OCF_RevDagger1H", "OCF_HuntingKnife1H", "OCF_WeapSmallBladed1H",
         "NA_Claw", "SSM_Claw", "SW_Claw", "SW_Tdagger", no=("REQ_UnarmedClaw",)))
cls("SPO_WeapClass_1H_Sword", "1h-sword",
    any_("WeapTypeSword", "OCF_Sword1H", "OCF_Katana1H", "OCF_Rapier1H", "OCF_Saber1H",
         "OCF_Scimitar1H", "OCF_CurvedSword1H", "OCF_Lightsaber1H", "OCF_Lightsaber1H_1Blade",
         "OCF_Lightsaber1H_2Blade", "NA_Katana", "SSM_Katana", "SW_Katana", "NA_Rapier",
         "SSM_Rapier", "SW_Rapier", "CL_Rapier"))
cls("SPO_WeapClass_1H_WarAxe", "1h-war-axe",
    any_("WeapTypeWarAxe", "OCF_WarAxe1H", "OCF_Hatchet1H", "OCF_WoodHatchet1H", "OCF_Woodaxe1H",
         "OCF_Pickaxe1H", "OCF_Cleaver1H", "OCF_Warpick1H", "CC_Pickaxe"))
cls("SPO_WeapClass_1H_Mace", "1h-mace",
    any_("WeapTypeMace", "OCF_Mace1H", "OCF_Club1H", "OCF_Hammer1H", "OCF_Flail1H", "OCF_Whip1H",
         "NA_Whip", "SSM_Whip", "SW_Whip"))
cls("SPO_WeapClass_1H_Unarmed", "1h-unarmed",
    any_("FORM:Unarmed", "REQ_UnarmedClaw", "OCF_WeapUnarmed", "OCF_Cestus1H", "NA_Cestus"))
cls("SPO_WeapClass_1H_Other", "1h-other",
    any_("OCF_WeapBladed1H", "OCF_WeapEdged1H", "OCF_WeapBlunt1H", "OCF_WeapPole1H",
         "OCF_WeapSmallBladed1H", "OCF_Quarterstaff1H", "Sh_LongMace"))


def ref(name: str) -> str:
    if name.startswith("FORM:"):
        return FORMS[name[5:]]
    return K[name]


def plugin_of(name: str) -> str:
    return ref(name).split("~", 1)[1]


def lines():
    out = []
    earlier: list[str] = []
    for kw, sub, rules in C:
        out.append(f"; ---- {sub}")
        nots = [f"-{e}" for e in earlier]
        for kind, names, extra in rules:
            xnot = [f"-{ref(n)}" for n in extra]
            if kind == "all":
                filt = ["+".join(ref(n) for n in names)]
                out.append(f"; ALL: {' + '.join(names)}")
                out.append(f"Keyword = {kw}|Weapon|{','.join(filt + nots + xnot)}")
                continue
            by_plugin = collections.OrderedDict()
            for n in names:
                by_plugin.setdefault(plugin_of(n), []).append(n)
            for plug, ns in by_plugin.items():
                out.append(f"; {', '.join(ns)}")  # own line: SimpleIni has no inline comments
                out.append(f"Keyword = {kw}|Weapon|{','.join([ref(n) for n in ns] + nots + xnot)}")
        earlier.append(kw)
        out.append("")
    return out


HEADER = """; SPO_WeaponClass_KID.ini — one weapon-class keyword per weapon (generated, do not edit by hand)
; Generator: scripts/gen_kid_weapons.py. Design: skills/weapons/design.md, refs/lorerim/weapons.md.
; Needs KID 3.4+, OCF (its keywords are distributed by OCF's own KID files first).
; Format: Keyword = <ours>|Weapon|<filters>. Plain entries = ANY, -X = NOT, A+B = ALL.
; Classes are listed in priority order; each class excludes every class above it.
; The SPO_WeapClass_* keywords must exist as KYWD records in our ESL.
; A weapon left without a class is handled by the DLL, not here: by its hand it trains
; 1h-other / 2h-other / archery-other (staff: not a weapon). Design Q-WPN-11.
"""


# DLL fallback for weapons without a class keyword (design Q-WPN-11): animation type -> class
FALLBACK = {"OneHandSword": "1h-other", "OneHandDagger": "1h-other", "OneHandAxe": "1h-other",
            "OneHandMace": "1h-other", "HandToHand": "1h-other", "TwoHandSword": "2h-other",
            "TwoHandAxe": "2h-other", "Bow": "archery-other", "Crossbow": "archery-other"}


# ---- static check on the dump ------------------------------------------------------------------
def check(research: Path):
    csv.field_size_limit(1 << 30)
    rows = list(csv.DictReader(open(research / "tables" / "weapon-types.tsv", encoding="utf-8"), delimiter="\t"))
    fk_of = {}
    for n, r in list(K.items()) + [("FORM:" + k, v) for k, v in FORMS.items()]:
        lid, plug = r.split("~", 1)
        fk_of[n] = f"{int(lid, 16):06X}:{plug}"
    res = collections.Counter()
    by_anim = collections.defaultdict(collections.Counter)
    unclassified = collections.Counter()
    fallback = collections.Counter()
    samples = collections.defaultdict(list)
    for w in rows:
        have = set(re.findall(r"\[([0-9A-F]{6}:[^\]]+)\]", w["keywords"]))
        have.add(w["formKey"])
        got = None
        for kw, sub, rules in C:
            for kind, names, extra in rules:
                fks = [fk_of[n] for n in names]
                hit = all(f in have for f in fks) if kind == "all" else any(f in have for f in fks)
                if hit and not any(fk_of[x] in have for x in extra):
                    got = sub
                    break
            if got:
                break
        key = got or "(none)"
        res[key] += 1
        by_anim[key][f"{w['type']}/{w['skill']}"] += 1
        if not got:
            unclassified[f"{w['type']}/{w['skill']}"] += 1
            fallback[FALLBACK.get(w["type"], "(not a weapon)")] += 1
            if len(samples[w["type"]]) < 4:
                samples[w["type"]].append(w["name"] or w["editorID"])
    print(f"## Static check: {len(rows)} WEAP records (static keywords only; OCF runtime keywords not seen)\n")
    print("| class | records | animation / skill (top 3) |\n|---|---|---|")
    for k, n in res.most_common():
        top = ", ".join(f"{a} ×{c}" for a, c in by_anim[k].most_common(3))
        print(f"| {k} | {n} | {top} |")
    print("\nUnclassified by animation:", dict(unclassified.most_common()))
    print("DLL fallback for them:", dict(fallback.most_common()))
    for t, s in samples.items():
        print(f"  {t}: {', '.join(s)}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out", type=Path)
    ap.add_argument("--check", type=Path, help="research dump with tables/weapon-types.tsv")
    a = ap.parse_args()
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(HEADER + "\n" + "\n".join(lines()) + "\n", encoding="utf-8")
    print("written:", a.out)
    if a.check:
        check(a.check)
    return 0


if __name__ == "__main__":
    sys.exit(main())
