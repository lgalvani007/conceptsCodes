#pragma once
#include <array>
#include <cstdint>
#include <string_view>

enum class GroupId : uint8_t {
  INITIAL_ROUTINE,
  CALIBRATION_ROUTINE,
  BLIND_ROUTINE,
  NUM_ROUTINE_GROUPS
};

constexpr std::array<std::string_view, static_cast<size_t>(GroupId::NUM_ROUTINE_GROUPS)> GroupNames = {
    "Initial Routine",
    "Calibration Routine",
    "Blind Routine"};

constexpr std::string_view getGroupName(GroupId id) {
  if (id >= GroupId::NUM_ROUTINE_GROUPS) return "Unknown Group";
  return GroupNames[static_cast<size_t>(id)];
}