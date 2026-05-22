# Sistema Digital de Alta Performance (RP2040 Zero-CPU PIO-DMA Chain Engine)

Este repositório contém o firmware de altíssima performance desenvolvido para a leitura paralela e instantânea de até 8 sensores digitais (como sensores de linha ou sensores de proximidade ópticos digitais para robôs micromouse e combate sumo).

A arquitetura foi projetada sob o conceito de **"Zero-CPU Interference"**, utilizando o coprocessador programável PIO junto com um encadeamento duplo de DMA (*DMA Hardware Chain*). Isso permite que o estado dos pinos seja atualizado na memória RAM continuamente sem gastar um único ciclo de processamento dos núcleos Core 0 ou Core 1.

---

## 📊 Diagrama Arquitetural de Fluxo de Dados

```text
                               CAMADA DE HARDWARE TOTALMENTE AUTÔNOMA
                               ┌─────────────────────────────────────┐
                               │        Sensores Digitais (S1~SM)    │
                               └──────────────────┬──────────────────┘
                                                  │ (Leitura Física)
                                                  ▼
                               ┌─────────────────────────────────────┐
                               │           Barramento PIO            │
                               │ - Coprocessador Paralelo (Ex: 20MHz)│
                               │ - Foto Atômica e Auto-Push 32-bits  │
                               └──────────────────┬──────────────────┘
                                                  │ (DREQ em 1 ciclo)
                                                  ▼
                               ┌─────────────────────────────────────┐
                               │            PIO RX FIFO              │
                               └──────────────────┬──────────────────┘
                                                  │
                                                  ▼
 ┌──────────────────────────────────────────────────────────────────────────────────┐
 │                           ECOSSISTEMA DMA HARDWARE CHAIN                         │
 │                                                                                  │
 │ ┌───────────────────────────────────┐     (Chain)    ┌─────────────────────────┐ │
 │ │         DMA Channel Main          │───────────────►│   DMA Channel Reloader  │ │
 │ │ - Puxa 32-bits do FIFO do PIO     │                │ - Monitora o Canal Main │ │
 │ │ - Sobrescreve a RAM 'sensor_gpios'│◄───────────────│ - Injeta 0xFFFFFFFF     │ │
 │ └─────────────────┬─────────────────┘  (Auto-Trigger)└─────────────────────────┘ │
 └───────────────────┼──────────────────────────────────────────────────────────────┘
                     │
                     │ (Movimentação Puramente por Silício / Zero Latência)
                     ▼
       CORE 0: PROCESSADOR PRINCIPAL                      CORE 1: COPROCESSADOR EXTRA
 ┌───────────────────────────────────┐              ┌───────────────────────────────────┐
 │       Loop Principal (main)       │              │                                   │
 │                                   │              │                                   │
 │ - Controle de Motores (PID)       │              │      - Totalmente Livre para      │
 │ - Algoritmos de Navegação         │              │        Outras Tarefas Críticas    │
 │ - Captura Atômica da RAM          │              │        (Ex: Leituras do ADC)      │
 └───────────────────────────────────┘              └───────────────────────────────────┘
```

---

## 🛠️ Funcionamento Detalhado das 4 Camadas

### 1. Inicialização Elétrica e Ajuste de Modo (GPIO Configuration)
O código inicializa os 8 pinos dos sensores através de um laço estruturado. Ele aplica Pull-Up interno para os 7 primeiros sensores (garantindo nível lógico estável contra ruídos eletromagnéticos) e deixa o último sensor (SM) sem resistores de pull, adequando-se ao circuito elétrico da placa.

* **Ajuste Crítico de Silício:** Os pinos que compartilham barramento com o ADC (GPIOs 26 a 29) são forçados via software a operar no modo digital padrão (`GPIO_FUNC_SIO`) com o buffer de entrada explicitamente habilitado (`gpio_set_input_enabled`), mitigando qualquer conflito de impedância de hardware.

### 2. Coprocessamento Paralelo e Foto Atômica (Motor do PIO com Auto-Push)
Em vez de usar a CPU para ler os pinos sequencialmente, o firmware monta e injeta um programa em tempo de execução diretamente na memória de instruções do PIO (*Programmable I/O*). Esta arquitetura foi otimizada para usar **apenas 1 instrução**.

* `pio_encode_in(pio_pins, 32)`: Captura o estado de todos os 32 pinos do chip de forma simultânea e atômica.
* **Hardware Auto-Push:** O recurso `sm_config_set_in_shift` está configurado para ejetar os dados para o FIFO automaticamente via hardware assim que o threshold de 32 bits for atingido, tudo no mesmo ciclo de clock da leitura.
* Utilizando um divisor padrão de `6.25f` sobre o clock de 125 MHz da Pico, a máquina de estados do PIO roda a **20 MHz**. Isso significa que uma foto digital completa de todo o barramento elétrico do robô é injetada no FIFO a cada **50 nanossegundos** (1 ciclo = 1 leitura + 1 push).

### 3. Loop Infinito por Silício (Ecossistema DMA Chain)
Para evitar que a CPU gaste tempo esvaziando o FIFO do PIO, a arquitetura utiliza uma técnica avançada de Encadeamento de DMA (*DMA Chain*) bloqueando dois canais de hardware:

* **Canal Principal (Main):** Fica em guarda escutando o PIO. Assim que o FIFO recebe o dado, o DMA Main pega o registro de 32 bits e joga direto na variável global `sensor_gpios` alocada na memória RAM. Ele faz isso decrementando o contador de transferências a partir de `0xFFFFFFFF` (valor máximo de 32 bits) para minimizar interrupções no barramento.
* **Canal de Recarga (Reloader):** Aponta diretamente para o registrador interno de gatilho do Canal Principal (`al1_transfer_count_trig`). No ciclo exato em que o Canal Principal esgota seus bilhões de ciclos e chega a zero, o hardware ativa o Canal Reloader em background. O Reloader faz uma escrita atômica do valor `0xFFFFFFFF` de volta no Canal Principal, reiniciando o loop instantaneamente.

Toda essa engrenagem roda de forma puramente física nos barramentos internos do chip, com **zero interrupções (IRQs)** de software e **zero consumo de CPU**.

### 4. Execução Livre de Overhead (Aplicações na Main)
Como o hardware atualiza a variável `sensor_gpios` em background na velocidade do silício (20.000.000 de vezes por segundo), o laço principal do seu robô (`while(true)`) executa de forma totalmente livre.

Para ler os sensores, a `main` faz apenas uma cópia local e atômica da variável global. Utilizando uma função otimizada (`inline`) com operadores de deslocamento de bits (Bitshift `>>`), o código isola os bits dos pinos desejados e compacta os dados na ordem visual exata do robô: `[SM][S7][S6][S5][S4][S3][S2][S1]`. A CPU fica com 100% de banda livre para rodar os PIDs de velocidade dos motores e os algoritmos de labirinto ou combate.