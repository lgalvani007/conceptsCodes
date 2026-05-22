# Sistema Analógico Turbo (RP2040 Dual-Core ADC-DMA-DSP Engine)

Este repositório contém o firmware de alta performance desenvolvido para a leitura de múltiplos sensores analógicos (como sensores ópticos de refletividade para robôs micromouse ou combate sumo). 

A arquitetura foi projetada sob o conceito de **"Zero CPU Overhead"** e **"Sincronismo Estrito por Hardware"**, garantindo latência ultrabaixa e imunidade a ruídos elétricos sem comprometer o processador principal (Core 0).

---

## 📊 Diagrama Arquitetural de Fluxo de Dados

```text
                                  CAMADA DE HARDWARE PUREZA ANALÓGICA
                                 ┌───────────────────────────────────┐
                                 │       Regulador RT6150 (PWM)      │
                                 └─────────────────┬─────────────────┘
                                                   │ (VREF Limpo)
                                                   ▼
[Sensor Físico 0 (GPIO 26)] ───► ┌───────────────────────────────────┐
                                 │         ADC do RP2040             │
[Sensor Físico 1 (GPIO 27)] ───► │  - Chaveamento por Round-Robin    │
                                 │  - Oversampling Nativo de 16x     │
                                 └─────────────────┬─────────────────┘
                                                   │ (DREQ)
                                                   ▼
                                 ┌───────────────────────────────────┐
                                 │          FIFO do ADC              │
                                 └─────────────────┬─────────────────┘
                                                   │
                                                   ▼
                                 ┌───────────────────────────────────┐
                                 │     DMA Principal (Ring Buffer)   │
                                 │  - Escreve em Loop Circular (2bit)│
                                 └─────────────────┬─────────────────┘
                                                   │
                                                   │ (Dispara IRQ0 Física ao fim do bloco)
                                                   ▼
                                      CORE 1: COPROCESSADOR DSP                          CORE 0: ESTRATÉGIA E PID
                                ┌───────────────────────────────────┐              ┌───────────────────────────────────┐
                                │        core1_dma_handler()        │              │            Loop Principal         │
                                │                                   │              │                                   │
                                │ 1. Rearma o DMA no Topo           │              │  - Controle de Motores            │
                                │ 2. Aplica Filtro IIR (50/50)      │              │  - Cálculo do PID                 │
                                │ 3. Avalia Histerese Dinâmica      │              │  - Leituras Livres de Overhead    │
                                └─────────────────┬─────────────────┘              └─────────────────▲─────────────────┘
                                                  │                                                  │
                                                  │ (Gatilho Válido)                                 │ (Interrupção Assíncrona)
                                                  └───────────► [ SIO FIFO INTER-CORE ] ─────────────┘
                                                                  (Mensagem de 32-bits)
```                           
---

## 🛠️ Funcionamento Detalhado das 5 Camadas

### 1. Blindagem Elétrica Nativa (Hardware Power Management)
Ao comutar o pino **GPIO 23** para nível alto (`1`), o firmware desativa o modo econômico PFM do regulador de tensão chaveado integrado da Raspberry Pi Pico, forçando-o a operar em **modo PWM de frequência fixa**. Isso elimina o ruído caótico de *ripple* na linha de referência analógica (VREF), estabilizando as leituras elétricas basais do ADC.

### 2. Oversampling e Cadência Controlada (Motor do ADC)
O ADC opera em modo contínuo (*Free-Running*) alternando os pinos ativos via *Round-Robin* por hardware. Através do registrador FCS, ativamos o **Oversampling Nativo de 16x**: para cada leitura entregue, o hardware colhe 16 amostras na velocidade máxima do silício e tira a média internamente. 

Configurando `adc_set_clkdiv(192)`, o clock do ADC é desacelerado para 250 kHz efetivos. 
* **Tempo de conversão por sensor:** 6,14 µs
* **Tempo do bloco (2 sensores):** 12,28 µs
* **Taxa de amostragem síncrona final:** **81,3 kHz**

### 3. Transporte Autônomo via Ring Buffer (Músculo do DMA)
Para evitar perda de sincronismo e *crosstalk* induzido por software, um único canal de **DMA (Direct Memory Access)** é sincronizado pelo sinal **DREQ (Data Request)** do ADC. Assim que o ADC termina a média de 16x, o DMA move o dado diretamente do FIFO para a memória RAM (`raw_readings`).

Ativando o **Ring Buffer de escrita em 2 bits (4 bytes)**, criamos um anel circular no silício: o DMA preenche o slot do Sensor 0, avança para o Sensor 1, dispara a interrupção física e reseta o seu ponteiro de memória de volta para o Sensor 0 de forma totalmente automática.

### 4. Coprocessamento Síncrono Isolado (O Papel Dedicado do Core 1)
O Core 1 é configurado para rodar exclusivamente em repouso elétrico profundo via instrução `__wfi()` (Wait For Interrupt), sem gastar ciclos de clock em laços vazios. 

No microssegundo exato em que o DMA preenche o bloco de sensores, a interrupção `DMA_IRQ_0` acorda o Core 1. A ISR executa três passos críticos:
1. **Rearme Imediato:** Atualiza o contador do DMA no topo da interrupção para que o próximo ciclo em background nunca transborde o FIFO.
2. **Filtro Digital IIR:** Aplica uma Média Móvel Exponencial adaptada com peso de 50/50 para amortecer ruídos espúrios.
3. **Máquina de Estados com Histerese Dinâmica:** Avalia se o sinal limpo entrou ou saiu das janelas de corte definidas, usando uma zona morta de proteção de 80 dígitos para evitar o efeito "metralhadora" de gatilhos nos limiares.

### 5. Comunicação de Eventos Sem Bloqueio (Inter-Core FIFO)
O Core 0 (processador principal) fica completamente livre para executar a malha de controle PID dos motores e a lógica de navegação/trajetória. Ele não gasta tempo lendo o ADC.

Quando o Core 1 detecta uma invasão real de *threshold*, ele altera a trava `inside_window` (evitando re-disparos) e despacha uma mensagem de 32 bits para a **SIO FIFO inter-core**. Isso gera um pulso assíncrono que interrompe o Core 0 (`core0_sio_fifo_irq_handler`), permitindo ações evasivas ou correções de trajetória instantâneas com latência total inferior a 13 µs.

---

## 📋 Requisitos e Calibração

* **Hardware:** Raspberry Pi Pico ou qualquer placa baseada no microcontrolador RP2040.
* **SDK:** Raspberry Pi Pico SDK v2.1.1 (ou superior), configurado com suporte a C++11/C++14.
* **Compilação:** Otimizado para CMake + Ninja (`-O3` recomendado para garantir a execução da ISR do Core 1 em tempo hábil).
* **Calibração de Pista:** Os valores de `threshold_min` e `threshold_max` dentro da estrutura `AdcSensorConfig` devem ser ajustados de acordo com a leitura analógica obtida sob as condições reais de luz e contraste dos seus sensores ópticos na pista.