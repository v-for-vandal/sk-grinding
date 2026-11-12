# Frameworks and libraries

Reference note for [`design-v2.md`](../design-v2.md) §14 (technical notes) and research tasks
R-18…R-21 (§15.2); §6: PrismaUI (§12.1).

**Status.** This note comes from web research done on 2026-11-11, mostly from the authors'
GitHub repositories, wikis and source code. The Nexus pages returned HTTP 403, so nothing was read
from them directly. Two kinds of claim need confirmation:

- *(unconfirmed)*: the source does not say it explicitly; it is an inference from code or from a
  summary.
- *(verify)*: needs a test on the Windows machine with the LoreRim profile.

Versions installed in LoreRim come from the LoreRim profile (design §1). The research mostly reads
the **current** versions, which can be newer.

The OCF keyword FormIDs found in the research match our static dump
([`lorerim/weapons.md`](lorerim/weapons.md)). For example, `OCF_WeapTypeSpear1H` is `0x82B~OCF.esp`
and `OCF_WeapTypePole1H_Thrust` is `0xCD0100~Update.esm`.

## 1. Summary: what matters for the mod

| # | Finding | Effect on the design |
|---|---|---|
| F-1 | OCF already classifies modded weapons, and it does so **through KID at runtime** | The weapon classifier uses OCF keywords as its main input. The static dump undercounts coverage (R-19) |
| F-2 | KID 3.4 supports **chaining** (a filter can use a keyword that KID itself distributes) and **NOT filters** (`-Keyword`), sorted by dependency | The classifier can be one `_KID.ini` of our own: priorities are written as exclusions. The DLL checks only our keywords (§2.3) |
| F-3 | KID 3.4 has **no exclusive groups** (they arrived in 4.0) and no priority field | Mutual exclusion is written by hand, as `-OtherClass` chains |
| F-4 | OCF also adds **vanilla** keywords (`WeapTypeSword`, `WeapTypeGreatsword`) to modded weapons | Vanilla, Requiem and Ordinator perks that filter by `WeapType*` already apply to modded weapons. Keep this in mind for the perk worksheet |
| F-5 | Keywords are distributed in KID's `kDataLoaded` handler. KID then sends the mod event `KID_KeywordDistributionDone` | Build the classification cache after that event, or lazily on load game, never during our own `kDataLoaded` |
| F-6 | AVG adds new actor values. ESP records refer to them through **aliases** of placeholder AVs. Its C++ API has `RegisterAVDelegate` for **functional** values computed by a plugin | A possible way for perk conditions and entry points to read sub-skills, with the DLL as the only source of values (§3.4). Not confirmed for 2.2.9.4 on AE (R-18) |
| F-7 | alandtse/CommonLibSSE-NG (branch `ng`) is the maintained fork. Its license is now **GPL-3.0-or-later with a modding exception** (it used to be MIT) | License of our DLL: Q-TECH-2 |
| F-8 | CLibUtil is header-only and contains the helpers KID and SPID use: config discovery, string utilities, RNG, SimpleINI | Optional dependency. Its `distribution` and `string` helpers suit our config loader |

## 2. Keyword Item Distributor (KID)

LoreRim has **3.4.0**. The facts below come from the `v3.4.0` tag of
https://github.com/powerof3/Keyword-Item-Distributor unless marked otherwise. The latest version
is 4.1.0.

### 2.1 Format

```
Keyword = <keyword formID~esp | keyword EditorID> | <type> | <filters> | <traits> | <chance>
```

- **Files.** KID reads every `Data\*_KID.ini` in alphabetical path order. Only the unnamed
  top-level section is read, and repeated keys are allowed.
- **Types.** Armor, Weapon, Ammo, Magic Effect, Potion, Scroll, Location, Ingredient, Book,
  Misc Item, Key, Soul Gem, Spell, Activator, Flora, Furniture, Race, Talking Activator,
  Enchantment.
- **Filters** are separated by commas. Each entry has one of four kinds:

  | Form | Meaning |
  |---|---|
  | `A+B` | ALL: every part must match |
  | `-A` | NOT: exclude |
  | `*A` | ANY: substring of the EditorID, name, model path or keyword EditorID |
  | `A` | MATCH: at least one plain entry must match |

  - Plain strings are compared exactly, case-insensitive, against the EditorID, the display
    name, AV names and `.nif` paths.
  - Form filters include keywords (the item's own, or those of its enchantment or costliest
    effect), FormLists (recursive), specific forms, and a plugin file (every form in that
    mod).
- **Order of checks for each item:**
  1. skip the item if it already has the keyword;
  2. roll the chance (deterministic per keyword and item);
  3. check ALL, then NOT, then MATCH, then ANY, then traits.
- **Weapon traits:**
  - `W(min max)` weight, `D(min max)` damage;
  - animation type: `OneHandSword`, `OneHandDagger`, `OneHandAxe`, `OneHandMace`,
    `TwoHandSword`, `TwoHandAxe`, `Bow`, `Staff`, `Crossbow`, `HandToHandMelee`;
  - `E`/`-E` enchanted, `T`/`-T` templated.
- **Armor traits:** `AR(..)`, `W(..)`, slot number, `HEAVY`/`LIGHT`/`CLOTHING`, `E`, `T`.
- **Missing keyword.** If the keyword EditorID does not exist, 3.4 creates the keyword at runtime.
  The current master logs an error instead. A keyword we want to rely on should still be a real
  record in our ESL.

### 2.2 Chaining and ordering

- KID builds a dependency graph from all filter kinds (ALL, NOT, MATCH, ANY) and distributes in
  topological order: a keyword is distributed before the keywords that filter on it.
- A cycle is broken by dropping the edge, with a WARN in the KID log. When there is no
  dependency, the order follows the config files, then EditorID.
- Inside one form type, a keyword just added is visible to the dependent keywords in the same pass.
- Chaining **across** form types (for example a spell filtered on a magic-effect keyword) has an
  order that is *(unconfirmed)*.

### 2.3 Use for our classifier

The priority rules in [`lorerim/weapons.md`](lorerim/weapons.md) can be written as a chain of
exclusions. Most specific first, then each broader class excludes the more specific ones:

```ini
; SPO_Weapons_KID.ini (sketch; names and rules are placeholders)
Keyword = SPO_WeapClass_Polearm|Weapon|OCF_WeapPole1H,OCF_WeapPole2H
Keyword = SPO_WeapClass_Katana|Weapon|OCF_WeapTypeKatana1H,OCF_WeapTypeKatana2H,-SPO_WeapClass_Polearm
Keyword = SPO_WeapClass_Sword|Weapon|WeapTypeSword,-SPO_WeapClass_Polearm,-SPO_WeapClass_Katana
Keyword = SPO_WeapClass_Greatsword|Weapon|WeapTypeGreatsword,-SPO_WeapClass_Polearm,-SPO_WeapClass_Katana
; fallback by animation type, for weapons without any type keyword
Keyword = SPO_WeapClass_Sword|Weapon|-SPO_WeapClass_Polearm,-SPO_WeapClass_Katana|OneHandSword
```

- The DLL then checks only `SPO_WeapClass_*`, one `HasKeyword` per class, and caches the result
  per base form.
- A new weapon mod means a new `_KID.ini` line, not C++ code. This replaces the TOML rule config
  sketched in `weapons.md`, or that config can be generated into this INI.
- Diagnostics: KID writes a log of how many items received each keyword. The DLL logs items that
  have no `SPO_WeapClass_*` keyword.
- *(verify)* The exact ALL/NOT/MATCH combinations (the summary did not show the comma
  delimiter in the code).
- *(verify)* The rule order produces one class per weapon on the LoreRim load order.

## 3. Actor Value Generator (AVG)

LoreRim has **2.2.9.4**. The research read the current wiki and the `main` branch
(https://github.com/NoahBoddie/actor-value-generator). Secondary sources give 2.3.6.0 as the
current version, and it needs LexiconSKSE from 2.0 on. The wiki itself is marked "(Unfinished)".

### 3.1 What it does

AVG adds **extra values**, which are treated like actor values but get their index at runtime.
There are three types:

| Type | Stored per actor | Value comes from |
|---|---|---|
| Adaptive | yes (like normal AVs: set/mod, regen, skill data) | stored |
| Functional | no | formulas or **functions registered by a plugin** |
| Exclusive | adaptive on the player, functional on NPCs | — (older docs say "not implemented") |

### 3.2 Config

- TOML files in `Data/SKSE/Lexicon/Resources/AVG`. The legacy path is
  `Data/SKSE/Plugins/ActorValueData/*_AVG.toml`, with different rules.
- Which path 2.2.9.4 uses is *(unconfirmed)*.
- A value is a table: `[MyValue]` with `type = "Adaptive" | "Functional" | "Exclusive"`, plus
  optionally `alias` and `plugins`, or an `Include` table.

### 3.3 How ESP records refer to an extra value: aliases

- An ESP record keeps using a placeholder vanilla AV, for example `Variable01`.
- At data load, AVG replaces that AV with the extra value in the listed plugins (or the forms
  selected by `Include`: Plugin < Keyword < EditorID < FormList < exact form, from least to most
  specific). With KID installed, includes wait for keyword distribution.
- So **one placeholder AV can mean a different extra value in each plugin, or in each perk**. This
  removes the "only ~10 spare AVs" limit of option 1 in
  [`dynamic-skill-effects.md`](dynamic-skill-effects.md) §2.

### 3.4 C++ API (`include/API_ActorValueGenerator.h`, InterfaceVersion3)

- Obtained with `GetModuleHandle("ActorValueGenerator.dll")` and
  `GetProcAddress("AVG_RequestInterfaceImpl")`. Safe at PostLoad.
- Main functions:
  - `ResolveExtraValue(RE::ActorValue)`;
  - `RegisterForActorValueChange(callback)`;
  - **`RegisterAVDelegate(name, get, set)`**;
  - `ProcessFormAliases(form)`.
- Reading and writing an extra value from a plugin: look up its ID by name, then use the normal
  `ActorValueOwner` calls. AVG hooks `GetActorValue`, `GetBaseActorValue`, `SetActorValue`,
  `ModActorValue` and friends on `Character` and `PlayerCharacter` *(unconfirmed that this is the
  intended pattern)*.
- Persistence: adaptive values persist like normal AVs. AVG has its own serialization manager;
  whether it uses the SKSE co-save is *(unconfirmed)*.

### 3.5 Idea: sub-skills as functional extra values

- Each sub-skill that perks need to read becomes a **functional** extra value whose getter is a
  delegate in our DLL (`RegisterAVDelegate`). AVG stores nothing: the DLL stays the only source of
  sub-skill values.
- CK perks then use a placeholder AV, aliased to `SPO_Sub_<id>`, in:
  - conditions: `GetActorValue Variable01 >= 70` becomes "smithing-dwarven ≥ 70";
  - entry points: `Multiply 1 + Actor Value Mult`, which gives continuous effects
    ([`dynamic-skill-effects.md`](dynamic-skill-effects.md) §1).
- This would solve Q-DYN-9 and Q-DYN-13 (the mirror into globals in
  [`perk-requirements.md`](perk-requirements.md)) with plain data. It would also let other mods
  read our sub-skills.

**Risks, all for R-18:**
- No source says explicitly that extra values work in perk conditions, entry points, magic-effect
  conditions or spell magnitudes. The code suggests remapping of entry-point AV fields
  (`BGSEntryPointFunctionDataTwoValue.h`) *(unconfirmed)*.
- Some AE addresses in the `main` source are placeholders (`000000`). Which hooks this affects
  on AE 1.6.1170 and in 2.2.9.4 is unknown.
- The console (`GetActorValueInfo`) and SKSE string-to-AV functions are known not to work fully
  with extra values.
- One more hard dependency, and an "unfinished" wiki.

**Fallback** if R-18 fails: the current proposals (globals for requirements, DLL hooks for
numeric bonuses).

## 4. Object Categorization Framework (OCF)

LoreRim has the build dated 2025-12-21. The latest release is 6.1.0; LoreRim's build is probably
6.0.x *(unconfirmed)*. Repo: https://github.com/GroundAura/Object-Categorization-Framework.
Keyword list: wiki "Keyword Reference".

- **Purpose.** OCF categorizes items from many mods and exposes the categories as keywords and
  FormLists for other mods to use as conditions. It is designed as a shared resource.
- **How keywords are applied:**
  - The keyword records are in `OCF.esp`. Some older keywords keep their `Update.esm` FormIDs.
  - KID INIs per record type distribute them (`ABA_KW-OCF_WEAP_KID.ini`, …), plus SPID and FLM
    files.
  - The weapon rules match by exact EditorID, name or EditorID substring, `.nif` path, specific
    forms and animation trait, with `OCF_varWeapNot*` exclusions.
- **Coverage seen** in the weapon INI: vanilla, CC, Heavy Armory, Immersive Weapons, Royal
  Armory, Legacy of the Dragonborn, Beyond Skyrim, and about 150 more plugins. **Animated Armoury
  (`NewArmoury.esp`) was not found**: it may be covered by a separate patch ("Keyword Patch
  Collection", Nexus 92529) *(verify what LoreRim has)*.
- **Weapon keyword tree** (current; abbreviated):

  | Group keyword | Subtypes (`OCF_WeapType*`) |
  |---|---|
  | `OCF_WeapSmallBladed1H` | Dagger, Claw, Tanto, Sai, Kunai, Shiv, … |
  | `OCF_WeapBladed1H` | Sword1H, Katana1H, Rapier1H, Saber1H, Scimitar1H, … |
  | `OCF_WeapEdged1H` | WarAxe, Hatchet, Pickaxe, Sickle, Warpick, … |
  | `OCF_WeapBlunt1H` | Mace, Club, Flail, Hammer, Whip1H, Quarterstaff1H, … |
  | `OCF_WeapPole1H` | Javelin1H; `_Pole1H_Swing` (Glaive, Halberd, Warscythe); `_Pole1H_Thrust` (Lance, Pike, Spear1H, Trident) |
  | `OCF_WeapUnarmed` | `_Basic`, `_Claws` (6.1.0) |
  | `OCF_WeapBladed2H` | Greatsword2H, Katana2H, … |
  | `OCF_WeapEdged2H` | Battleaxe2H, Scythe2H, … |
  | `OCF_WeapBlunt2H` | Warhammer2H, Quarterstaff2H, … |
  | `OCF_WeapPole2H` | `_Pole2H_Swing`, `_Pole2H_Thrust` (Spear2H, Pike, Lance, …) |
  | `OCF_WeapRanged` | Bow, Crossbow, Gun, Blowgun, Slingshot |
  | `OCF_WeapCasting` | Staff1H, Wand1H, … |

  - Other useful weapon keywords: `OCF_WeapThrowable`, `OCF_WeapReverseGrip`, `OCF_EquipType*`,
    `OCF_HasEnchantmentT/F`.
  - OCF also has keyword families for armor (type, material, slot), magic effects and spells
    (class, element, delivery), ingredients and other record types. These are possible inputs for
    the armor, Smithing-material and magic classifiers.
- **Renames happen between versions.** In 5.4.0, for example, `OCF_WeapTypePole1H` became
  `OCF_WeapPole1H`. Our rules should refer to OCF keywords by EditorID and pin a minimum OCF
  version.

## 5. CommonLibSSE-NG and the build setup

- **alandtse/CommonLibSSE-NG**, branch `ng` (https://github.com/alandtse/CommonLibSSE-NG): the
  maintained fork.
  - One DLL supports SE, AE and VR. Needs C++23 and VS2022 or clang-cl. Works with CMake or xmake.
  - The README calls the colorglass vcpkg port old and unmaintained.
  - **License: GPL-3.0-or-later with a modding exception** (see its `EXCEPTIONS.md`). → Q-TECH-2.
- **CharmedBaryon/CommonLibSSE-NG**: the original NG. Last version mentioned is 3.5.2. Available as
  the vcpkg port `commonlibsse-ng` from the colorglass registry.
- **Third-party vcpkg ports** (Monitor221hz/modding-vcpkg-ports): `commonlibsse-ng-fork`, which
  points at alandtse `ng`, and others.
- **Templates:**

  | Template | Build | CommonLib |
  |---|---|---|
  | libxse/commonlibsse-ng-template | xmake 3.0+, C++23; generates a VS solution (`xmake project -k vsxmake`) | alandtse submodule |
  | SkyrimDev/HelloWorld-using-CommonLibSSE-NG (+ tutorial at skyrim.dev) | CMake + vcpkg | colorglass port |

  Others found only in search results: FlayaN (submodule), codepuncher (CMake, CI), epinter,
  Monitor221hz.
- **CLibUtil** (powerof3, MIT, header-only, `clib-util` 1.5.1). Headers:
  - `string`: hashing, split/join, `to_num`;
  - `distribution`: `get_configs` to find drop-in config files by suffix;
  - `rng`, `simpleINI`, `editorID`, `hotkeys`, `timer`, `numeric`, `bitmasks`.

  Whether there is a public vcpkg port is *(unconfirmed)*; it can be used as a submodule.
- **Recommendation for stage 2 (plan):** alandtse `ng` as a submodule, with the libxse xmake
  template or a CMake + vcpkg template. Decide together with Q-TECH-2.

## 6. PrismaUI

Web research of 2026-11-12 (docs https://www.prismaui.dev, source
https://github.com/PrismaUI-SKSE/framework, official example
https://github.com/PrismaUI-SKSE/example-skse-plugin). The Nexus page (mod 148718) returned
HTTP 403; facts from it come from search snippets. Used by
[`impl/status-ui.md`](../impl/status-ui.md).

- **Version:** 1.5.1 (latest; 1.0.0, 1.1.0 and 1.2.0 each needed a full reinstall). HTML / CSS / JS
  rendered by Ultralight (WebKit). Custom license: free for non-commercial use; plugins that use
  it are fine.
- **Game:** docs say SE and AE; 1.5.0 added 1.7 and VR. Built on CommonLib-NG with Address
  Library. **1.6.1170 is not listed explicitly** *(verify)*; one user reports 1.5.1 working on
  1.6.1170.
- **Requirements:** SKSE, Address Library, **Media Keys Fix SKSE** (Nexus 92948, needed for
  keyboard input). LoreRim already has Media Keys Fix SKSE 1.0.1 and Address Library (profile of
  2026-11-11). No VC++ runtime mentioned *(verify)*.
- **Files:** everything under `Data` since 1.2.0 (no files in the game root). Views in
  `Data/PrismaUI/views/<name>/`. Log: `Documents\My Games\Skyrim Special Edition\SKSE\PrismaUI.log`.
  Exact file list *(verify)*.
- **Conflicts:** "non-invasive" (does not touch Scaleform; runs alongside SkyUI). Nothing official
  on ENB, Community Shaders or SKSE Menu Framework *(verify)*. Some reports of CTDs when opening
  views in other mods (SkyrimNet issue 647); some mods now prefer "Magelight UI".
- **C++ API** (`PrismaUI_API.h`, header-only, namespace `PRISMA_UI_API`; interfaces `IVPrismaUI1`,
  `IVPrismaUI2`):
  - `RequestPluginAPI(InterfaceVersion)`: call at or after PostLoad; docs and example use
    `kDataLoaded`.
  - `PrismaView CreateView(htmlPath, OnDomReady)`: path relative to `Data/PrismaUI/views/`; call JS
    only after DOM ready. Docs recommend one view per plugin.
  - `Invoke(view, script, cb)` (async, result as string); `InteropCall(view, fn, arg)` (fast, one
    string argument, global JS function, no result) for frequent updates.
  - `RegisterJSListener(view, name, cb)` creates `window.name(str)`; the C++ side gets one
    `const char*` (send JSON). Listeners survive page reloads.
  - `Show` / `Hide` / `IsHidden`, `Focus(view, pauseGame, disableFocusMenu)` (opens a modal focus
    menu: cursor, game controls off) / `Unfocus` / `HasFocus`, `SetOrder`, `Destroy`.
  - Inspector: `CreateInspectorView`, `SetInspectorBounds`, `SetInspectorVisibility` (first open
    ~5 s). V2: `RegisterConsoleCallback` forwards JS `console.*` (exact signature *(verify)*).
  - All callbacks are plain function pointers.
- **Threads:** API callbacks run on the main thread since 1.1.0 (internal per-view queue);
  Ultralight runs on its own thread. Whether callers may use the API from other threads is not
  documented: call it from the main thread.
- **Hotkey:** none built in. The example registers a `BSTEventSink<RE::InputEvent*>` on
  `BSInputDeviceManager` and toggles `Focus` / `Unfocus` on a key-down. Whether game input events
  still reach our sink while a view is focused *(verify)* (the example closes with the same key,
  so probably yes).
- **Limits:** ES2022, no WebGL, no video or audio, CPU rendering at ≤ 60 FPS (avoid heavy CSS
  effects), `contextmenu` does not fire; local files from the views folder, remote URLs allowed
  but not recommended. A page can be developed in a normal browser if the bound functions are
  guarded.

## 7. Not researched yet

- Custom Skills Framework API (deferred).
- Precision, TDM and Parrying RPG APIs (integration points in design §14).
- SPID, FLM, po3 Tweaks and po3 Papyrus Extender details.
