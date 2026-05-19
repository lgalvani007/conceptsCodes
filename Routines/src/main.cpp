#include <iostream>

#include "RoutineManager.hpp"
#include "Routines.hpp"

int main() {
  RoutineManager& manager = RoutineManager::getInstance();

  // Registering routines
  std::cout << "========================================\n";
  std::cout << "   MEGA SUMO - GERENCIADOR DE ROTINAS   \n";
  std::cout << "========================================\n";
  std::cout << "Total de Rotinas (com variacoes): " << manager.getRoutineCount() << "\n";
  std::cout << "Total de Estrategias Base: " << manager.getBaseRoutineCount() << "\n\n";

  while (true) {
    std::cout << "\n--- MENU PRINCIPAL ---\n";
    manager.dumpBaseRoutines();

    std::cout << "\nDigite o indice da Estrategia Base (ou 999 para sair): ";
    size_t baseIndex;
    if (!(std::cin >> baseIndex)) {
      std::cout << "[Erro] Digite um numero valido.\n";
      std::cin.clear();
      std::cin.ignore(1000, '\n');
      continue;
    }

    if (baseIndex == 999) break;

    size_t varCount = manager.getRoutineVariantionsCount(baseIndex);
    if (varCount == 0) {
      std::cout << "[Erro] Indice base invalido!\n";
      continue;
    }

    size_t selectedVariation = 0;

    if (varCount > 1) {
      std::cout << "\nEstrategia [" << manager.getBaseRoutineName(baseIndex)
                << "] possui " << varCount << " variacoes.\n";

      for (size_t v = 0; v < varCount; ++v) {
        size_t globalIdx = manager.getRoutineIndexByVariation(baseIndex, v);
        std::cout << "  [" << v << "] " << manager.getRoutineName(globalIdx) << "\n";
      }

      std::cout << "Escolha a variacao (0 a " << (varCount - 1) << "): ";
      if (!(std::cin >> selectedVariation) || selectedVariation >= varCount) {
        std::cout << "[Erro] Variacao invalida. Abortando.\n";
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        continue;
      }
    }

    size_t exactIndex = manager.getRoutineIndexByVariation(baseIndex, selectedVariation);

    std::cout << "\n>>> PREPARANDO PARA LUTAR <<<\n";
    std::cout << "Executando: " << manager.getRoutineName(exactIndex) << " (ID Global: " << exactIndex << ")\n";
    manager.run(exactIndex);
    std::cout << "----------------------------------------\n";
  }

  std::cout << "Encerrando simulacao.\n";
  return 0;
}