#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Component.hpp"
#include "RoutineGroups.hpp"

class RoutineManager {
 public:
  static RoutineManager& getInstance() {
    static RoutineManager instance;
    return instance;
  }

  RoutineManager(const RoutineManager&) = delete;
  void operator=(const RoutineManager&) = delete;

  template <typename Func>
  void registerRoutine(GroupId group, std::string_view name, const Func& routine) {
    size_t variations = 0;
    auto addRoutine = [&](std::string fullName, RoutineFunction variantFunc, ComponentLength l, ComponentDirection d) {
      Routines.push_back({std::move(fullName), std::move(variantFunc), d, l});
      ++variations;
    };
    size_t startIndex = Routines.size();
    if constexpr (std::is_invocable_v<Func, ComponentLength, ComponentDirection> || std::is_invocable_v<Func, ComponentDirection, ComponentLength>) {
      for (size_t d = 0; d < dirs.size(); ++d) {
        for (size_t l = 0; l < lengths.size(); ++l) {
          std::string fullName = std::string(name) + dir_suffix[d] + len_suffix[l];
          if constexpr (std::is_invocable_v<Func, ComponentLength, ComponentDirection>) {
            addRoutine(std::move(fullName), BothRoutineFunction(routine), lengths[l], dirs[d]);
          } else {
            addRoutine(std::move(fullName), BothReverseRoutineFunction(routine), lengths[l], dirs[d]);
          }
        }
      }
    } else if constexpr (std::is_invocable_v<Func, ComponentLength>) {
      for (size_t l = 0; l < lengths.size(); ++l) {
        addRoutine(std::string(name) + len_suffix[l],
                   DistanceRoutineFunction(routine), lengths[l], dirs[0]);
      }
    } else if constexpr (std::is_invocable_v<Func, ComponentDirection>) {
      for (size_t d = 0; d < dirs.size(); ++d) {
        addRoutine(std::string(name) + dir_suffix[d],
                   SideRoutineFunction(routine), lengths[0], dirs[d]);
      }
    } else if constexpr (std::is_invocable_v<Func>) {
      addRoutine(std::string(name),
                 NoneRoutineFunction(routine), lengths[0], dirs[0]);
    } else {
      static_assert(!std::is_same_v<Func, Func>, "Invalid routine function signature");
    }
    BaseRoutines.push_back({std::string(name), Routines.size() - variations, variations});
    Groups[static_cast<size_t>(group)].baseIndices.push_back(BaseRoutines.size() - 1);
  }

  // ***************
  // Dumping Routines and Groups for Debugging
  // ***************
  void dumpRoutines() const {
    size_t index = 0;
    for (const auto& routine : Routines) {
      std::cout << "[" << index++ << "] " << routine.name << " (Direction: " << static_cast<int>(routine.direction)
                << ", Length: " << static_cast<int>(routine.length) << ")\n";
    }
  }

  void dumpGroups() const {
    for (size_t i = 0; i < Groups.size(); ++i) {
      std::cout << "Group " << i << ":\n";
      for (size_t baseIndex : Groups[i].baseIndices) {
        std::cout << "  - " << BaseRoutines[baseIndex].name << "\n";
      }
    }
  }

  void dumpRoutinesByGroup(GroupId group) const {
    size_t groupIndex = static_cast<size_t>(group);
    if (groupIndex >= Groups.size()) return;

    std::cout << "Routines in Group " << groupIndex << ":\n";
    for (size_t baseIndex : Groups[groupIndex].baseIndices) {
      std::cout << "  - " << BaseRoutines[baseIndex].name << "\n";
    }
  }

  void dumpBaseRoutines() const {
    for (size_t i = 0; i < BaseRoutines.size(); ++i) {
      std::cout << "[" << i << "] " << BaseRoutines[i].name
                /*<< "Real start index: " << BaseRoutines[i].startIndex*/
                << ", Variations: " << BaseRoutines[i].variationCount << "\n";
    }
  }

  void dumpBaseRoutineByGroup(GroupId group) const {
    size_t groupIndex = static_cast<size_t>(group);
    if (groupIndex >= Groups.size()) return;

    std::cout << "Base Routines in Group " << groupIndex << ":\n";
    for (size_t baseIndex : Groups[groupIndex].baseIndices) {
      std::cout << "  - [" << baseIndex << "] " << BaseRoutines[baseIndex].name
                /*<< "Real start index: " << BaseRoutines[baseIndex].startIndex*/
                << ", Variations: " << BaseRoutines[baseIndex].variationCount << "\n";
    }
  }

  // ***************
  // Getters
  // ***************
  std::string_view getRoutineName(size_t index) const {
    if (index >= Routines.size()) return {};
    return Routines[index].name;
  }

  std::string_view getBaseRoutineName(size_t baseIndex) const {
    if (baseIndex >= BaseRoutines.size()) return {};
    return BaseRoutines[baseIndex].name;
  }

  std::string_view getGroupName(GroupId group) const {
    size_t groupIndex = static_cast<size_t>(group);
    if (groupIndex >= Groups.size()) return {};
    return GroupNames[groupIndex];
  }

  size_t getRoutineCount() const {
    return Routines.size();
  }

  size_t getBaseRoutineCount() const {
    return BaseRoutines.size();
  }

  size_t getGroupCount() const {
    return Groups.size();
  }

  size_t getRoutineVariantionsCount(size_t baseIndex) const {
    if (baseIndex >= BaseRoutines.size()) return 0;
    return BaseRoutines[baseIndex].variationCount;
  }

  size_t getRoutineIndex(std::string_view name) const {
    for (size_t i = 0; i < Routines.size(); ++i) {
      if (Routines[i].name == name) return i;
    }
    return static_cast<size_t>(-1);
  }

  size_t getRoutineIndexByVariation(size_t baseIndex, size_t variation) const {
    if (baseIndex >= BaseRoutines.size()) return static_cast<size_t>(-1);
    size_t variationCount = BaseRoutines[baseIndex].variationCount;
    if (variation >= variationCount) return static_cast<size_t>(-1);

    size_t startIndex = BaseRoutines[baseIndex].startIndex;
    const auto& baseRoutine = Routines[startIndex];
    return startIndex + variation;
  }

  // ***************
  // Relative Getters (Relative to Group)
  // ***************

  std::string_view getRoutineName(size_t relativeIndex, GroupId group) const {
    // This function is used when you want all routines without baseRoutine filter
    if (group >= GroupId::NUM_ROUTINE_GROUPS) return {};
    const auto& groupData = Groups[static_cast<size_t>(group)];
    for (size_t index : groupData.baseIndices) {
      const auto& baseRoutine = BaseRoutines[index];
      if (relativeIndex < baseRoutine.variationCount) {
        return getRoutineName(baseRoutine.startIndex + relativeIndex);
      }
      relativeIndex -= baseRoutine.variationCount;
    }
    return {};
  }

  std::string_view getRoutineName(size_t relativeBaseIndex, size_t variationIndex, GroupId group) const {
    // This function is used when you want to get routine with baseRoutine filter
    if (group >= GroupId::NUM_ROUTINE_GROUPS) return {};
    const auto& groupData = Groups[static_cast<size_t>(group)];

    if (relativeBaseIndex >= groupData.baseIndices.size()) return {};

    size_t actualBaseIndex = groupData.baseIndices[relativeBaseIndex];
    const auto& baseRoutine = BaseRoutines[actualBaseIndex];

    if (variationIndex >= baseRoutine.variationCount) return {};

    return getRoutineName(baseRoutine.startIndex + variationIndex);
  }

  std::string_view getBaseRoutineName(size_t relativeBaseIndex, GroupId group) const {
    if (group >= GroupId::NUM_ROUTINE_GROUPS) return {};
    const auto& groupData = Groups[static_cast<size_t>(group)];

    if (relativeBaseIndex >= groupData.baseIndices.size()) return {};

    size_t actualBaseIndex = groupData.baseIndices[relativeBaseIndex];
    return getBaseRoutineName(actualBaseIndex);
  }

  size_t getRoutineVariantionsCount(size_t relativeBaseIndex, GroupId group) const {
    if (group >= GroupId::NUM_ROUTINE_GROUPS) return 0;
    const auto& groupData = Groups[static_cast<size_t>(group)];

    if (relativeBaseIndex >= groupData.baseIndices.size()) return 0;

    size_t actualBaseIndex = groupData.baseIndices[relativeBaseIndex];
    const auto& baseRoutine = BaseRoutines[actualBaseIndex];
    return baseRoutine.variationCount;
  }

  size_t getRoutineIndexByVariation(size_t relativeBaseIndex, size_t variationIndex, GroupId group) const {
    if (group >= GroupId::NUM_ROUTINE_GROUPS) return static_cast<size_t>(-1);
    const auto& groupData = Groups[static_cast<size_t>(group)];

    if (relativeBaseIndex >= groupData.baseIndices.size()) return static_cast<size_t>(-1);

    size_t actualBaseIndex = groupData.baseIndices[relativeBaseIndex];
    const auto& baseRoutine = BaseRoutines[actualBaseIndex];

    if (variationIndex >= baseRoutine.variationCount) return static_cast<size_t>(-1);

    return baseRoutine.startIndex + variationIndex;
  }

  void run(size_t index) {
    if (index >= Routines.size()) return;

    const auto& routine = Routines[index];
    std::visit([&](const auto& func) {
      using T = std::decay_t<decltype(func)>;
      if constexpr (std::is_same_v<T, DistanceRoutineFunction>) {
        func(routine.length);
      } else if constexpr (std::is_same_v<T, SideRoutineFunction>) {
        func(routine.direction);
      } else if constexpr (std::is_same_v<T, BothRoutineFunction>) {
        func(routine.length, routine.direction);
      } else if constexpr (std::is_same_v<T, BothReverseRoutineFunction>) {
        func(routine.direction, routine.length);
      } else if constexpr (std::is_same_v<T, NoneRoutineFunction>) {
        func();
      }
    },
               routine.function);
  }

 private:
  RoutineManager() = default;

  using DistanceRoutineFunction = std::function<void(ComponentLength)>;
  using SideRoutineFunction = std::function<void(ComponentDirection)>;
  using BothRoutineFunction = std::function<void(ComponentLength, ComponentDirection)>;
  using BothReverseRoutineFunction = std::function<void(ComponentDirection, ComponentLength)>;
  using NoneRoutineFunction = std::function<void()>;
  using RoutineFunction = std::variant<DistanceRoutineFunction, SideRoutineFunction, BothRoutineFunction, BothReverseRoutineFunction, NoneRoutineFunction>;

  struct Routine {
    std::string name;
    RoutineFunction function;
    ComponentDirection direction;
    ComponentLength length;
  };

  struct BaseRoutine {
    std::string name;
    size_t startIndex;
    size_t variationCount;
  };

  struct RoutineGroup {
    std::vector<size_t> baseIndices;
  };

  std::vector<Routine> Routines;
  std::vector<BaseRoutine> BaseRoutines;
  std::array<RoutineGroup, static_cast<size_t>(GroupId::NUM_ROUTINE_GROUPS)> Groups;

  static constexpr std::array<ComponentLength, 3> lengths = {ComponentLength::Short, ComponentLength::Normal, ComponentLength::Long};
  static constexpr std::array<ComponentDirection, 2> dirs = {ComponentDirection::Left, ComponentDirection::Right};

  static constexpr std::array<const char*, 3> len_suffix = {" C", " M", " L"};
  static constexpr std::array<const char*, 2> dir_suffix = {" E", " D"};
};