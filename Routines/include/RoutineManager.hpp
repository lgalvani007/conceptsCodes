#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "Component.hpp"
#include "Routines.hpp"

class RoutineManager {
 public:
  template <typename Func>
  void registerRoutine(std::string_view name, const Func& routine) {
    auto addRoutine = [&](std::string fullName, RoutineFunction variantFunc, ComponentLength l, ComponentDirection d) {
      Routines.push_back({std::move(fullName), std::move(variantFunc), d, l});
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
    size_t generatedCount = Routines.size() - startIndex;
    BaseRoutines.push_back({std::string(name), startIndex, generatedCount});
  }

  void dumpRoutines() const {
    size_t index = 0;
    for (const auto& routine : Routines) {
      std::cout << "[" << index++ << "] " << routine.name << " (Direction: " << static_cast<int>(routine.direction)
                << ", Length: " << static_cast<int>(routine.length) << ")\n";
    }
  }

  void dumpBaseRoutines() const {
    for (size_t i = 0; i < BaseRoutines.size(); ++i) {
      std::cout << "[" << i << "] " << BaseRoutines[i].name
                /*<< "Real start index: " << BaseRoutines[i].startIndex*/
                << ", Variations: " << BaseRoutines[i].variationCount << "\n";
    }
  }

  std::string_view getRoutineName(size_t index) const {
    if (index >= Routines.size()) return {};
    return Routines[index].name;
  }

  std::string_view getBaseRoutineName(size_t baseIndex) const {
    if (baseIndex >= BaseRoutines.size()) return {};
    return BaseRoutines[baseIndex].name;
  }

  size_t getRoutineCount() const {
    return Routines.size();
  }

  size_t getBaseRoutineCount() const {
    return BaseRoutines.size();
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

  size_t getRoutineIndexByParams(size_t baseIndex, size_t direction = 0, size_t length = 0) const {
    if (baseIndex >= BaseRoutines.size()) return static_cast<size_t>(-1);

    size_t startIndex = BaseRoutines[baseIndex].startIndex;
    const auto& baseRoutine = Routines[startIndex];

    size_t variationOffset = 0;
    std::visit([&](const auto& func) {
      using T = std::decay_t<decltype(func)>;
      if constexpr (std::is_same_v<T, DistanceRoutineFunction>) {
        variationOffset = length;
      } else if constexpr (std::is_same_v<T, SideRoutineFunction>) {
        variationOffset = direction;
      } else if constexpr (std::is_same_v<T, BothRoutineFunction> || std::is_same_v<T, BothReverseRoutineFunction>) {
        variationOffset = (direction * lengths.size()) + length;
      }
    },
               baseRoutine.function);

    size_t finalIndex = startIndex + variationOffset;
    if (finalIndex >= Routines.size()) return static_cast<size_t>(-1);

    return finalIndex;
  }

  size_t getRoutineIndexByVariation(size_t baseIndex, size_t variation) const {
    if (baseIndex >= BaseRoutines.size()) return static_cast<size_t>(-1);

    size_t variationCount = BaseRoutines[baseIndex].variationCount;
    if (variation >= variationCount) return static_cast<size_t>(-1);

    size_t startIndex = BaseRoutines[baseIndex].startIndex;
    const auto& baseRoutine = Routines[startIndex];

    size_t direction = 0, length = 0;

    std::visit([&](const auto& func) {
      using T = std::decay_t<decltype(func)>;

      if constexpr (std::is_same_v<T, DistanceRoutineFunction>) {
        length = variation;
      } else if constexpr (std::is_same_v<T, SideRoutineFunction>) {
        direction = variation;
      } else if constexpr (std::is_same_v<T, BothRoutineFunction> || std::is_same_v<T, BothReverseRoutineFunction>) {
        length = variation % lengths.size();
        direction = variation / lengths.size();
      } else if constexpr (std::is_same_v<T, NoneRoutineFunction>) {
        // no variations
      }
    },
               baseRoutine.function);

    return getRoutineIndexByParams(baseIndex, direction, length);
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
  std::vector<Routine> Routines;
  std::vector<BaseRoutine> BaseRoutines;

  static constexpr std::array<ComponentLength, 3> lengths = {ComponentLength::Short, ComponentLength::Normal, ComponentLength::Long};
  static constexpr std::array<ComponentDirection, 2> dirs = {ComponentDirection::Left, ComponentDirection::Right};

  static constexpr std::array<const char*, 3> len_suffix = {" C", " M", " L"};
  static constexpr std::array<const char*, 2> dir_suffix = {" E", " D"};
};