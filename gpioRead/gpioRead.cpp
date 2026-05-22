#include <stdio.h>

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

const uint S1 = 26;
const uint S2 = 22;
const uint S3 = 21;
const uint S4 = 11;
const uint S5 = 19;
const uint S6 = 18;
const uint S7 = 17;
const uint SM = 16;

// 7 primeiros pinos com Pull-Up, o último (SM) sem Pull-Up
const uint SENSOR_PINS[8] = {S1, S2, S3, S4, S5, S6, S7, SM};

// Valor de contagem para o bloco de DMA resetar por hardware (Loop Infinito maximizado)
#define DMA_RELOAD_VALUE 0xFFFFFFFF

// Variável global de 32 bits que receberá a foto bruta dos pinos via DMA
volatile uint32_t sensor_gpios = 0;

int dma_chan;
uint pio_sm;

// Função inline para extração otimizada dos bits dos sensores
inline uint8_t extract_sensor_mask(uint32_t raw_snapshot) {
  return (((raw_snapshot >> SM) & 1) << 7) |
         (((raw_snapshot >> S7) & 1) << 6) |
         (((raw_snapshot >> S6) & 1) << 5) |
         (((raw_snapshot >> S5) & 1) << 4) |
         (((raw_snapshot >> S4) & 1) << 3) |
         (((raw_snapshot >> S3) & 1) << 2) |
         (((raw_snapshot >> S2) & 1) << 1) |
         (((raw_snapshot >> S1) & 1) << 0);
}

void setup_static_sensors(PIO pio) {
  uint16_t instructions[4];
  uint idx = 0;

  // 1. Inicializa os 8 pinos
  for (int i = 0; i < 8; i++) {
    uint pin = SENSOR_PINS[i];

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);

    // Ajuste preventivo para pinos do ADC (26 a 29) operarem no modo digital
    if (pin >= 26) {
      gpio_set_function(pin, GPIO_FUNC_SIO);
      gpio_set_input_enabled(pin, true);
    }

    // Regra de indexação: os 7 primeiros com Pull-Up, o último (SM) sem
    if (i == 7) {
      gpio_disable_pulls(pin);
    } else {
      gpio_pull_up(pin);
    }
  }

  // 2. PROGRAMA PIO: Tira a foto instantânea de 32 bits dos pinos (Apenas 1 instrução com Autopush)
  instructions[idx++] = pio_encode_in(pio_pins, 32);

  pio_program_t prg = {
      .instructions = reinterpret_cast<const uint16_t*>(instructions),
      .length = static_cast<uint8_t>(idx),
      .origin = -1};

  uint offset = pio_add_program(pio, &prg);
  pio_sm = pio_claim_unused_sm(pio, true);

  pio_sm_config config = pio_get_default_sm_config();
  sm_config_set_wrap(&config, offset, offset + idx - 1);
  sm_config_set_in_pins(&config, 0);

  // Autopush = true, threshold = 32
  sm_config_set_in_shift(&config, true, true, 32);

  // ========================================================================
  // TABELA DE CONFIGURAÇÃO DE FREQUÊNCIA DO PIO (Otimizada c/ Auto-Push: 1 ciclo)
  // Clock base do RP2040 = 125 MHz
  // ========================================================================
  // Comente/Descomente a taxa que deseja aplicar:

  // -> VELOCIDADE MÁXIMA ABSOLUTA (125 MHz / Atualiza a RAM a cada 8 ns)
  // sm_config_set_clkdiv(&config, 1.0f);

  // -> TAXA ULTRA RÁPIDA (50 MHz / Atualiza a RAM a cada 20 ns)
  // sm_config_set_clkdiv(&config, 2.5f);

  // -> TAXA DE ALTA PERFORMANCE (40 MHz / Atualiza a RAM a cada 25 ns)
  // sm_config_set_clkdiv(&config, 3.125f);

  // -> TAXA CRAVADA EM 20 MHz (Atualiza a RAM a cada 50 ns)
  sm_config_set_clkdiv(&config, 6.25f);

  // -> TAXA CRAVADA EM 10 MHz (Atualiza a RAM a cada 100 ns ou 0.1 µs)
  // sm_config_set_clkdiv(&config, 12.5f);

  // -> TAXA PADRÃO ESTÁVEL (2 MHz / Atualiza a RAM a cada 500 ns ou 0.5 µs)
  // sm_config_set_clkdiv(&config, 62.5f);

  // -> TAXA PADRÃO LENTA (1 MHz / Atualiza a RAM a cada 1.0 µs)
  // sm_config_set_clkdiv(&config, 125.0f);

  // ========================================================================

  pio_sm_init(pio, pio_sm, offset, &config);

  // ========================================================================
  // 3. CONFIGURAÇÃO DO DMA EM LOOP INFINITO (Mecanismo Hardware Chain)
  // ========================================================================
  dma_chan = dma_claim_unused_channel(true);
  int dma_reloader = dma_claim_unused_channel(true);

  // CANAL PRINCIPAL: Lê o FIFO do PIO e descarrega na RAM
  dma_channel_config c_main = dma_channel_get_default_config(dma_chan);
  channel_config_set_transfer_data_size(&c_main, DMA_SIZE_32);
  channel_config_set_read_increment(&c_main, false);
  channel_config_set_write_increment(&c_main, false);
  channel_config_set_dreq(&c_main, pio_get_dreq(pio, pio_sm, false));

  // CONEXÃO DE HARDWARE: Quando o canal principal esgota suas transferências, ativa o canal de recarga
  channel_config_set_chain_to(&c_main, dma_reloader);

  dma_channel_configure(
      dma_chan, &c_main,
      reinterpret_cast<volatile void*>(const_cast<uint32_t*>(&sensor_gpios)),
      &pio->rxf[pio_sm],
      DMA_RELOAD_VALUE,  // Conta de 0xFFFFFFFF até 0
      false              // Não dispara imediatamente, aguarda o reloader
  );

  // CANAL RELOADER: Reescreve o registrador de contagem do canal principal
  dma_channel_config c_reload = dma_channel_get_default_config(dma_reloader);
  channel_config_set_transfer_data_size(&c_reload, DMA_SIZE_32);
  channel_config_set_read_increment(&c_reload, false);
  channel_config_set_write_increment(&c_reload, false);

  static const uint32_t reload_value = DMA_RELOAD_VALUE;

  dma_channel_configure(
      dma_reloader, &c_reload,
      &dma_channel_hw_addr(dma_chan)->al1_transfer_count_trig,
      &reload_value,
      1,
      true  // Starta o reloader para armar o sistema
  );

  // Dá o pontapé inicial elétrico no canal principal
  dma_channel_start(dma_chan);

  // 4. ATIVA A MÁQUINA DE ESTADOS DO PIO
  pio_sm_set_enabled(pio, pio_sm, true);
}

int main() {
  stdio_init_all();

  // Aguarda o terminal USB estabelecer comunicação
  sleep_ms(5000);
  printf("Starting sensor reading with Infinite DMA Engine...\n");

  setup_static_sensors(pio0);

  while (true) {
    // Captura instantânea e atômica da variável atualizada pelo DMA na RAM
    uint32_t raw_snapshot = sensor_gpios;

    // Extração rápida dos bits
    uint8_t final_mask = extract_sensor_mask(raw_snapshot);

    // (Espaço livre para as malhas de controle, PIDs e navegação)

    // Impressão visual
    for (int i = 7; i >= 0; i--) {
      printf("%d", (final_mask >> i) & 1);
    }
    printf("\n");

    sleep_ms(10);
  }
}