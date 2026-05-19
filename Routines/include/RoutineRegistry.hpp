#pragma once

template <typename Func>
class RoutineRegistry {
 public:
  RoutineRegistry(GroupId groupId, std::string_view name, Func func) {
    RoutineManager::getInstance().registerRoutine(groupId, name, func);
  }
};