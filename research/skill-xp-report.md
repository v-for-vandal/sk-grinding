# R-22: опыт навыков, цепочки перков и настройки

Исследован профиль **Default**, LoreRim **5.0 с пользовательскими дополнениями**, 3492 активных плагина. Игра не запускалась; файлы игры, модов и MO2 не изменялись. «Ваниль» ниже — установленные Skyrim.esm + Update.esm, без независимой проверки эталонного дистрибутива.

## AVIF и разночтение Smithing

**Smithing UseMult: Skyrim.esm = 160, Update.esm = 1**, FormKey `000450:Skyrim.esm`, EditorID `AVSmithing`. Это разные версии одной записи, а не ошибка округления. Остальные три поля в обеих версиях: `0 / 0.25 / 300`.

Все 18 наборов чисел из таблицы задания подтверждаются для Skyrim.esm, если для Smithing выбрать 160. Для ванили с Update.esm нужен 1. Других расхождений с приведёнными числами нет. Иллюзия в реальных файлах называется **AVMysticism**, FormKey `00045B:Skyrim.esm`; `AVIllusion` не найден. Это один из 18 основных навыков, не дополнительный навык.

[skill-avif-chain.tsv](tables/skill-avif-chain.tsv) содержит каждую версию всех 18 AVIF. Индексы загрузки начинаются с 0, включают автоматически загружаемые masters/CC и не являются runtime-префиксами FormID. У всех итоговых 18 AVIF `UseMult=0`, `ImproveOffset=9999`; сохранены фактические плагины-победители. Восстановление естественного роста потребует учитывать и эти записи, и GMST, и SSL-скрипт.

| Навык (EditorID) | FormKey | Skyrim.esm: Use / Offset / Improve / Offset | Победитель |
|---|---|---|---|
| AVOneHanded | 00044C:Skyrim.esm | 6.3 / 0 / 2 / 0 | Ordinator - Perks of Skyrim.esp |
| AVTwoHanded | 00044D:Skyrim.esm | 5.95 / 0 / 2 / 0 | Ordinator - Perks of Skyrim.esp |
| AVMarksman | 00044E:Skyrim.esm | 9.3 / 0 / 2 / 0 | Ordinator - Perks of Skyrim.esp |
| AVBlock | 00044F:Skyrim.esm | 8.1 / 0 / 2 / 0 | LoreRim - Spells and Magic Effects.esp |
| AVSmithing | 000450:Skyrim.esm | 160 / 0 / 0.25 / 300 | Ordinator - Perks of Skyrim.esp |
| AVHeavyArmor | 000451:Skyrim.esm | 3.8 / 0 / 2 / 0 | Ordinator - Perks of Skyrim.esp |
| AVLightArmor | 000452:Skyrim.esm | 4 / 0 / 2 / 0 | Ordinator - Perks of Skyrim.esp |
| AVPickpocket | 000453:Skyrim.esm | 8.1 / 0 / 0.25 / 250 | Ordinator - Perks of Skyrim.esp |
| AVLockpicking | 000454:Skyrim.esm | 45 / 10 / 0.25 / 300 | Ordinator - Perks of Skyrim.esp |
| AVSneak | 000455:Skyrim.esm | 11.25 / 0 / 0.5 / 120 | LoreRim - Spells and Magic Effects.esp |
| AVAlchemy | 000456:Skyrim.esm | 0.75 / 0 / 1.6 / 65 | Ordinator - Perks of Skyrim.esp |
| AVSpeechcraft | 000457:Skyrim.esm | 0.36 / 0 / 2 / 0 | LoreRim - Spells and Magic Effects.esp |
| AVAlteration | 000458:Skyrim.esm | 3 / 0 / 2 / 0 | Ordinator - Perks of Skyrim.esp |
| AVConjuration | 000459:Skyrim.esm | 2.1 / 0 / 2 / 0 | LoreRim - Spells and Magic Effects.esp |
| AVDestruction | 00045A:Skyrim.esm | 1.35 / 0 / 2 / 0 | Big Tweaks.esp |
| AVMysticism | 00045B:Skyrim.esm | 4.6 / 0 / 2 / 0 | LoreRim - Spells and Magic Effects.esp |
| AVRestoration | 00045C:Skyrim.esm | 2 / 0 / 2 / 0 | Big Tweaks.esp |
| AVEnchanting | 00045D:Skyrim.esm | 900 / 0 / 1 / 170 | Ordinator - Perks of Skyrim.esp |

## GMST

[gmst-skill-xp-chain.tsv](tables/gmst-skill-xp-chain.tsv) — все версии GMST по регулярному выражению задания **плюс все GMST SSL**, включая не совпавший с regex `fLockpickBrokenPicksMult`. Победитель GMST определяется по EditorID, даже если разные плагины создавали разные FormKey для одной настройки. [gmst-skill-xp-comparison.tsv](tables/gmst-skill-xp-comparison.tsv) содержит сравнение, включая неизвестную ваниль и значения SSL.

Все найденные изменения относительно Skyrim.esm + Update.esm:

| EditorID | FormKey победителя | Ваниль → итог | Победитель | Смысл изменения |
|---|---|---|---|---|
| fAlchemySkillFactor | 10C4D9:Skyrim.esm | 1.5 → 2 | Requiem - Alchemy Redone.esp | Параметр зависимости от навыка: 1.5 → 2; не универсальный множитель XP ×4/3 |
| fConstructibleSkillUseConst | 000943:Update.esm | 25 → 0 | Requiem.esp | Вместе: 25 + 3 × V^0.65 → 0.5 × V до AVIF/прочих множителей |
| fConstructibleSkilluseExp | 000942:Update.esm | 0.65 → 1 | Requiem.esp | Вместе: 25 + 3 × V^0.65 → 0.5 × V до AVIF/прочих множителей |
| fConstructibleSkillUseMult | 000941:Update.esm | 3 → 0.5 | Requiem.esp | Вместе: 25 + 3 × V^0.65 → 0.5 × V до AVIF/прочих множителей |
| fDisenchantSkillUse | 10C0E5:Skyrim.esm | 0.0025 → 0 | Requiem.esp | Коэффициент опыта за изучение зачарования обнулён |
| fSkillUsageLockPickAverage | 0FEAD8:Skyrim.esm | 5 → 0 | StaticSkillLeveling.esp | Компонент опыта взлома/поломки обнулён SSL |
| fSkillUsageLockPickBroken | 0FEAD5:Skyrim.esm | 0.25 → 0 | StaticSkillLeveling.esp | Компонент опыта взлома/поломки обнулён SSL |
| fSkillUsageLockPickEasy | 0FEAD7:Skyrim.esm | 3 → 0 | StaticSkillLeveling.esp | Компонент опыта взлома/поломки обнулён SSL |
| fSkillUsageLockPickHard | 0FEADA:Skyrim.esm | 8 → 0 | StaticSkillLeveling.esp | Компонент опыта взлома/поломки обнулён SSL |
| fSkillUsageLockPickVeryEasy | 0FEAD6:Skyrim.esm | 2 → 0 | StaticSkillLeveling.esp | Компонент опыта взлома/поломки обнулён SSL |
| fSkillUsageLockPickVeryHard | 0FEAD9:Skyrim.esm | 13 → 0 | StaticSkillLeveling.esp | Компонент опыта взлома/поломки обнулён SSL |
| fSkillUsageRechargeMult | 10E2B3:Skyrim.esm | 0.0002 → 0.001 | Requiem - Magic Redone.esp | Коэффициент опыта за перезарядку ×5 до остальных множителей |
| fSneakAttackSkillUsageMelee | 105CB7:Skyrim.esm | 30 → 60 | Requiem.esp | Коэффициент опыта скрытой атаки в ближнем бою ×2 |
| iCrimeGoldStealHorse | 05158E:Skyrim.esm | 100 → 2500 | LoreRim - Economy Overhaul.esp | Штраф/преступление; прямой формулы сырого XP не установлено |
| iCrimeGoldWerewolf | 0F0832:Skyrim.esm | 1000 → 10000 | LoreRim - Economy Overhaul.esp | Штраф/преступление; прямой формулы сырого XP не установлено |
| sAddCrimeGold | 02F579:Skyrim.esm | bounty added to → septims more on my head in | Requiem.esp | Текст интерфейса; сырой XP не меняет |
| sLegendaryResetConfirm | 0009D9:Update.esm |  → Making a skill Legendary will return its perks. Are you sure? Requiem does not support this feature, some perks will no longer work as intended. | Requiem.esp | Текст интерфейса; сырой XP не меняет |
| sLegendaryResetSecondConfirm | 0009D8:Update.esm |  → Are you sure you want to make this skill Legendary? Requiem does not support this feature, some perks will no longer work as intended. | Requiem.esp | Текст интерфейса; сырой XP не меняет |
| sNoLockPickIfCrimeAlert | 0D4EAC:Skyrim.esm | You can't pick a lock while being pursued by guards. →  | Requiem - No Messages.esp | Текст интерфейса; сырой XP не меняет |
| sRemoveCrimeGold | 02F57A:Skyrim.esm | bounty removed from → septims less on my head in | Requiem.esp | Текст интерфейса; сырой XP не меняет |

Формула крафта сопоставлена с [исходным скриптом Crafting Hotkeys](https://github.com/Rukan/Grimy-Skyrim-Papyrus-Source/blob/master/GUI_Script_CraftingHotkeys.psc), который рассчитывает выдаваемый опыт из этих GMST. Это подтверждение используемой формулы в скрипте мода, а не дизассемблирование Skyrim.exe. V — стоимость предмета; количество изделий и другие множители надо учитывать отдельно. Итоговый нулевой UseMult в LoreRim не позволяет трактовать эту формулу как фактически полученный прирост навыка.

GMST, впервые явно заданные модами, имеют пустые vanillaValue и `changed=unknown`: отсутствие записи в vanilla-плагинах не означает значение 0. В частности, это fLegendarySkillResetValue, fSneakAttackSkillUsageRanged, iSneakSkillUseDistance, настройки дней заключения и часть штрафов. Их текущие значения и все версии сохранены в таблицах; ванильные значения движка не подставлялись по памяти. `fSkillUseCurve=1.95` (`024E4E:Skyrim.esm`, Skyrim.esm) не переопределён.

**Все семь GMST StaticSkillLeveling.esp:**

| EditorID | FormKey | Ваниль | SSL | Итог |
|---|---|---|---|---|
| fSkillUsageLockPickHard | 0FEADA:Skyrim.esm | 8 | 0 | 0 (StaticSkillLeveling.esp) |
| fSkillUsageLockPickVeryHard | 0FEAD9:Skyrim.esm | 13 | 0 | 0 (StaticSkillLeveling.esp) |
| fSkillUsageLockPickAverage | 0FEAD8:Skyrim.esm | 5 | 0 | 0 (StaticSkillLeveling.esp) |
| fSkillUsageLockPickEasy | 0FEAD7:Skyrim.esm | 3 | 0 | 0 (StaticSkillLeveling.esp) |
| fSkillUsageLockPickVeryEasy | 0FEAD6:Skyrim.esm | 2 | 0 | 0 (StaticSkillLeveling.esp) |
| fSkillUsageLockPickBroken | 0FEAD5:Skyrim.esm | 0.25 | 0 | 0 (StaticSkillLeveling.esp) |
| fLockpickBrokenPicksMult | 0F9641:Skyrim.esm | 0 | 0 | 0 (StaticSkillLeveling.esp) |

## Перки и деревья

**842** PERK имеют больше одной версии; экспортировано **983** перка / **2418** версий с учётом всех деревьев и рекурсивного NextPerk каждой версии. **341** перк переопределяют как минимум два плагина помимо владельца FormKey.

- [perk-override-chain.tsv](tables/perk-override-chain.tsv): полные имена, описания, флаги, число рангов, NextPerk, условия и эффекты каждой версии. `$type` условия задаёт функцию, Flags сохраняет OR, отсутствие OR означает связку AND; RunOnTabIndex сохраняет вкладку условий эффекта. Числовые AV и tab index не переименовываются догадками.
- [perk-override-diff.tsv](tables/perk-override-diff.tsv): сравнение строго с предыдущей версией. `added`/`removed` содержат полные добавленные/удалённые элементы, `changedIndices` — изменённые позиции. Изменение элемента представлено удалением старого и добавлением нового; это структурный diff, не оценка игрового баланса. Пустой changedFields означает совпадение исследуемых полей.
- [perk-conflicts.tsv](tables/perk-conflicts.tsv): **для каждого** конфликтующего перка указаны победитель, каждая промежуточная версия и её заменённые значения. Это фиксирует, какие данные не дошли до победителя; само по себе не доказывает ошибку совместимости. Одинаковые эффекты, перенесённые в иной перк, автоматически не распознаются.
- [avif-perktree-chain.tsv](tables/avif-perktree-chain.tsv): узлы, координаты, связи и индексы каждой версии AVIF; служебный узел с `Perk=Null` сохранён.
- [avif-perktree-diff.tsv](tables/avif-perktree-diff.tsv): сравнение каждого дерева с итоговым, включая добавленные/удалённые узлы, перестановки и изменение координат/связей.

Ссылки в JSON дополнены FormKey, EditorID и labelSource: для PERK берётся последняя версия на момент исследуемого плагина, для прочих типов — каталог итоговых записей предыдущего исследования. Это явно отмечено, чтобы итоговое имя не выдавалось за историческое. Сравнение производится по исходным ссылкам FormKey, до добавления имён. В diff игнорируется дублирующий сырой Index внутри FormLinkOrIndex, если Link уже содержит разрешённый FormKey: изменение локального индекса masters само по себе не считается изменением условия. Для ссылок на alias/index без разрешённой формы Index сохраняет значение.

Топ плагинов по количеству записей PERK, ссылающихся на чужой FormKey (включая технические и совпадающие overrides):

| Плагин | Число |
|---|---|
| Requiem.esp | 406 |
| Big Tweaks.esp | 197 |
| Wintersun - Reqtificated.esp | 106 |
| Requiem - Magic Redone.esp | 105 |
| Unofficial Skyrim Special Edition Patch.esp | 84 |
| Requiem - Weapons and Armor Redone.esp | 65 |
| Requiem_-_Modern_Combat_Rework.esp | 59 |
| SheepSheriff_SubclassesOfSkyrim_Patch.esp | 43 |
| Requiem - Stealth Redone.esp | 23 |
| Growl - Reqtificated.esp | 22 |
| Fozars_Dragonborn_-_Requiem_Patch.esp | 21 |
| Traits - LoreRim Balance Patch.esp | 21 |
| Dragonborn.esm | 20 |
| Sacrilege - Minimalistic Vampires of Skyrim.esp | 18 |
| Apocalypse - Requiem & Magic Redone Patch.esp | 18 |

Все плагины цепочки каждого навыка приведены ниже. Число после имени — узлы, включая служебные; `=` — дерево полностью совпадает с итоговым, `≠` — отличается. Точные различия с **каждым** модом, включая Ordinator/Requiem, вынесены в avif-perktree-diff.tsv.

| AVIF / FormKey | Цепочка деревьев |
|---|---|
| AVOneHanded / 00044C:Skyrim.esm | Skyrim.esm (11; ≠) → Requiem.esp (13; ≠) → Requiem - Special Feats.esp (15; ≠) → Requiem - Weapons and Armor Redone.esp (21; ≠) → Big Tweaks.esp (23; ≠) → Ordinator - Perks of Skyrim.esp (28; =) |
| AVTwoHanded / 00044D:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (12; ≠) → Requiem - Weapons and Armor Redone.esp (16; ≠) → Ordinator - Perks of Skyrim.esp (20; =) |
| AVMarksman / 00044E:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (12; ≠) → Requiem - Special Feats.esp (14; ≠) → Requiem - Weapons and Armor Redone.esp (15; ≠) → Big Tweaks.esp (17; ≠) → Ordinator - Perks of Skyrim.esp (20; =) |
| AVBlock / 00044F:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (10; ≠) → Requiem - Special Feats.esp (12; ≠) → LoreRim - Weaponmaster.esp (18; =) → LoreRim - Spells and Magic Effects.esp (18; =) |
| AVSmithing / 000450:Skyrim.esm | Skyrim.esm (11; ≠) → Update.esm (11; ≠) → Requiem.esp (13; ≠) → Fozars_Dragonborn_-_Requiem_Patch.esp (16; ≠) → Requiem - Weapons and Armor Redone.esp (13; ≠) → Requiem Smithing Books Give Perks.esp (14; ≠) → Ordinator - Perks of Skyrim.esp (16; =) |
| AVHeavyArmor / 000451:Skyrim.esm | Skyrim.esm (9; ≠) → Requiem.esp (11; ≠) → Requiem - Special Feats.esp (14; ≠) → Ordinator - Perks of Skyrim.esp (18; =) |
| AVLightArmor / 000452:Skyrim.esm | Skyrim.esm (7; ≠) → Requiem.esp (10; ≠) → Requiem - Special Feats.esp (12; ≠) → Requiem_-_Modern_Combat_Rework.esp (12; ≠) → Ordinator - Perks of Skyrim.esp (15; =) |
| AVPickpocket / 000453:Skyrim.esm | Skyrim.esm (9; ≠) → Requiem.esp (7; ≠) → Requiem - Pickpocket Rework.esp (12; ≠) → LoreRim Trapper.esp (16; ≠) → Trapper - Sound Body Connection.esp (17; ≠) → Ordinator - Perks of Skyrim.esp (20; =) |
| AVLockpicking / 000454:Skyrim.esm | Skyrim.esm (12; ≠) → Requiem.esp (6; ≠) → Requiem - Lockpicking Rework.esp (14; ≠) → Wayfarer - Survival Perks.esp (20; ≠) → Ordinator - Perks of Skyrim.esp (23; =) |
| AVSneak / 000455:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (9; ≠) → Requiem - Special Feats.esp (10; ≠) → Ordinator - Perks of Skyrim.esp (15; ≠) → LoreRim - Spells and Magic Effects.esp (16; =) |
| AVAlchemy / 000456:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (12; ≠) → Requiem - Special Feats.esp (15; ≠) → Requiem - Alchemy Redone.esp (12; ≠) → Big Tweaks.esp (15; ≠) → Ordinator - Perks of Skyrim.esp (17; =) |
| AVSpeechcraft / 000457:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (13; ≠) → Requiem - Special Feats.esp (16; =) → LoreRim - Spells and Magic Effects.esp (16; =) |
| AVAlteration / 000458:Skyrim.esm | Skyrim.esm (11; ≠) → Requiem.esp (14; ≠) → Requiem - Special Feats.esp (17; ≠) → Requiem - Magic Redone.esp (20; ≠) → Big Tweaks.esp (23; ≠) → Ordinator - Perks of Skyrim.esp (23; =) |
| AVConjuration / 000459:Skyrim.esm | Skyrim.esm (16; ≠) → Requiem.esp (19; ≠) → Requiem - Magic Redone.esp (26; =) → LoreRim - Spells and Magic Effects.esp (26; =) |
| AVDestruction / 00045A:Skyrim.esm | Skyrim.esm (15; ≠) → Unofficial Skyrim Special Edition Patch.esp (15; ≠) → Requiem.esp (18; ≠) → Requiem - Magic Redone.esp (26; ≠) → Big Tweaks.esp (29; =) |
| AVMysticism / 00045B:Skyrim.esm | Skyrim.esm (14; ≠) → Requiem.esp (16; ≠) → Requiem - Magic Redone.esp (19; =) → LoreRim - Spells and Magic Effects.esp (19; =) |
| AVRestoration / 00045C:Skyrim.esm | Skyrim.esm (13; ≠) → Requiem.esp (15; ≠) → Requiem - Special Feats.esp (16; ≠) → Requiem - Magic Redone.esp (21; ≠) → Wintersun - Reqtificated.esp (22; ≠) → Big Tweaks.esp (26; =) |
| AVEnchanting / 00045D:Skyrim.esm | Skyrim.esm (10; ≠) → Requiem.esp (9; ≠) → Requiem - Special Feats.esp (11; ≠) → Requiem - Magic Redone.esp (10; ≠) → Big Tweaks.esp (12; ≠) → Wand Keywords.esp (13; ≠) → Ordinator - Perks of Skyrim.esp (15; =) |

## Конфиги и скрипты

Все цитаты с модом-поставщиком, относительным путём, номером строки и SHA-256: [skill-xp-configs.tsv](tables/skill-xp-configs.tsv). Для loose-файлов учтены enabled modlist, приоритет и overwrite. BSA не распаковывались; содержимое сохранения/MCM и исполняемое поведение DLL не снимались.

**Experience 3.7.3**, победивший `LoreRim - MCM and INI Settings/SKSE/Plugins/Experience.ini`:

```ini
bEnableKilling = true
bEnableReading = false
bEnableSkillXP = false
iMaxPlayerLevel = 101
fSkillCapBase = 50.000000
fSkillCapMult = 1.000000
bUseRacialCaps = false
```

Это отключает **опыт персонажа** за повышение навыка. Лимит естественного роста/тренировки навыка вычисляется как `50 + уровень персонажа × 1`, без расового бонуса: 51 на уровне 1, 100 на уровне 50. Это не cap формул урона/защиты и не гарантия невозможности скриптом SetActorValue выставить больше. Формула и назначение подтверждаются [описанием автора Experience](https://www.nexusmods.com/skyrimspecialedition/mods/17751?BH=0&tab=description). SSL отдельно вызывает `Experience.GetSkillCap` для своего меню. Общий предел навыка движка сверх этого здесь не устанавливался.

Из того же INI: обнаружение любой настроенной категории локаций — **10 XP**; зачистка — **100 XP**, Fort/NordicRuin/DwemerRuin/DragonLair — **200 XP**. Квесты Main/Daedric/Dawnguard/Dragonborn — **200**, College/Thieves/Brotherhood/Companions/Side — **100**, CivilWar — **150**, None/Misc/Objectives — **0**. Killing: `fLevelFactor=0.8`, `fGroupFactor=0`, `fMaxLevelMult=3`, `fDamageThreshold=0.2`, `fKillingMult=1`; Reading: `fReadingMult=1`, но канал выключен. Полная формула убийств по одной конфигурации не доказывается; коэффициенты сохранены буквально. Дополнительные базовые награды задаются `Experience/SKSE/Plugins/Experience/Actors/Default.ini` и `Races/Default.ini`, а также `LoreRim - MCM and INI Settings/SKSE/Plugins/Experience/Races/GeneratedExperiencePatch.ini`; все три файла в таблице. Loose MCM-конфигов Experience среди проверенных включённых файлов не найдено.

**SSL.** `LoreRim - MCM and INI Settings/MCM/Config/StaticSkillLeveling/settings.ini` задаёт `iSkillPointsPerLevel=5`, `fSkillPointsLevelMultiplier=0`, стоимости 1/1/1/1, cap очков 0 и `bDisableSkillLeveling=1`. Файл `MCM/Settings/StaticSkillLeveling.ini` того же мода задаёт пользовательские overrides: `iSkillPointsPerLevel=30`, стоимости **1/2/3/6**, `iMaxLevelsPerSkillPerPlayerLevel=5`, `bDisableSkillLeveling=1`. Это разные слои, не два одновременно действующих тарифа; действующие значения конкретного сохранения надо сверить в MCM. User-layer не содержит своего fSkillPointsLevelMultiplier.

Победивший исходник `Static Skill Leveling Rewritten - Enhanced Script (FIX)/source/StaticSkillLevelingQuestScript.psc`:

```papyrus
If DoDisableSkillLeveling.GetValueInt()
  ModSkillsLeveling(0.0)
Else
  ModSkillsLeveling(-1.0)
EndIf
```

Ветка -1 восстанавливает только ранее сохранённые положительные SkillUseMults; при нулевых исходных множителях это **не гарантирует восстановление ванили**. Распределение происходит отдельно через событие `SSL_SkillsDistributionCompleted`, затем `IncrementSkillBy`, fallback `AdvanceSkill`/`SetActorValue` и сброс XP навыка в 0.

**Как прекратить выдачу новых очков без удаления SSL:** в MCM задать Points per level = 0 и Level multiplier = 0. Для прекращения расходования старого банка необходимо отдельно обнулить `SSL_PointsLeftAfterLvlUp` (`000B12:StaticSkillLeveling.esp`) в тестовом сохранении; без этого банк продолжает участвовать в формуле. Это следует из `pointsEarnedOnLevel = multiplier × playerLevel + pointsLeft + pointsPerLevel`. Сам интерфейс и обработчики при таком способе остаются зарегистрированы. Отдельного подтверждённого master-переключателя полного отключения распределения в исследованных INI/MCM/исходнике **не найдено**. `bDisableSkillLeveling=0` решает другую задачу. Команды изменения/stopquest здесь не выполнялись, их корректность на живом сохранении не проверялась.

Кроме семи GMST, SSL содержит **11 GLOB и один стартовый QUST**: `StaticSkillLevelingQuest`, `000D7E:StaticSkillLeveling.esp`, с PlayerAlias, SSL_LoadGameAlias, StaticSkillLevelingQuestScript и StaticSkillLevelingMCMScript. GLOB включают банк, флаг естественного роста, бюджет/множитель/стоимости/лимит и debug. Полная запись и VMAD: `local/r22/ssl-records.json`; переносимая таблица [ssl-plugin-records.tsv](tables/ssl-plugin-records.tsv). Числа GLOB в плагине не заменяют сохранённые MCM-значения.

**Тюрьма/преступления.** Все совпадения enabled mods по jail/prison/crime/bounty, включая косметические и не относящиеся к потере навыков: [skill-xp-mod-candidates.tsv](tables/skill-xp-mod-candidates.tsv). Плагины с такими именами: [skill-xp-plugin-candidates.tsv](tables/skill-xp-plugin-candidates.tsv).

Подтверждённое изменение заключения: **Requiem - Jail Duration Fix**, `Less Jail.esp`: `iCrimeMaxNumberofDaysinJail`, `012616:Requiem.esp`, **100 → 7**; `iCrimeDaysInPrisonMod`, `012617:Requiem.esp`, **10 → 1000**, обе цепочки Requiem.esp → Less Jail.esp. В vanilla-файлах этих GMST нет. Назначение сокращения времени заключения подтверждает [автор мода](https://www.nexusmods.com/skyrimspecialedition/mods/114061). Точная формула потери прогресса навыков при отсидке не установлена. No Crime Teleport RE, Zero Bounty Hostility Fix и bounty-патчи найдены по именам; одно имя не доказывает изменение skill XP. Immersive Winterhold Jail, Dragon's Teeth Prison, Imperial jail grates — кандидаты окружения, не свидетельство изменения XP.

**Legendary.** Явный включённый loose-переключатель запрета Legendary не найден. Это ограниченный результат текстового поиска, не доказательство доступности кнопки в пропатченном SWF/DLL. В Requiem.esp есть `fLegendarySkillResetValue=5` (`4D4253:Requiem.esp`) и предупреждения `sLegendaryResetConfirm` (`0009D9:Update.esm`), `sLegendaryResetSecondConfirm` (`0009D8:Update.esm`), что Requiem не поддерживает функцию. Предупреждение и значение сброса не являются запретом. Моды Legendary Enemies/Legendary Map не относятся автоматически к Legendary навыков; активного Uncapper по имени DLL не обнаружено. Нужна визуальная проверка в игре без выполнения сброса.

## Команды для ручной проверки

Эти команды **только читают** настройки. Для отсутствующих записей нельзя гарантировать даже существование имени в данной версии движка; ошибка getgs тоже полезный результат. Если имя не встречается ни в одном плагине, текущий результат всё ещё может быть изменён DLL во время работы, поэтому это не обязательно чистый Bethesda default.

Не найдены ни в одном активном плагине:
```text
getgs fSkillUsageSneakHidden
getgs fBlockSkillUseMult
getgs fWeaponBlockSkillUseMult
getgs fShieldBashSkillUseBase
getgs fWeaponBashSkillUseBase
getgs fXPPerSkillRank
```
Обязательные по заданию команды для уже найденных записей — сверить runtime с ESP:
```text
getgs fSkillUsageSneakPerSecond
getgs fSkillUsageRechargeMult
getgs fSkillUsageLockPickBroken
```

Имена с `*` — семейства, а не консольные команды: найдены ConstructibleSkillUse (Mult/Exp/Const), TemperingSkillUse (Mult/ItemValMult/ItemValExp/ItemValConst), LockPick (VeryEasy/Easy/Average/Hard/VeryHard/Broken), SneakPerSecond и SneakAttackSkillUsageMelee/Ranged. Полный список фактически существующих записей — GMST TSV.

## Ограничения и воспроизведение

Mutagen 0.54.4 не смог разобрать Effects трёх версий PERK:

- `00080E:Requiem - Special Feats.esp` в Requiem - Special Feats.esp;
- `2474CB:Wintersun - Faiths of Skyrim.esp` в Wintersun - Reqtificated.esp;
- `00082D:Wintersun - Reqtificated.esp` в Wintersun - Reqtificated.esp.

Первые две версии входят в выбранные цепочки и помечены в perk-override-chain.tsv. Третья имеет одну версию и не попала в деревья/NextPerk выборки; ошибка сохранена в raw versions.jsonl и снимке, но отдельной строки для неё в выбранной PERK-таблице нет.

Причина: `PerkEntryPointModifyActorValue did not have expected parameter type flag: Float`. Эти ячейки явно содержат `$error`, decodeErrors помечен. Остальные доступные поля этих версий сохранены. Diff с таким Effects неполон; ошибка не выдаётся за удаление эффектов модом. Для полного декодирования этих трёх записей потребуется xEdit/доработка парсера; в этом проходе эффекты не восстановлены. Пустые переведённые строки (например vanilla sLegendaryResetConfirm в Update.esm) сохранены буквально; отсутствие текста в overlay не доказывает пустую строку в игре. BSA-строки отдельно не извлекались. Соответствие поставленного PSC байткоду PEX отдельно не проверялось.

Исследование статическое: состояние сохранения, победившие упакованные скрипты/ресурсы, внутренности всех DLL и реальная выдача/потеря XP в игре не проверены. Полной формулы алхимии, убийств и тюремной потери опыта здесь нет; значения GMST не заменяют доказательство формулы. Перечни кандидатов по названиям не являются полным семантическим аудитом всех 3946 модов.

`r22-snapshot.json` фиксирует SHA-256 каждого плагина, порядок и статистику без путей установки. `local/r22/versions.jsonl` сохраняет все прочитанные версии PERK/GMST и 18 AVIF. Полные ошибки локальны. Ссылки на запись без переопределения не выдумываются: если Update.esm не содержит AVIF, строки Update для него нет.

Проверено: сборка без предупреждений/ошибок; полный запуск wrapper; независимое восстановление выборки PERK из raw-версий и замыкание NextPerk (983/2418); покрытие GMST (96 версий); 95 версий 18 AVIF; 1435 соседних PERK-diff; 341 конфликт; JSON в ячейках, порядок и единственность победителей; совпадение всех 3492 хешей с основным снимком. Абсолютных путей установки в новых переносимых артефактах не найдено.

Повторить из корня репозитория после подготовки SDK/Mutagen по README:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run-research.ps1 -R22
```
