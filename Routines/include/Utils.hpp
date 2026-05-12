#pragma once

#include <iostream>

void moveMotor(int leftSpeed, int rightSpeed) {
  // Implementação para controlar os motores
  std::cout << "Movendo motores - Esquerdo: " << leftSpeed << ", Direito: " << rightSpeed << std::endl;
}

void temporizadorRotina(int duration) {
  // Implementação para temporizar a rotina
  std::cout << "Temporizando rotina por " << duration << " ms" << std::endl;
}

void Quebra() {
  // Implementação para realizar uma pausa
  std::cout << "Quebra" << std::endl;
}

struct ComponentTiming {
  int value;
};

ComponentTiming shortFront = {500};          // Exemplo de valor para frente curta
ComponentTiming normalFront = {1000};        // Exemplo de valor para frente normal
ComponentTiming longFront = {1500};          // Exemplo de valor para frente longa
ComponentTiming shortReverse = {500};        // Exemplo de valor para ré curta
ComponentTiming normalReverse = {1000};      // Exemplo de valor para ré normal
ComponentTiming longReverse = {1500};        // Exemplo de valor para ré longa
ComponentTiming turn45L = {45};             // Exemplo de valor para giro 45° à esquerda
ComponentTiming turn45R = {45};             // Exemplo de valor para giro 45° à direita
ComponentTiming turn60L = {60};             // Exemplo de valor para giro 60° à esquerda
ComponentTiming turn60R = {60};             // Exemplo de valor para giro 60° à direita
ComponentTiming turn90L = {90};             // Exemplo de valor para giro 90° à esquerda
ComponentTiming turn90R = {90};             // Exemplo de valor para giro 90° à direita
ComponentTiming turn135L = {135};           // Exemplo de valor para giro 135° à esquerda
ComponentTiming turn135R = {135};           // Exemplo de valor para giro 135° à direita
ComponentTiming turn180L = {180};           // Exemplo de valor para giro 180° à esquerda
ComponentTiming turn180R = {180};           // Exemplo de valor para giro 180° à direita
ComponentTiming lineEscape90L = {90};       // Exemplo de valor para escape em linha 90° à esquerda
ComponentTiming lineEscape90R = {90};       // Exemplo de valor para escape em linha 90° à direita
ComponentTiming lineEscapeReverse = {1000};  // Exemplo de valor para escape em linha ré