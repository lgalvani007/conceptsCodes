#include <iostream>

#include "RoutineManager.hpp"
#include "Routines.hpp"

// Macro utilitária para evitar digitar o nome duas vezes e prevenir erros de digitação
#define REG_ROUTINE(mgr, func) mgr.registerRoutine(#func, func)

int main() {
  RoutineManager manager;

  // 1. Registro Limpo e Elegante
  REG_ROUTINE(manager, Frente);
  REG_ROUTINE(manager, Re);
  REG_ROUTINE(manager, Sete);
  REG_ROUTINE(manager, setePerfeito);
  REG_ROUTINE(manager, SeteMQP);
  REG_ROUTINE(manager, Sete135);
  REG_ROUTINE(manager, SetePerfeito135);
  REG_ROUTINE(manager, SeteMQP135);
  REG_ROUTINE(manager, Quatorze);
  REG_ROUTINE(manager, VinteUm);
  REG_ROUTINE(manager, AvancaGalena);
  REG_ROUTINE(manager, Redirections);
  REG_ROUTINE(manager, ReAvancado);
  REG_ROUTINE(manager, ReAvancado60);
  REG_ROUTINE(manager, Desempate);

  // 2. Info de Debug
  std::cout << "========================================\n";
  std::cout << "   MEGA SUMO - GERENCIADOR DE ROTINAS   \n";
  std::cout << "========================================\n";
  std::cout << "Total de Rotinas (com variacoes): " << manager.getRoutineCount() << "\n";
  std::cout << "Total de Estrategias Base: " << manager.getBaseRoutineCount() << "\n\n";

  // 3. Loop do Menu Simulado (Idêntico ao fluxo da Interface GUI/OLED)
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

    // Se tiver mais de 1 variação, pergunta qual o usuário quer
    if (varCount > 1) {
      std::cout << "\nEstrategia [" << manager.getBaseRoutineName(baseIndex)
                << "] possui " << varCount << " variacoes.\n";

      // Mostra as variações disponíveis
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

    // Resolve o índice final e executa
    size_t exactIndex = manager.getRoutineIndexByVariation(baseIndex, selectedVariation);

    std::cout << "\n>>> PREPARANDO PARA LUTAR <<<\n";
    std::cout << "Executando: " << manager.getRoutineName(exactIndex) << " (ID Global: " << exactIndex << ")\n";
    manager.run(exactIndex);
    std::cout << "----------------------------------------\n";
  }

  std::cout << "Encerrando simulacao.\n";
  return 0;
}