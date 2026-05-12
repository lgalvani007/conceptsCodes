#pragma once

#include <cstdint>

enum class ComponentDirection {
  Left = 0,
  Right = 1
};

inline ComponentDirection oppositeDirection(ComponentDirection direction) {
  return static_cast<ComponentDirection>(static_cast<uint8_t>(direction) ^ 1);
}
enum class ComponentLength {
  Short = 0,
  Normal = 1,
  Long = 2
};
