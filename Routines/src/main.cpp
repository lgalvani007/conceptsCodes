#include <conio.h>  // Biblioteca do Windows para capturar teclas sem dar Enter

#include <iostream>
#include <string>
#include <vector>

#include "RoutineManager.hpp"
#include "Routines.hpp"

// ==========================================
// FUNÇÃO UTILITÁRIA PARA DESENHAR MENUS (CARROSSEL)
// ==========================================
// Retorna o índice (0 a N) da opção selecionada pelo usuário
int drawMenu(const std::string& title, const std::vector<std::string>& options) {
  if (options.empty()) return -1;

  int selected = 0;
  while (true) {
    system("cls");  // Limpa a tela do terminal do Windows

    std::cout << "========================================\n";
    std::cout << "   " << title << "\n";
    std::cout << "========================================\n\n";

    // Exibe APENAS a opção atual (Estilo Carrossel)
    std::cout << "           < " << options[selected] << " >\n\n";

    // Exibe o contador de navegação para o usuário não se perder
    std::cout << "             [ " << (selected + 1) << " / " << options.size() << " ]\n\n";

    std::cout << "----------------------------------------\n";
    std::cout << "(<-/-> ou UP/DOWN para mudar, ENTER para escolher)";

    // Captura a tecla
    int key = _getch();

    if (key == 224) {  // 224 é o código que avisa que uma Seta foi apertada
      key = _getch();  // O segundo getch pega qual seta foi

      // Seta para CIMA (72) ou Seta para ESQUERDA (75) = Voltar
      if (key == 72 || key == 75) {
        selected = (selected > 0) ? selected - 1 : options.size() - 1;
      }
      // Seta para BAIXO (80) ou Seta para DIREITA (77) = Avançar
      else if (key == 80 || key == 77) {
        selected = (selected < options.size() - 1) ? selected + 1 : 0;
      }
    } else if (key == 13) {  // 13 é o código da tecla ENTER
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
    std::vector<std::string> modeOptions = {
        "Modo Lista Direta (Todas as variacoes de uma vez)",
        "Modo Hierarquico (Filtro por Estrategia Base)",
        "Sair do Simulador"};
    int mode = drawMenu("MEGA SUMO - MODO DE NAVEGACAO", modeOptions);

    if (mode == 2) break;  // Sair

    std::vector<std::string> groupOptions;
    for (size_t i = 0; i < static_cast<size_t>(GroupId::NUM_ROUTINE_GROUPS); ++i) {
      groupOptions.push_back(std::string(manager.getGroupName(static_cast<GroupId>(i))));
    }
    groupOptions.push_back("Voltar");

    int groupSelection = drawMenu("SELECIONE O GRUPO", groupOptions);
    if (groupSelection == groupOptions.size() - 1) continue;  // Voltou

    GroupId selectedGroup = static_cast<GroupId>(groupSelection);
    size_t exactGlobalIndex = static_cast<size_t>(-1);  // Vai guardar a rotina final a ser executada

    if (mode == 0) {
      // MODO 1: LISTA DIRETA (Flat)
      std::vector<std::string> flatOptions;
      std::vector<size_t> flatGlobalIndices;  // Guarda o índice global de cada opção para podermos dar o run() depois

      size_t baseIdx = 0;
      // Varre todas as bases desse grupo usando as funções relativas
      while (true) {
        std::string_view baseName = manager.getBaseRoutineName(baseIdx, selectedGroup);
        if (baseName.empty()) break;  // Acabaram as bases desse grupo

        size_t varCount = manager.getRoutineVariantionsCount(baseIdx, selectedGroup);
        for (size_t v = 0; v < varCount; ++v) {
          flatOptions.push_back(std::string(manager.getRoutineName(baseIdx, v, selectedGroup)));
          // Salva o ID global dessa variação
          flatGlobalIndices.push_back(manager.getRoutineIndexByVariation(baseIdx, v, selectedGroup));
        }
        baseIdx++;
      }

      flatOptions.push_back("Voltar");
      int flatSelection = drawMenu("TODAS AS VARIACOES DO GRUPO", flatOptions);
      if (flatSelection == flatOptions.size() - 1) continue;

      exactGlobalIndex = flatGlobalIndices[flatSelection];

    } else {
      // MODO 2: HIERÁRQUICO (Base -> Variação)
      std::vector<std::string> baseOptions;
      size_t baseIdx = 0;

      // Pega o nome das Bases
      while (true) {
        std::string_view baseName = manager.getBaseRoutineName(baseIdx, selectedGroup);
        if (baseName.empty()) break;
        baseOptions.push_back(std::string(baseName));
        baseIdx++;
      }

      baseOptions.push_back("Voltar");
      int selectedBase = drawMenu("ESCOLHA A ESTRATEGIA BASE", baseOptions);
      if (selectedBase == baseOptions.size() - 1) continue;

      // Agora pega as Variações dessa Base específica
      std::vector<std::string> varOptions;
      size_t varCount = manager.getRoutineVariantionsCount(selectedBase, selectedGroup);
      for (size_t v = 0; v < varCount; ++v) {
        varOptions.push_back(std::string(manager.getRoutineName(selectedBase, v, selectedGroup)));
      }

      varOptions.push_back("Voltar");
      int selectedVar = drawMenu("ESCOLHA A VARIACAO (Parametros)", varOptions);
      if (selectedVar == varOptions.size() - 1) continue;

      exactGlobalIndex = manager.getRoutineIndexByVariation(selectedBase, selectedVar, selectedGroup);
    }

    system("cls");
    std::cout << "\n========================================\n";
    std::cout << "        PREPARANDO PARA LUTAR!          \n";
    std::cout << "========================================\n\n";
    std::cout << "Rotina Selecionada: " << manager.getRoutineName(exactGlobalIndex) << "\n";
    std::cout << "Indice Global na Memoria: [" << exactGlobalIndex << "]\n\n";

    manager.run(exactGlobalIndex);

    std::cout << "\n========================================\n";
    std::cout << "Pressione qualquer tecla para voltar ao menu...";
    _getch();  // Pausa a tela para você ler o console antes de limpar
  }
  system("cls");
  return 0;
}