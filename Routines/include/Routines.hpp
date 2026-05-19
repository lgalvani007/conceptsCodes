#pragma once

#include "Component.hpp"
#include "Components.hpp"
#include "RoutineRegistry.hpp"

void Sete(ComponentDirection direction) {
  Frente(ComponentLength::Long);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void setePerfeito(ComponentLength length, ComponentDirection direction) {
  Giro45(direction);
  Quebra();
  Frente(length);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void SeteMQP(ComponentDirection direction) {
  Giro90(direction);
  Quebra();
  Frente(ComponentLength::Long);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void Sete135(ComponentDirection direction) {
  Frente(ComponentLength::Long);
  Quebra();
  Giro135(oppositeDirection(direction));
  Quebra();
}

void SetePerfeito135(ComponentDirection direction, ComponentLength length) {
  Giro45(direction);
  Quebra();
  Frente(length);
  Quebra();
  Giro135(oppositeDirection(direction));
  Quebra();
}

void SeteMQP135(ComponentDirection direction) {
  Giro90(direction);
  Quebra();
  Frente(ComponentLength::Long);
  Quebra();
  Giro135(oppositeDirection(direction));
  Quebra();
}

void SetePerfeitoCurto(ComponentDirection direction) {
  Giro60(direction);
  Quebra();
  Frente(ComponentLength::Normal);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void SetePerfeitoCurto135(ComponentDirection direction) {
  Giro60(direction);
  Quebra();
  Frente(ComponentLength::Normal);
  Quebra();
  Giro135(oppositeDirection(direction));
  Quebra();
}

void Quatorze(ComponentDirection direction) {
  Giro45(direction);
  Quebra();
  Frente(ComponentLength::Long);
  Quebra();
  Giro45(oppositeDirection(direction));
  Quebra();
  Frente(ComponentLength::Normal);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void VinteUm(ComponentDirection direction) {
  Frente(ComponentLength::Normal);
  Quebra();
  Giro45(oppositeDirection(direction));
  Quebra();
  Frente(ComponentLength::Short);
  Quebra();
  Giro45(oppositeDirection(direction));
  Quebra();
  Frente(ComponentLength::Short);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void AvancaGalena(ComponentDirection direction) {
  // Giro45(oppositeDirection(direction));
  // Quebra();
  Frente(ComponentLength::Long);
  Quebra();
  Re(ComponentLength::Long);
  Quebra();
  Giro90(direction);
  Quebra();
  Frente(ComponentLength::Long);
  Quebra();
  Giro90(oppositeDirection(direction));
  Quebra();
}

void Redirections(ComponentDirection direction) {
  Re(ComponentLength::Short);
  Quebra();
  Giro45(oppositeDirection(direction));
  Quebra();
}

void ReAvancado(ComponentDirection direction) {
  Giro45(oppositeDirection(direction));
  Quebra();
  Re(ComponentLength::Normal);
  Quebra();
}

void ReAvancado60(ComponentDirection direction) {
  Giro60(oppositeDirection(direction));
  Quebra();
  Re(ComponentLength::Normal);
  Quebra();
}

void Desempate(ComponentDirection direction) {
  Giro180(direction);
  Quebra();
}

static RoutineRegistry FrenteRoutine(GroupId::INITIAL_ROUTINE, "Frente", Frente);
static RoutineRegistry ReRoutine(GroupId::INITIAL_ROUTINE, "Re", Re);
static RoutineRegistry SeteRoutine(GroupId::INITIAL_ROUTINE, "Sete", Sete);
static RoutineRegistry setePerfeitoRoutine(GroupId::INITIAL_ROUTINE, "SetePerfeito", setePerfeito);
static RoutineRegistry SeteMQPRoutine(GroupId::INITIAL_ROUTINE, "SeteMQP", SeteMQP);
static RoutineRegistry Sete135Routine(GroupId::INITIAL_ROUTINE, "Sete135", Sete135);
static RoutineRegistry SetePerfeito135Routine(GroupId::INITIAL_ROUTINE, "SetePerfeito135", SetePerfeito135);
static RoutineRegistry SeteMQP135Routine(GroupId::INITIAL_ROUTINE, "SeteMQP135", SeteMQP135);
static RoutineRegistry QuatorzeRoutine(GroupId::INITIAL_ROUTINE, "Quatorze", Quatorze);
static RoutineRegistry VinteUmRoutine(GroupId::INITIAL_ROUTINE, "VinteUm", VinteUm);
static RoutineRegistry AvancaGalenaRoutine(GroupId::INITIAL_ROUTINE, "AvancaGalena", AvancaGalena);
static RoutineRegistry RedirectionsRoutine(GroupId::INITIAL_ROUTINE, "Redirections", Redirections);
static RoutineRegistry ReAvancadoRoutine(GroupId::INITIAL_ROUTINE, "ReAvancado", ReAvancado);
static RoutineRegistry ReAvancado60Routine(GroupId::INITIAL_ROUTINE, "ReAvancado60", ReAvancado60);
static RoutineRegistry DesempateRoutine(GroupId::INITIAL_ROUTINE, "Desempate", Desempate);