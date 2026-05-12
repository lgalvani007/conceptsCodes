cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(Routines)

# Especifica o padrão do C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Informa onde estão os arquivos .hpp
include_directories(include)

# Cria o executável juntando o main.cpp e os outros .cpp
add_executable(Routines src/main.cpp)