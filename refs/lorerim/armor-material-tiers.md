# LoreRim armor materials → tier (armor rating effect of material sub-skills)

For the material armor rating effect ([armor](../../skills/armor/design.md) *Continuous effects*:
`−30 %` at 0, `0` at 50, `+F1(tier) %` at 100, `+F2(tier) %` at 200). Fill **Tier** per row and the
two tables below.

Rows: armor-set / material keywords on HeavyArmor / LightArmor **body** pieces in the research
dump (`research/tables/armor-materials.tsv`, 2026-11-11), sorted by the number of cuirasses.
Requiem keywords are set families, not physical materials (see [`materials.md`](materials.md));
one row may become one material sub-skill or be merged (Q-ARM-3). *Examples*: non-enchanted
base items (no template). *Smithing proposal*: from `materials.md`.

**Cuirass armor rating is not in the dump** (the ARMO export has no `ArmorRating` / `Weight`
fields). The column is filled after a re-export with those two fields
([R-35](../../research/R-35-armor-rating-export.md)).
Regenerate the rows: `python3 -I scripts/gen_armor_material_tiers.py research/tables/armor-materials.tsv refs/lorerim/materials.md`.

## F1, F2 by tier (to fill)

| Tier | F1: bonus at 100, % | F2: bonus at 200, % |
|---|---|---|
| 1 |  |  |
| 2 |  |  |
| 3 |  |  |

## Materials

| Keyword | FormKey | Cuirasses | Examples | Smithing proposal | **Cuirass armor rating** | **Tier** |
|---|---|---|---|---|---|---|
| `REQ_ArmorSet_Leather` | `06BBDB:Skyrim.esm` | Light 150 | Travelling Priest Robes - Blue, Travelling Priest Robes - Black, Leather Cuirass | leather |  |  |
| `REQ_ArmorSet_Ebony` | `06BBD8:Skyrim.esm` | Heavy 93 | Vvardenfell Ebony Cuirass, Ebony Spell Knight Armor (CoW - Blue), Ebony Spell Knight Armor (DG - Brown) | ebony |  |  |
| `REQ_ArmorSet_Elven` | `06BBD9:Skyrim.esm` | Light 91 | Imga Priest Robes, Elven Enforcer Armor, Elven Cuirass | elven |  |  |
| `REQ_ArmorSet_Steel` | `06BBE6:Skyrim.esm` | Heavy 72 | Steel Cuirass, Steel Armor, Fur-lined Steel Armor | steel |  |  |
| `REQ_ArmorSet_Iron` | `06BBE3:Skyrim.esm` | Heavy 72 | Iron Plate Cuirass, Sleeved Iron Armour, Poor Iron Armour | iron |  |  |
| `REQ_ArmorSet_Hide` | `06BBDD:Skyrim.esm` | Light 66 | Colovian Fur Armor, Hide Cuirass, Skaal MofuMofu Coat | hide / leather |  |  |
| `REQ_ArmorSet_Glass` | `06BBDC:Skyrim.esm` | Light 62 | Alessian Battlemage Armor, Alessian Soldier Armor, Colovian Malachite Armor | glass |  |  |
| `REQ_ArmorSet_Daedric` | `06BBD4:Skyrim.esm` | Heavy 58 | Daedric Helmet, Xivkyn Armor, mino1 | daedric |  |  |
| `REQ_ArmorSet_Scale` | `06BBDE:Skyrim.esm` | Light 57 | Scale Cuirass, River's Clothes, Shadowscale Armor | steel (scaled) |  |  |
| `REQ_ArmorSet_Dragonplate` | `06BBD5:Skyrim.esm` | Heavy 49 | Dragonplate Cuirass, Dragonplate Insulated Cuirass, Dragonbone Mail | dragon |  |  |
| `REQ_ArmorSet_SteelPlate` | `06BBE7:Skyrim.esm` | Heavy 46 | Steel Plate Cuirass, Steel Plate Armor, Full Plate Armor | steel (plate? Q-SMI-5) |  |  |
| `REQ_ArmorSet_Dragonscale` | `06BBD6:Skyrim.esm` | Light 46 | Light Dragonbone Cuirass, Studded Dragonscale Cuirass, Miraak's Light Armor | steel (scaled) |  |  |
| `REQ_ArmorSet_DwarvenHeavy` | `06BBD7:Skyrim.esm` | Heavy 44 | Dwarven Heavy Cuirass, Immaculate Dwarven Armor, Aetherial Armor | dwarven |  |  |
| `REQ_ArmorSet_ChitinLight` | `024102:Dragonborn.esm` | Light 42 | Reaver's Garb, Indoril Wayfarer Armor, Morag Tong Cuirass | chitin |  |  |
| `REQ_ArmorSet_Quicksilver` | `024105:Dragonborn.esm` | Heavy 39 | Pelinal's Armor (Rusty), Mythic Dawn Cuirass, Lunar Guard Armor | nordic / quicksilver |  |  |
| `REQ_LEGACY_ArmorMaterialElvenGilded` | `06BBDA:Skyrim.esm` | Light 38 | Imga Priest Robes, Elven Cuirass, Gallant Bandit Armor | elven |  |  |
| `FZR_ArmorSet_Bonemold` | `024101:Dragonborn.esm` | Heavy 37 | Bonemold Cuirass, Her Hand Armor, Heathguard Armor | bonemold |  |  |
| `FZR_ArmorSet_StalhrimLight` | `024107:Dragonborn.esm` | Light 33 | Rimeweld Cuirass, Deathbrand Cuirass, Stalhrim Light Cuirass | stalhrim |  |  |
| `FZR_ArmorSet_StalhrimHeavy` | `024106:Dragonborn.esm` | Heavy 30 | Stalhrim Fur Cuirass, Stalhrim Heavy Cuirass | stalhrim |  |  |
| `REQ_ArmorSet_OrcishHeavy` | `06BBE5:Skyrim.esm` | Heavy 28 | Orcish Plate Cuirass, Orcish Warchief Armor, Orcish Chain Armor | orcish |  |  |
| `REQ_ArmorSet_ChitinHeavy` | `024103:Dragonborn.esm` | Heavy 28 | Rhino Beetle Armor, Chitin Heavy Cuirass, Plated Mudcrab Chitin Cuirass | chitin |  |  |
| `ccBGSSSE025_ArmorMaterialMadness` | `21BD65:ccBGSSSE025-AdvDSGS.esm` | Heavy 28 | Madness Armor | madness (CC) |  |  |
| `REQ_ArmorSet_DwarvenLight` | `ADDD6F:Requiem.esp` | Light 27 | Dwarven Mail Cuirass, Rihad Swordsman Armor, Dwarven Light Cuirass | dwarven |  |  |
| `ccBGSSSE025_ArmorMaterialAmber` | `21BD66:ccBGSSSE025-AdvDSGS.esm` | Light 27 | Amber Armor | amber (CC) |  |  |
| `REQ_ArmorSet_Vampire` | `01463E:Dawnguard.esm` | Light 26 | Vampire Cuirass, Vampire Royal Cuirass, Vampiric Light Armored Robes | leather / ? (vampire) |  |  |
| `REQ_ArmorSet_Guard` | `AD39C3:Requiem.esp` | Light 17, Heavy 9 | Stormcloak Padded Gambeson with Scaled Cuirass , Stormcloak Chainmail, Stormcloak Chainmail with Leather Cuirass  | iron/steel (faction) |  |  |
| `REQ_LEGACY_ArmorMaterialIronBanded` | `06BBE4:Skyrim.esm` | Heavy 23 | Poor Housecarl Armor, Iron Cuirass | iron |  |  |
| `REQ_ArmorSet_Vigilant` | `AD3B02:Requiem.esp` | Heavy 21, Light 1 | Vigilant Cuirass, Vigil Veteran Sashed Cuirass, Vigil Enforcer Cuirass | ? (faction set: check COBJ) |  |  |
| `REQ_ArmorSet_Fur` | `AD3ADE:Requiem.esp` | Light 19 | Fur Armor, Fur Cuirass, Makeshift Fur Armor | hide / leather |  |  |
| `REQ_ArmorSet_ImperialHeavy` | `06BBE2:Skyrim.esm` | Heavy 16 | Imperial Legate Armor, Rust Armor, Old Armor | steel (imperial) |  |  |
| `REQ_ArmorSet_ImperialLight` | `06BBE0:Skyrim.esm` | Light 16 | Tattered Imperial Armor, Imperial Chainmail, Imperial Light Cuirass | steel (imperial) |  |  |
| `REQ_ArmorSet_NetchLeather` | `ADDD70:Requiem.esp` | Light 13 | Netch Leather Cuirass, Boiled Netch Leather Cuirass, Boiled Netch Leather Armor | leather (netch?) |  |  |
| `REQ_ArmorSet_Forsworn` | `0009B9:Update.esm` | Light 11, Heavy 2 | Forsworn Skirt, Ancient Reach Minotaur Armor, Forsworn Ravager Armor | hide / bone (forsworn) |  |  |
| `REQ_ArmorSet_AncientNord` | `AD39BE:Requiem.esp` | Heavy 13 | Ancient Nord Armor, Ancient Nord Cuirass | ancient nord (draugr) |  |  |
| `REQ_ArmorSet_Chainmail` | `ADDD5E:Requiem.esp` | Light 11 | Marukhati Selective Armor, Long Nordic Chainmail, Nordic Chainmail | iron/steel |  |  |
| `REQ_ArmorSet_DawnguardLight` | `0050C4:Dawnguard.esm` | Light 9, Heavy 1 | Dawnguard Light Cuirass, Dawnguard Armor | steel / leather (dawnguard) |  |  |
| `REQ_LEGACY_ArmorMaterialStormcloak` | `0AC13A:Skyrim.esm` | Light 8, Heavy 2 | Stormcloak Padded Gambeson with Scaled Cuirass , Stormcloak Chainmail, Stormcloak Chainmail with Leather Cuirass  | iron/steel (faction) |  |  |
| `REQ_ArmorSet_Alikr` | `ADDD9F:Requiem.esp` | Light 9 | Alik'r Light Armor, Remnant Cuirass, Remnant Armor | ? (faction set: check COBJ) |  |  |
| `Trad_002_ArmorSet_SpellKnight` | `23EE93:Trad_AE_CC_Collection_Requiem_Patch.esp` | Heavy 9 | Ebony Spell Knight Armor (CoW - Blue), Ebony Spell Knight Armor (DG - Brown), Ebony Spell Knight Armor (CoW - Gold) | ? (faction set: check COBJ) |  |  |
| `REQ_ArmorSet_DawnguardHeavy` | `012CCD:Dawnguard.esm` | Heavy 8 | Dawnguard Heavy Cuirass, Dawnguard Heavy Armor SPR | steel / leather (dawnguard) |  |  |
| `REQ_ArmorSet_Shrouded` | `10FD62:Skyrim.esm` | Light 7 | Worn Shrouded Cuirass, Shrouded Cuirass, Taliesin's Armor | leather (faction) |  |  |
| `REQ_ArmorSet_SilverHand` | `ADDD71:Requiem.esp` | Heavy 7 | Silver Hand Veteran Cuirass, Vigil Silver Hand Cuirass, Silver Hand Elite Cuirass | silver |  |  |
| `REQ_LEGACY_ArmorMaterialImperialStudded` | `06BBE1:Skyrim.esm` | Light 7 | Imperial Chainmail | leather/iron |  |  |
| `REQ_LEGACY_ArmorMaterialStudded` | `06BBDF:Skyrim.esm` | Light 7 | Hide Cuirass | leather/iron |  |  |
| `Trad_CC_ArmorSet_DaedricLight` | `3B2D08:Trad_AE_CC_Collection_Requiem_Patch.esp` | Light 6 | Daedric Mail Cuirass, Savior's Cuirass, Savior's Hide | daedric |  |  |
| `REQ_ArmorSet_OrcishLight` | `ADDD9E:Requiem.esp` | Light 6 | Orcish Scaled Cuirass, Orcish Berserk Armor, Orcish Forgemaster Armor | orcish |  |  |
| `REQ_ArmorSet_VampireHeavy` | `000AB0:Big Tweaks.esp` | Heavy 6 | Ancient Vampire Armor, Vampiric Heavy Cuirass, Vampiric Heavy Battlemage Cuirass | leather / ? (vampire) |  |  |
| `REQ_ArmorSet_ThievesGuild` | `0009BC:Update.esm` | Light 6 | Thieves Guild Cuirass, Thieves Guild Variant Cuirass, Karliah's Cuirass | leather (faction) |  |  |
| `REQ_ArmorSet_Akaviri` | `0009C0:Update.esm` | Heavy 5 | Akaviri Cuirass | steel (akaviri) |  |  |
| `REQ_ArmorSet_AncientFalmer` | `AD39BF:Requiem.esp` | Light 4 | Ancient Falmer Cuirass | falmer (chaurus) |  |  |
| `REQ_ArmorSet_Nightingale` | `10FD61:Skyrim.esm` | Light 4 | Nightingale Cuirass, Nightingale Armor | leather (faction) |  |  |
| `REQ_ArmorSet_Ebony_Light` | `00084F:Big Tweaks.esp` | Light 3 | Ebony Light Cuirass, Elmlock Armor, Ebony Mail | ebony |  |  |
| `REQ_ArmorSet_Wolf` | `AD3A3F:Requiem.esp` | Heavy 3 | Legendary Atmoran Cuirass, Atmoran Cuirass, Wolf Cuirass | hide / leather |  |  |
| `ccBGSSSE025_ArmorMaterialGolden` | `21BD63:ccBGSSSE025-AdvDSGS.esm` | Heavy 2, Light 1 | Golden Saint Armor, Golden Seducer Armor | gold (jewelry?) |  |  |
| `REQ_ArmorSet_FalmerHeavy` | `012CCF:Dawnguard.esm` | Heavy 3 | Falmer Heavy Cuirass | falmer (chaurus) |  |  |
| `REQ_ArmorSet_ImperialLegate` | `AD39C5:Requiem.esp` | Heavy 3 | Imperial Dragon Armor, Imperial Legate Cuirass | steel (imperial) |  |  |
| `Trad_ArmorSet_Vigilant_Corrupted` | `3B2D0E:Trad_AE_CC_Collection_Requiem_Patch.esp` | Heavy 2 | Vigil Corrupted Sashed Cuirass | ? (faction set: check COBJ) |  |  |
| `REQ_ArmorSet_StormcloakOfficer` | `0009BE:Update.esm` | Light 1, Heavy 1 | Storm-Bear Armor, Stormcloak Officer Cuirass | iron/steel (faction) |  |  |
| `ccBGSSSE025_ArmorMaterialDark` | `21BD64:ccBGSSSE025-AdvDSGS.esm` | Light 2 | Dark Seducer Armor | dark (CC) |  |  |
| `FZR_ArmorSet_Deathbrand` | `024022:Dragonborn.esm` | Light 2 | Deathbrand Cuirass | ? (faction set: check COBJ) |  |  |
| `FZR_ArmorSet_MoragTong` | `03A328:Dragonborn.esm` | Light 2 | Morag Tong Cuirass, Morag Tong Assassin's Armour | leather (faction) |  |  |
| `REQ_ArmorSet_ThievesGuildMaster` | `0009BF:Update.esm` | Light 2 | Guild Master Cuirass | leather (faction) |  |  |
| `Trad_056_ArmorSet_Silver` | `253488:Trad_AE_CC_Collection_Requiem_Patch.esp` | Heavy 1 | Silver Cuirass | silver |  |  |
| `Trad_001_ArmorSet_StormBear` | `18796D:Trad_AE_CC_Collection_Requiem_Patch.esp` | Light 1 | Storm-Bear Armor | iron/steel (faction) |  |  |
| `Trad_001_ArmorSet_ImperialDragon` | `18796E:Trad_AE_CC_Collection_Requiem_Patch.esp` | Heavy 1 | Imperial Dragon Armor | ? |  |  |
| `REQ_NULL_DLC1ArmorMaterialFalmerHardened` | `012CCE:Dawnguard.esm` | Heavy 1 | Falmer Hardened Armor | falmer (chaurus) |  |  |
| `USLEEPArmorMaterialBlackguard` | `900014:Update.esm` | Light 1 | Blackguard Cuirass | iron/steel (faction) |  |  |
| `REQ_ArmorSet_Skaal` | `AD39C1:Requiem.esp` | Light 1 | Skaal Coat | hide / leather |  |  |
| `REQ_ArmorSet_FalmerLight` | `0009BD:Update.esm` | Light 1 | Falmer Light Cuirass | falmer (chaurus) |  |  |
| `REQ_ArmorSet_Executioner` | `AD39C0:Requiem.esp` | Light 1 | Executioner Cuirass | ? (faction set: check COBJ) |  |  |
| `REQ_ArmorSet_PenitusOculatus` | `0009BB:Update.esm` | Light 1 | Penitus Oculatus Cuirass | steel (imperial) |  |  |
| `REQ_ArmorSet_AncientShrouded` | `AD39C4:Requiem.esp` | Light 1 | Ancient Shrouded Cuirass | leather (faction) |  |  |
| `REQ_NULL_ArmorMaterialMS02Forsworn` | `0009BA:Update.esm` | Light 1 | — | hide / bone (forsworn) |  |  |
| `REQ_ArmorSet_SummersetShadows` | `ADDDB5:Requiem.esp` | Light 1 | Summerset Shadows Cuirass | elven |  |  |
| `REQ_ArmorSet_Linwe` | `900015:Update.esm` | Light 1 | Linwe's Cuirass | elven |  |  |
