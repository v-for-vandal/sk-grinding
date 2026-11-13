#pragma once
#include <string_view>
namespace sp {
struct SpellShape {
  enum class Delivery { Other, Self, Touch, Actor, Location };
  Delivery delivery = Delivery::Other;
  bool summon = false, cloak = false, concentration = false, beam = false,
       flame = false, barrier = false, missile = false, explosion = false;
};
inline std::string_view spellForm(const SpellShape& s) {
  using D = SpellShape::Delivery;
  if (s.summon) return "";
  if (s.cloak) return "cloak";
  if (s.delivery == D::Self) return "self";
  if (s.delivery == D::Touch) return "touch";
  if (s.delivery == D::Actor) return "direct";
  if (s.delivery == D::Location) return s.concentration ? "wall" : "rune";
  if (s.beam) return "beam";
  if (s.flame) return "flame";
  if (s.barrier) return "wall";
  if (s.missile) return s.explosion ? "explosion" : "missile";
  return "";
}
}  // namespace sp
