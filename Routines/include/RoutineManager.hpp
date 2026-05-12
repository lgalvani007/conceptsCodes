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
  }

  void dumpRoutines() const {
    size_t index = 0;
    for (const auto& routine : Routines) {
      std::cout << "[" << index++ << "] " << routine.name << " (Direction: " << static_cast<int>(routine.direction)
                << ", Length: " << static_cast<int>(routine.length) << ")\n";
    }
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
  std::vector<Routine> Routines;

  static constexpr std::array<ComponentLength, 3> lengths = {ComponentLength::Short, ComponentLength::Normal, ComponentLength::Long};
  static constexpr std::array<ComponentDirection, 2> dirs = {ComponentDirection::Left, ComponentDirection::Right};

  static constexpr std::array<const char*, 3> len_suffix = {" C", " M", " L"};
  static constexpr std::array<const char*, 2> dir_suffix = {" E", " D"};
};