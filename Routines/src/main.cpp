#include <iostream>

#include "RoutineManager.hpp"
#include "Routines.hpp"

int main() {
  RoutineManager manager;

  manager.registerRoutine("Frente", Frente);
  manager.registerRoutine("Re", Re);
  manager.registerRoutine("Sete", Sete);
  manager.registerRoutine("setePerfeito", setePerfeito);
  manager.registerRoutine("SeteMQP", SeteMQP);
  manager.registerRoutine("Sete135", Sete135);
  manager.registerRoutine("SetePerfeito135", SetePerfeito135);
  manager.registerRoutine("SeteMQP135", SeteMQP135);
  manager.registerRoutine("Quatorze", Quatorze);
  manager.registerRoutine("VinteUm", VinteUm);
  manager.registerRoutine("AvancaGalena", AvancaGalena);
  manager.registerRoutine("Redirections", Redirections);
  manager.registerRoutine("ReAvancado", ReAvancado);
  manager.registerRoutine("ReAvancado60", ReAvancado60);
  manager.registerRoutine("Desempate", Desempate);

  manager.dumpRoutines();

  std::cout << "\n\n"
            << std::endl;

  size_t routineIndex;
  while (true) {
    std::cout << "\nDigite o indice da rotina para rodar (ou 999 para sair): ";

    if (!(std::cin >> routineIndex)) {
      std::cout << "Entrada invalida! Digite um numero.\n";
      std::cin.clear();
      std::cin.ignore(1000, '\n');
      continue;
    }

    if (routineIndex == 999) break;

    manager.run(routineIndex);
  }
  return 0;
}