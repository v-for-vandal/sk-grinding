# §15 — исследование установленного LoreRim

Дата: **2026-11-11**. Объект: `D:\Modlists\LoreRim`, профиль MO2 **Default**,
выбранный в `ModOrganizer.ini`. **LoreRim 5.0 с дополнительными модами** — версия
и профиль подтверждены пользователем. Это снимок конкретной изменённой установки.
Локальный `LoreRim.compiler_settings` содержит техническое `ModlistVersion = 0.0.1.0`,
непригодное для определения релиза. Названия «4.0 Patch» не использовались как
версия всей сборки. Объём работы согласован: **исследование и инструменты**,
реализация телеметрической DLL в эту задачу не входит.

## Что меняет план разработки

1. **Обычная прокачка навыков здесь отключена двумя способами.** У всех 18 основных
   AVIF `UseMult = 0`, `ImproveOffset = 9999`; в настройках Static Skill Leveling
   `bDisableSkillLeveling = 1`. Поэтому текущая сборка не даёт осмысленного baseline
   «сколько часов до навыка 100 от использования». Телеметрия должна различать
   попытку начисления XP, модификаторы и реальное изменение прогресса.
2. **Character XP не только от квестов.** В Experience включены убийства,
   открытия локаций и зачистки. Связь роста персонажа с ростом навыков отключена,
   но формулировка §1 «quests only» неверна для этого профиля.
3. **Материалы брони классифицируются по переименованным Requiem keywords.**
   Например, исходный `ArmorMaterialEbony` теперь называется `REQ_ArmorSet_Ebony`,
   сохраняя FormKey. Конфиг должен ссылаться на plugin + локальный ID, а не
   полагаться только на ванильные EditorID.
4. **Деревья смешанные:** Requiem + Magic Redone + Special Feats + Ordinator +
   патчи LoreRim. Одного исходного `Requiem.esp` для проектирования перков мало.
5. **Существуют дополнительные actor values с деревьями.** Выгрузка находит 20 AVIF
   со структурой Skill, в том числе `AVHealRatePowerMod` и `AVMagickaRateMod`.
   Их нельзя автоматически считать дополнительными обычными навыками. `AVMysticism`
   в этой выгрузке — ванильная Illusion по её FormKey, не отдельная школа Mysticism.

## Метод и границы достоверности

Инструмент: read-only C# overlays Mutagen **0.54.4**, SDK **10.0.404**.
Подготовка и повторный запуск описаны в [README](README.md).
Порядок MO2 учитывается на двух уровнях: победитель файла в левой панели и
последний override записи в порядке активных плагинов. Включены принудительные
masters и Creation Club из `Skyrim.ccc`; проверяется наличие и порядок masters.

Полные данные находятся в `local/records/*.jsonl`; `manifest.json` содержит
SHA-256 источников, порядок чтения и ошибки. Производные таблицы находятся в
[`tables/`](tables/). Условия сохраняют тип функции, RunOn, операции сравнения,
параметры, OR-флаги, а эффекты — entry point, ранг и значение. Таблицы не являются
готовым конфигом распределения XP: они содержат также NPC-only, тестовые и
неполучаемые игроком записи. Частота keyword — число записей, не частота встреч в игре.

Снимок включает **3 946 включённых модов**, **3 492 плагина**, **313 SKSE DLL**
и **82 508 уникальных записей** в 12 группах: 13 140 WEAP, 10 919 ARMO,
8 722 SPEL, 11 885 MGEF, 2 744 PERK, 3 694 KYWD и вспомогательные записи.
Все masters найдены и расположены раньше зависимых файлов. Переносимый список
источников с хешами, числа записей и частичные формы: [snapshot.json](snapshot.json).
Списки модов и DLL: [enabled-mods.tsv](tables/enabled-mods.tsv),
[skse-plugins.tsv](tables/skse-plugins.tsv).

Это статическое исследование. Не запускались игра и CK; загрузка DLL и значения
на конкретном сохранении не проверялись. Список DLL означает наличие победившего
файла, а не доказательство успешной загрузки SKSE. BSA и все скомпилированные PEX
не декомпилировались; runtime-distribution через KID/SPID/SkyPatcher может менять
результат. Различия с живым процессом требуют проверки после загрузки игры.

Три PERK имеют неподдержанное Mutagen представление параметров
`PerkEntryPointModifyActorValue` (`expected ... Float`). Их эффекты неполны:

| Плагин-победитель | FormKey |
|---|---|
| Requiem - Special Feats.esp | `00080E:Requiem - Special Feats.esp` |
| Wintersun - Reqtificated.esp | `2474CB:Wintersun - Faiths of Skyrim.esp` |
| Wintersun - Reqtificated.esp | `00082D:Wintersun - Reqtificated.esp` |

Идентификаторы, описания и условия сохранены. Для этих трёх эффектов нужен xEdit
или отдельное чтение бинарных подзаписей; считать их отсутствующими нельзя.

## R-1 — runtime и SKSE

- `Stock Game/SkyrimSE.exe`: **1.6.1170.0**, по FileVersion исполняемого файла.
- Победитель SKSE поставляет `Root/skse64_1_6_1170.dll`; changelog начинается с
  **2.2.6**, с поддержкой 1.6.1170.
- `Stock Game/CreationKit.exe`: **1.6.1378.1**. Версия CK не равна runtime игры.
- Проверять и собирать будущую DLL следует для **AE 1.6.1170 + SKSE 2.2.6**.
  Адреса/сигнатуры хуков требуют отдельной проверки именно на этом runtime.

Источник: локальные EXE, SKSE Root и `skse64_whatsnew.txt`. Закрыто статически.

## R-2 — XP, книги, тренеры, уровни

Основные включённые компоненты:

| Компонент | Наблюдение / влияние |
|---|---|
| Experience 3.7.3 | Навыки не дают character XP; действует skill cap |
| Static Skill Leveling Rewritten | Распределение навыков при росте уровня; use XP отключён |
| Enhanced Script (FIX) для SSL | Вызывает `IncrementSkillBy`, fallback `AdvanceSkill`, сбрасывает skill XP; при загрузке выставляет skill-use multiplier |
| Norden Starting Skills / Starting Skills | Меняет стартовые навыки; нулевой старт сабскиллов нельзя выводить из названия мода |
| Simply Smart Training 1.1 | Неиспользованные уроки переносятся между уровнями; это не тождественно безлимитному обучению |
| Little Lessons | Дополнительное взаимодействие обычных книг с навыками; обработчик BOOK нужно проверять вместе с этим модом |
| Typography Training, Tome Trials, Beneficial Speech Checks | Дополнительные пути выдачи XP/наград; нужны в таблице интеграций |
| Requiem, Magic Redone, Stealth Redone, Alchemy Redone | Меняют формулы, условия, эффекты и перки |
| LoreRim - Economy Overhaul | Цена обучения и торговли |

Побеждающий `Experience.ini` из `LoreRim - MCM and INI Settings`:

```ini
bEnableKilling = true
bEnableReading = false
bEnableSkillXP = false
iMaxPlayerLevel = 101
fSkillCapBase = 50
fSkillCapMult = 1
bUseRacialCaps = false
```

Это параметры **Experience**, их нельзя интерпретировать как наш main-XP pipeline.
В частности, `bEnableSkillXP=false` отключает вклад навыков в character XP.
Значения открытия локаций и зачисток ненулевые. Настройки SSL: 30 очков навыков
за уровень, максимум 5 повышений одного навыка за уровень, `bDisableSkillLeveling=1`.
Сохранение может содержать другие значения MCM.

Статический `iTrainingNumAllowedPerLevel = 5` (Requiem). Поэтому утверждение
«лимит 5 снят» локальными GMST не подтверждается. Simply Smart Training переносит
остаток; фактический текущий лимит надо посмотреть в новом тестовом сохранении.

Исходник `StaticSkillLevelingQuestScript.psc` в Enhanced Script (FIX), строки
54–59, 101–150, 166–185, прямо подтверждает изменение мультипликаторов и навыков.
Наш обработчик внешних инкрементов не должен превращать распределение SSL в
неограниченную покупку potential. Нужен совместимый режим, отключающий
распределение SSL **в отдельном тестовом профиле** перед внедрением новой прогрессии.

## R-3 — PrismaUI и MCM Helper

MCM Helper **1.6.2** включён; найден `MCMHelper.dll` с той же версией.
PrismaUI среди включённых модов и победивших SKSE DLL не обнаружен.
SKSE Menu Framework включён и остаётся доступным кандидатом для запасного UI.

PrismaUI — отдельная зависимость: автор предоставляет framework и документацию.
Возможность добавления в этот профиль требует пробного открытия/закрытия панели,
проверки ввода, фокуса и существующего HUD. Наличие подходящего релиза не является
доказательством совместимости со всеми 300+ DLL. Собирать сам PrismaUI из исходников
для использования готового релиза не требуется; требования сборки framework нельзя
путать с требованиями установки. [Исходный проект PrismaUI](https://github.com/PrismaUI-SKSE/framework).

## R-4 — оружие

Полный каталог: [weapon-types.tsv](tables/weapon-types.tsv), статистика —
[keyword-usage.tsv](tables/keyword-usage.tsv).

Помимо ванильных типов реально используются quarterstaff, halberd, pike, spear,
javelin, katana, rapier, claw, whip, scythe и отдельные OCF-варианты.
Например, `WeapTypeSpear` существует в нескольких разных плагинах — одинаковый
EditorID не означает одинаковую форму. У предмета могут одновременно быть
ванильный Sword и модовый Rapier; специфичный тип должен иметь приоритет.

Нужные ключи Requiem:

| Группа | FormKey |
|---|---|
| heavy bow | `9F9914:Requiem.esp` — `REQ_WeaponType_BowHeavy` |
| light bow | `9F9915:Requiem.esp` — `REQ_WeaponType_BowLight` |
| heavy crossbow | `899DBE:Requiem.esp` — `REQ_WeaponType_CrossbowHeavy` |
| light crossbow | `9F7179:Requiem.esp` — `REQ_WeaponType_CrossbowLight` |
| wand | `000800:Wand Keywords.esp` — `WeapTypeWand` |

Классификатор: явный per-form override → wand → модовый тип → vanilla type →
weapon data skill/type → `other`. Для bow/crossbow отдельный проверенный keyword
определяет тяжесть; не угадывать её по весу. У шаблонных WEAP нужно учитывать
унаследованные свойства, а не считать пустой список keywords окончательным.
Родитель 1H/2H должен опираться также на итоговый weapon data skill.

## R-5 — броня и материалы

Каталог: [armor-materials.tsv](tables/armor-materials.tsv).
Исходные и итоговые имена всех keywords, включая оружие и магию:
[keyword-aliases.tsv](tables/keyword-aliases.tsv). Исходный EditorID взят из
определяющего плагина, итоговый — из последнего override; история переименований
сохранена. В остальных таблицах изменённые ссылки дополнены `{original: ...}`.
Это словарь для переносимых правил между коллекциями; старое имя само по себе
не становится доступным через runtime-поиск EditorID. Подробнее о происхождении
и ограничениях данных — [README](README.md#исходные-keywords-и-переносимость).
В этой сборке широко используются `REQ_ArmorSet_*`, например:

| Материал/семейство | FormKey |
|---|---|
| Ebony | `06BBD8:Skyrim.esm` |
| Steel / SteelPlate | `06BBE6:Skyrim.esm` / `06BBE7:Skyrim.esm` |
| DwarvenHeavy / DwarvenLight | `06BBD7:Skyrim.esm` / `ADDD6F:Requiem.esp` |
| OrcishHeavy / OrcishLight | `06BBE5:Skyrim.esm` / `ADDD9E:Requiem.esp` |
| ChitinLight / ChitinHeavy | `024102:Dragonborn.esm` / `024103:Dragonborn.esm` |
| Quicksilver | `024105:Dragonborn.esm` |

Также обнаружены Chainmail, NetchLeather, Fur, Guard, AncientNord, Akaviri,
Vigilant, SilverHand, варианты Dawnguard, Ebony_Light и VampireHeavy.
Ключи `REQ_LEGACY_*` и `REQ_NULL_*` нельзя автоматически считать равнозначными
новым семействам. Семейство набора не всегда является одним физическим материалом.
В частности, Quicksilver нельзя механически подписать как Stalhrim по старой таблице.

Для брони определять Heavy/Light/Clothing по BodyTemplate, слот — по slot mask,
материал — по конфигу FormKey. Для Smithing сопоставление набор → материал должно
сверяться с COBJ/ингредиентами. Решение о слиянии hide/leather, steel/plate и новых
семейств остаётся дизайнерским Q-SMI-5/Q-ARM-3; исследование не назначает tier и T.

## R-6 — Block и парирования

Статические источники/механики: обычный блок щитом/оружием, bash/power bash,
перки блокирования стрел и стихийного урона, Shield Charge, Torch Combat.
Включены Precision **2.0.4**, Parrying RPG **2.0.1**, Dual Wield Parrying NG
**1.0.1**, MCO Block Recovery и моды block cancel/recovery.
`blockOverhaul.ini` включает mageBlock и mageWard; это не доказывает выдачу Block XP.

У Parrying RPG успешное столкновение атак прерывает обычную обработку melee hit.
В исследованном обработчике нет явного начисления skill XP. Следовательно,
successful parry нельзя приравнивать к событию vanilla block; его XP потребует
отдельного адаптера, если этот вид парирования включён в дизайн.
[Исходник обработчика](https://github.com/DennisSoemers/ParryingRPG/blob/main/src/OnMeleeHit.cpp).

Таблица перков содержит условия и entry points Block. Она подтверждает защитные
эффекты, но не частоту engine XP callbacks. Минимальный runtime-тест: по одному
обычному блоку, weapon block, bash, power bash, arrow block, elemental block,
ward, collision parry. Сопоставить события и XP; исключить двойное начисление.
**Точные awards каждого случая остаются runtime-проверкой.**

## R-7 и R-8 — магия и направления школ

Данные: [magic-effects.tsv](tables/magic-effects.tsv),
[spell-effects.tsv](tables/spell-effects.tsv), [perk-trees.tsv](tables/perk-trees.tsv).
В SPEL keywords часто отсутствуют: основная семантика находится в MGEF.
Движковый `MagicEffect.Archetype` (SummonCreature, ValueModifier и т. п.) — другой
смысл слова archetype, чем дизайнерские druid/shaman/warlock.

Подтверждён `Traits_DruidicSpell = 00007B:Biggie Traits.esp`; Triumvirate использует
семейства `TVR_*`, включая Calling, Balefire, Hex, Holy, Shadows, Totem.
Единой симметричной группы `MagicArchetypeDruid/Shaman/...` не обнаружено.
Поэтому архетипы требуют явных списков keywords/форм; происхождение spell из
Triumvirate само по себе не является точной классификацией архетипа.

Направления, подтверждённые фактическими ветвями и условиями:

| Школа | Семейства направлений |
|---|---|
| Alteration | mage armor/protection, transmutation, kinetomancy/telekinesis, weather; metamagic отдельно от формы заклинания |
| Conjuration | bound weapon/armor, necromancy, daedric, spirit, soul magic |
| Destruction | fire, frost, shock, arcane, entropic, blood; cloaks/battlemage — отдельная поперечная категория |
| Illusion (`AVMysticism`) | perceptual, emotional/command, delusive/pain, shadow |
| Restoration | healing, protection/wards, turning, heliomancy/sun, venomancy/poison; recovery — пассивная ветвь |

Примеры реальных ключей Magic Redone:

- `005B00` / `005B01`: `Nox_KW_Destruction_Arcane` / `..._Entropic`;
- `005DE4` / `005E72`: `Nox_KW_Conjuration_BoundWeapon` / `..._BoundArmor`;
- `005F21`: `Nox_KW_Conjuration_Necrotic`;
- `005C22`: `Nox_KW_Restoration_Poison`;
- `005DAB` / `005C64`: `Nox_KW_Alteration_Transmutation` / `..._Telekinetic`;
- `005EC9` / `005F04`: `Nox_KW_Illusion_Command` / `..._ShadowSummon`.

Все ID в этом списке относятся к **Requiem - Magic Redone.esp**. Используются и
vanilla `MagicDamageFire/Frost/Shock`, `MagicRestoreHealth`, `MagicSummon*`.
Некоторые ветви проверяют альтернативные условия, perk-specific keywords и
исключения NoMagnitude/NoDurationScaling. Нельзя заменить всё одной проверкой школы.
Полная структура условий сохранена в таблице, включая OR и RunOnTabIndex.

Рекомендуемый mapping: определять effect → направление, затем применять решение
Q-MAG-2 для многокомпонентных spells. При неизвестном effect оставлять `school/other`.
Разбиение направлений и доли XP — предложения дизайна, не извлечённые факты.

## R-9 — summons/thralls

Magic Redone уже меняет длительность/силу undead, daedra и spirits; Cognitive
Flexibility описывает лимит 2/3 сущностей, включая другие школы. Shadow summons
Illusion имеют отдельные перки и дополнительные лимиты. Enhanced Reanimation
**1.5.1.1.0** включён: в INI активны fast-travel fix, reanimation decapitated NPCs,
necromancer cast и patch horses.

В доступных исходниках Shadow Clone создаётся `PlaceActorAtMe`, добавляется в
teammate/faction и удаляется по завершении эффекта. Это важное исключение:
поиск только engine summoned creatures не охватит все сущности дизайна.
Dark Channeling, напротив, даёт регенерацию magicka при daedra summons.

Найден конкретный кандидат существующей цены thrall: `NecroDeadThrallCost`
(`0DE4AE:Undeath.esp`) содержит скрипт `Mycastanotherspellscript`, параметр
`basecost=1000` и ссылки `RavageMagickaCost`, `RavageMagickaCostPerked`,
`RestoreMagickaCost`, `MultiSoulsCost`. Это свидетельство отдельной скриптовой
механики Undeath, но по названиям свойств нельзя доказать периодичность списания,
размер резервируемой magicka или достижимость эффекта игроком.

**Единый постоянный magicka upkeep для всех summons/thralls не подтверждён.**
Отсутствие такого подтверждения не доказывает отсутствие скриптов в BSA/PEX.
Перед реализацией нового upkeep нужно снять активные эффекты с обычного summon,
Dead Thrall, shadow clone и bound item, проверить magicka/regen/maximum и завершение
при смерти, dispel и загрузке. Различать постоянный thrall, временный summon,
созданный скриптом clone и follower.

## R-10 — staffs/wands

Ветка **Enchanting**, а не отдельное дерево Staff. Подтверждены:

- Arcane Artificery I–III: сила staffs/scrolls и расход зарядов;
- Staff Channeling I–II: усиление собственных spells при экипированном staff;
- Wand Mastery: `000803:Wand Keywords.esp`, требование base Enchanting ≥50 и
  Staff Channeling I; ability — `000802:Wand Keywords.esp`;
- Soul Gem Mastery, Arcane Experimentation, Artificer's Insight: создание staves;
- Charge Tap и You Shall Not Pass из Ordinator.

Полные purchase/effect conditions находятся в perk-trees. Staff Channeling
использует проверки экипированного типа, поэтому одного keyword WeapTypeStaff
для воспроизведения всех эффектов недостаточно. Wands могут иметь одновременно
staff и wand keywords; классификатор должен выбирать wand первым.

## R-11 — инструменты, skills, MCP

Найдены реальные исходные проекты MCP:

| Проект | Заявленная область | Оценка для этой задачи |
|---|---|---|
| [Ciltress/skyrim-modding-tools-mcp](https://github.com/Ciltress/skyrim-modding-tools-mcp) | MO2/Amethyst, записи, патчи, Papyrus, crash logs | Кандидат для интерактивного исследования; локально не установлен/не проверен |
| [zsliwoski/skyrim-mcp-tools](https://github.com/zsliwoski/skyrim-mcp-tools) | Чтение/редактирование ESP, quests/dialogue, Papyrus | Кандидат для авторинга; не доказывает поддержку C++ hooks |
| [nawnie/creation-kit-mcp-v2](https://github.com/nawnie/creation-kit-mcp-v2) | Репозиторий CK MCP | Найден, но возможности и корректность не валидировались |

Специализированного подключённого MCP для SKSE/CommonLib в этой сессии нет.
Существование репозитория не равно его доступности в текущем агенте. Установка
дополнительного MCP для данного исследования не потребовалась: воспроизводимая
выгрузка Mutagen уже работает. Готовой установленной skill именно для Skyrim/CK
в списке текущей сессии нет. CommonLib следует использовать как зависимость C++
с фиксированным commit; пример [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG).
Эта работа не является проверкой всех операций найденных серверов.

## R-12 — custom skill frameworks

В профиле включены **Custom Skills Framework 3.1.0** и Custom Skills Menu.
Это практическое подтверждение использования, более полезное для совместимости,
чем количество GitHub stars. У автора есть релиз **3.2.0**, ориентированный также
на более новый runtime; автоматически обновлять имеющуюся зависимость нельзя.
[Релизы CSF](https://github.com/Exit-9B/CustomSkills/releases).

Для нашего интерфейса CSF всё равно не предоставляет готовую модель
floor/current/potential/peak и таблицу 120+ профилей: хранение и динамика остаются
за нашей DLL. Custom Skills Menu — оболочка/объединение деревьев, не замена
хранилища состояния. Альтернатива для данных — собственное co-save состояние;
для представления — PrismaUI либо SKSE Menu Framework.

Обнаружен также [Actor Value Generator](https://github.com/NoahBoddie/actor-value-generator)
с моделью дополнительных значений. Это возможный отдельный эксперимент, а не
проверенная drop-in замена CSF/UI и не обязательная зависимость проекта.
Сопоставимых достоверных метрик популярности всех альтернатив не получено;
приписывать им рейтинг без downloads/dependents и общей даты нельзя.

## R-13 — collision/projectile hooks

Precision **2.0.4** включён. Его API предоставляет pre-hit/post-hit callbacks и
данные удара; это предпочтительная точка интеграции физического melee до
добавления собственного конкурирующего перехвата.
[PrecisionAPI.h](https://github.com/ersh1/Precision/blob/main/src/PrecisionAPI.h).

Parrying RPG перехватывает melee hit call site; нельзя предполагать, что отменённый
им удар дойдёт до стандартного TESHitEvent. True Directional Movement **2.2.5**
включён. В его исходниках есть ProjectileAimSupport, InitProjectile, обработка
velocity и beam target pick. Это **не доказательство хука именно Projectile::Launch**,
но подтверждение потенциального конфликта: predict/homing может изменить направление
после начального launch spread. [TDM Hooks.cpp](https://github.com/ersh1/TrueDirectionalMovement/blob/master/src/Hooks.cpp).

Для aim perturbation нужны проверки free aim / target lock predict / homing,
1st/3rd person и mounted casting. Точное совпадение binary hooks всех 300+ DLL
не установлено; для него нужны исходники конкретных версий либо runtime-анализ.
Свободный адрес `Projectile::Launch` по одному списку имён DLL доказать нельзя.

## R-14 — значения навыков выше 100

Из итоговых GMST:

| Настройка | Значение | Что действительно ограничивает |
|---|---:|---|
| fSneakPerceptionSkillMax | 100 | Граница параметра sneak perception; не общий cap AV |
| fPickPocketMaxChance / MinChance | 100 / 0 | Вероятность, а не уровень навыка |
| fMaxArmorRating | 75 | Предел механики armor reduction; не Heavy/Light Armor level |
| fBlockMax | 1 | Предел блокирования, а не Block level |
| fBarterBuyMin / SellMax | 2 / 0.5 | Границы цен, а не Speech level |
| fBarterMax / Min | 4 / 3 | Параметры торговли; поведение зависит также от перков/DLL |
| fDamageSkillMin / Max | 1 / 1 | Ванильное масштабирование оружия заменено Requiem-логикой |
| fMagicCasterPCSkillCostBase | 0 | Настройка Magic Redone; ванильную формулу переносить нельзя |

Таблица [game-settings.tsv](tables/game-settings.tsv) содержит источники значений.
Достижение порога 100 в условии перка — **не clamp**. Аналогично, меню 0–100 и
фактическая возможность положительного AV >100 — разные вещи.

Experience дополнительно ограничивает рост навыков своим cap, зависящим от уровня
персонажа; максимальный уровень персонажа в INI 101. Parrying RPG включает weapon
skill в score с весом 1, так что значение >100 способно продолжать влиять на парирование.
Следовательно, утверждения «всё обрезается на 100» и «всё безопасно выше 100»
одинаково не подтверждены.

Практическое предложение для первого прототипа: **main skills 0–100**, сабскиллы
хранить независимо 0–200. Это рекомендация, Q-DYN-8 остаётся открытым.
При выборе main >100 нужны runtime-пробы 99/100/101/150/200: стоимость spells,
торговля, pickpocket, sneak detection, armor/block, weapon damage, crafting и
отображение меню. Снимать итоговые числа, доступность perks и отрицательные/NaN
значения; статическая выгрузка не доказывает формулы машинного кода игры.

## §15.1 — корректный следующий шаг для pacing

Числа «50–100 часов до 100» из draft не верифицированы и не использовались.
Из файлов нельзя получить XP/hour: это зависит от реальных действий игрока.
Для текущего профиля use-based baseline до 100 отсутствует по самой конфигурации.

Предлагаемый протокол будущей телеметрии:

1. Отдельный тестовый MO2-профиль и новое сохранение. Зафиксировать load-order
   hashes, AVIF, GMST и runtime-модификаторы.
2. Режим observer ничего не меняет: пишет входной award и результат, skill,
   game time, monotonic real time, current XP/level/cap, use multiplier, состояние
   меню/паузы, контекст weapon/spell/target и источник, если его можно доказать.
3. Не приписывать контекст по «последнему spell cast»: периодические эффекты,
   summons и одновременные попадания делают такую атрибуцию неоднозначной.
4. Отдельно решать, нужен ли эксперимент с восстановленным use XP. Отключение SSL
   само по себе недостаточно: AVIF уже содержат нули и offset 9999. Такая сессия
   должна называться модифицированным reference-профилем, а не неизменённым LoreRim.
5. Несколько обычных игровых сессий: считать XP на active real hour, распределения
   по активностям/навыкам и длительности. Не смешивать реальные и игровые часы.

Телеметрическая DLL по согласованному объёму работы не реализуется; Q-RES-1
не отвечен за пользователя. До выбора режима и игровых сессий численные цели
Q-XP-7 и нормализация по реальной скорости остаются открытыми.

## Что ещё требует игры или дополнительного разбора

- Проверить три неполных эффекта PERK в xEdit.
- Подтвердить реальные Block XP callbacks, лимит обучения и runtime skill caps.
- Проверить summons/thralls и возможный upkeep из BSA/PEX/runtime effects.
- Проверить PrismaUI и взаимодействие aim hooks с TDM/Precision.
- Если main >100 выбран: выполнить численные пробы R-14.
- Для pacing: реализовать observer, затем собрать несколько игровых сессий.

Все R-1…R-14 рассмотрены выше; runtime-зависимые пункты не объявляются закрытыми
только потому, что их статические источники найдены. Изменения сборки и сохранений
для этого исследования не выполнялись.
