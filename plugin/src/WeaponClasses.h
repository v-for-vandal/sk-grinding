#pragma once
#include <array>
#include <string_view>
namespace sp {
struct WeaponClass {
  std::string_view keyword, sub;
};
// Sorted by EditorID; record IDs are 0x800 + row (tools/esp-gen).
inline constexpr std::array<WeaponClass, 22> weaponClasses{
    {{"SPO_WeapClass_1H_Dagger", "1h-dagger"},
     {"SPO_WeapClass_1H_Mace", "1h-mace"},
     {"SPO_WeapClass_1H_Other", "1h-other"},
     {"SPO_WeapClass_1H_Scythe", "1h-scythe"},
     {"SPO_WeapClass_1H_ShortSword", "1h-short-sword"},
     {"SPO_WeapClass_1H_Spear", "1h-spear"},
     {"SPO_WeapClass_1H_Sword", "1h-sword"},
     {"SPO_WeapClass_1H_Unarmed", "1h-unarmed"},
     {"SPO_WeapClass_1H_WarAxe", "1h-war-axe"},
     {"SPO_WeapClass_2H_Battleaxe", "2h-battleaxe"},
     {"SPO_WeapClass_2H_Other", "2h-other"},
     {"SPO_WeapClass_2H_Polearm", "2h-polearm"},
     {"SPO_WeapClass_2H_Quarterstaff", "2h-quarterstaff"},
     {"SPO_WeapClass_2H_Scythe", "2h-scythe"},
     {"SPO_WeapClass_2H_Sword", "2h-sword"},
     {"SPO_WeapClass_2H_Warhammer", "2h-warhammer"},
     {"SPO_WeapClass_Archery_HeavyBow", "archery-heavy-bow"},
     {"SPO_WeapClass_Archery_HeavyCrossbow", "archery-heavy-crossbow"},
     {"SPO_WeapClass_Archery_LightBow", "archery-light-bow"},
     {"SPO_WeapClass_Archery_LightCrossbow", "archery-light-crossbow"},
     {"SPO_WeapClass_Archery_Other", "archery-other"},
     {"SPO_WeapClass_Casting", ""}}};
}  // namespace sp
