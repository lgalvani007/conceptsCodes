#include <conio.h>  // Lembre-se de remover/substituir se for compilar para o RP2040

#include <iostream>
#include <string_view>

#include "RoutineGroups.hpp"
#include "RoutineManager.hpp"
#include "Routines.hpp"

// ==========================================
// FUNÇÃO DE RENDERIZAÇÃO ZERO-RAM
// ==========================================
template <typename FetchNameFunc>
int drawMenu(std::string_view title, size_t itemCount, FetchNameFunc getName) {
  if (itemCount == 0) return -1;

  int selected = 0;
  size_t totalOptions = itemCount + 1;  // +1 é sempre a opção "Voltar"

  while (true) {
    system("cls");  // No RP2040, substitua pelas funções do seu display OLED (ex: display.clear())

    std::cout << "========================================\n";
    std::cout << "   " << title << "\n";
    std::cout << "========================================\n\n";

    // Pega o nome direto do Manager na hora de desenhar
    std::string_view currentName = (selected < itemCount) ? getName(selected) : "Voltar";

    std::cout << "           < " << currentName << " >\n\n";
    std::cout << "             [ " << (selected + 1) << " / " << totalOptions << " ]\n\n";
    std::cout << "----------------------------------------\n";
    std::cout << "(<-/-> ou UP/DOWN para mudar, ENTER para escolher)\n";

    int key = _getch();
    if (key == 224) {
      key = _getch();
      if (key == 72 || key == 75) {  // Cima / Esquerda
        selected = (selected > 0) ? selected - 1 : totalOptions - 1;
      } else if (key == 80 || key == 77) {  // Baixo / Direita
        selected = (selected < totalOptions - 1) ? selected + 1 : 0;
      }
    } else if (key == 13) {  // ENTER
      return selected;
    }
  }
}

// ==========================================
// FUNÇÃO PRINCIPAL
// ==========================================
int main() {
  auto& manager = RoutineManager::getInstance();

  while (true) {
    // ---------------------------------------------------------
    // 1. MODO DE NAVEGAÇÃO
    // ---------------------------------------------------------
    int mode = drawMenu("MODO DE NAVEGACAO", 2, [&](size_t i) -> std::string_view {
      return (i == 0) ? "Lista Direta (Sem Base)" : "Hierarquico (Filtro por Base)";
    });
    if (mode == 2) break;  // Saiu do programa

    // ---------------------------------------------------------
    // 2. ESCOLHER O GRUPO
    // ---------------------------------------------------------
    size_t groupCount = manager.getGroupCount();
    int groupSelection = drawMenu("SELECIONE O GRUPO", groupCount, [&](size_t i) {
      return manager.getGroupName(static_cast<GroupId>(i));
    });
    if (groupSelection == groupCount) continue;  // Clicou em "Voltar"

    GroupId selectedGroup = static_cast<GroupId>(groupSelection);
    size_t exactGlobalIndex = static_cast<size_t>(-1);

    // ---------------------------------------------------------
    // 3 e 4. NAVEGAÇÃO INTERNA
    // ---------------------------------------------------------
    if (mode == 0) {
      // MODO LISTA DIRETA: Mostra todas as variações do grupo
      size_t flatCount = manager.getRoutineCount(selectedGroup);
      if (flatCount == 0) continue;  // Grupo vazio

      int flatSelection = drawMenu("TODAS AS VARIACOES", flatCount, [&](size_t i) {
        return manager.getRoutineName(i, selectedGroup);
      });
      if (flatSelection == flatCount) continue;  // Voltar

      exactGlobalIndex = manager.getRoutineIndex(flatSelection, selectedGroup);

    } else {
      // MODO HIERÁRQUICO: Escolhe Base -> Escolhe Variação
      size_t baseCount = manager.getBaseRoutineCount(selectedGroup);
      if (baseCount == 0) continue;  // Grupo vazio

      int selectedBase = drawMenu("ESTRATEGIA BASE", baseCount, [&](size_t i) {
        return manager.getBaseRoutineName(i, selectedGroup);
      });
      if (selectedBase == baseCount) continue;  // Voltar

      size_t varCount = manager.getRoutineVariantionsCount(selectedBase, selectedGroup);

      int selectedVar = drawMenu("VARIACAO (Parametros)", varCount, [&](size_t i) {
        return manager.getRoutineName(selectedBase, i, selectedGroup);
      });
      if (selectedVar == varCount) continue;  // Voltar

      exactGlobalIndex = manager.getRoutineIndexByVariation(selectedBase, selectedVar, selectedGroup);
    }

    // ---------------------------------------------------------
    // 5. EXECUTAR
    // ---------------------------------------------------------
    if (exactGlobalIndex != static_cast<size_t>(-1)) {
      system("cls");
      std::cout << "\n========================================\n";
      std::cout << "        PREPARANDO PARA LUTAR!          \n";
      std::cout << "========================================\n\n";
      std::cout << "Estrategia: " << manager.getRoutineName(exactGlobalIndex) << "\n";
      std::cout << "Indice Global na Memoria: [" << exactGlobalIndex << "]\n\n";

      manager.run(exactGlobalIndex);

      std::cout << "\n========================================\n";
      std::cout << "Pressione qualquer tecla para voltar ao menu...";
      _getch();
    }
  }

  return 0;
}