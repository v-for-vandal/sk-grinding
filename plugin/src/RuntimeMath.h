#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace sp {
// In the shield-bash log, UseSkill and its hit flag were <2 ms apart; deferred
// worker calls were within ~9 ms. A 50 ms correlation window allows either
// ordering without treating the whole 750 ms context window as one bash.
// Prefer a nearer blocked hit if both exist; simultaneous events are ambiguous.
inline bool isBashContext(std::uint64_t use, std::uint64_t bash,
                          std::uint64_t block) {
  const auto distance = [use](std::uint64_t event) {
    return event ? (event > use ? event - use : use - event)
                 : std::numeric_limits<std::uint64_t>::max();
  };
  return distance(bash) <= 50000 && distance(bash) < distance(block);
}

// Skyrim's horizontal forward axis is +Y, yaw rotates towards +X.
// Rotate direction while retaining speed; scaling components independently
// would change flight speed/gravity behaviour as well as accuracy.
inline std::array<double, 3> rotateVelocity(double x, double y, double z,
                                            double pitch, double yaw) {
  const auto speed = std::sqrt(x * x + y * y + z * z);
  const auto p = std::atan2(z, std::hypot(x, y)) + pitch;
  const auto a = std::atan2(x, y) + yaw;
  return {speed * std::cos(p) * std::sin(a), speed * std::cos(p) * std::cos(a),
          speed * std::sin(p)};
}
}  // namespace sp
