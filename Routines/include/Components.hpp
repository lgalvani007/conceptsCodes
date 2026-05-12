#pragma once

#include "Component.hpp"
#include "Utils.hpp"

void Frente(ComponentLength length) {
  moveMotor(100, 100);
  switch (length) {
    case ComponentLength::Short:
      temporizadorRotina(shortFront.value);
      break;

    case ComponentLength::Normal:
      temporizadorRotina(normalFront.value);
      break;

    case ComponentLength::Long:
      temporizadorRotina(longFront.value);
      break;
  }
}

void Re(ComponentLength length) {
  moveMotor(-100, -100);
  switch (length) {
    case ComponentLength::Short:
      temporizadorRotina(shortReverse.value);
      break;

    case ComponentLength::Normal:
      temporizadorRotina(normalReverse.value);
      break;

    case ComponentLength::Long:
      temporizadorRotina(longReverse.value);
      break;
  }
}

void Giro45(ComponentDirection direction) {
  if (direction == ComponentDirection::Left) {
    moveMotor(-100, 100);
    temporizadorRotina(turn45L.value);
  } else {
    moveMotor(100, -100);
    temporizadorRotina(turn45R.value);
  }
}

void Giro90(ComponentDirection direction) {
  if (direction == ComponentDirection::Left) {
    moveMotor(-100, 100);
    temporizadorRotina(turn90L.value);
  } else {
    moveMotor(100, -100);
    temporizadorRotina(turn90R.value);
  }
}

void Giro135(ComponentDirection direction) {
  if (direction == ComponentDirection::Left) {
    moveMotor(-100, 100);
    temporizadorRotina(turn135L.value);
  } else {
    moveMotor(100, -100);
    temporizadorRotina(turn135R.value);
  }
}

void Giro180(ComponentDirection direction) {
  if (direction == ComponentDirection::Left) {
    moveMotor(-100, 100);
    temporizadorRotina(turn180L.value);
  } else {
    moveMotor(100, -100);
    temporizadorRotina(turn180R.value);
  }
}

void Giro60(ComponentDirection direction) {
  if (direction == ComponentDirection::Left) {
    moveMotor(-100, 100);
    temporizadorRotina(turn60L.value);
  } else {
    moveMotor(100, -100);
    temporizadorRotina(turn60R.value);
  }
}

void ReLinha() {
  moveMotor(-100, -100);
  temporizadorRotina(lineEscapeReverse.value);
}

void Giro90Linha(ComponentDirection direction) {
  if (direction == ComponentDirection::Left) {
    moveMotor(-100, 100);
    temporizadorRotina(lineEscape90L.value);
  } else {
    moveMotor(100, -100);
    temporizadorRotina(lineEscape90R.value);
  }
}