#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

// ============================================================================
// CONFIGURAÇÃO E ESTRUTURA DE DADOS (ARQUITETURA DE ALTA VELOCIDADE)
// ============================================================================

struct AdcSensorConfig {
  uint gpio_pin;           // Pino GPIO (26, 27, 28)
  uint16_t threshold_min;  // Limite inferior da janela
  uint16_t threshold_max;  // Limite superior da janela
  bool inside_window;      // Estado atual (travado/livre)
};

// Configuração dos sensores com os thresholds da sua bancada
AdcSensorConfig my_sensors[] = {
    {26, 1200, 3500, false},
    {27, 1200, 3500, false}};

const size_t NUM_SENSORS = sizeof(my_sensors) / sizeof(my_sensors[0]);

// Buffer circular de hardware (Ring Buffer) alinhado para 2 sensores de 16-bits
alignas(4) volatile uint16_t raw_readings[2];

// Valores filtrados finais para uso na estratégia do robô
volatile uint16_t filtered_readings[NUM_SENSORS];

int dma_channel;
dma_channel_config dma_cfg;

// Constantes de calibração fina para 81.3 kHz
const uint16_t HYSTERESIS_MARGIN = 80;
const uint16_t EV_ENTERED_WINDOW = 0x3333;

// ============================================================================
// [CORE 0] TRATAMENTO DE EVENTOS (ESTRATÉGIA)
// ============================================================================
void core0_sio_fifo_irq_handler() {
  while (multicore_fifo_rvalid()) {
    uint32_t msg = multicore_fifo_pop_blocking();
    uint16_t event_type = (msg >> 16) & 0xFFFF;
    uint16_t sensor_idx = msg & 0xFFFF;

    if (sensor_idx < NUM_SENSORS && event_type == EV_ENTERED_WINDOW) {
      // Este bloco só roda quando o Core 1 detecta uma invasão real de threshold
      printf("\n>>>> [EVENTO] Sensor %u detectou objeto na janela! <<<<\n\n", sensor_idx);
    }
  }
}

// ============================================================================
// [CORE 1] PROCESSAMENTO DE SINAL (DSP & ESTADOS)
// ============================================================================
void core1_dma_handler() {
  // 1. Limpa a interrupção de hardware imediatamente
  dma_channel_acknowledge_irq0(dma_channel);

  // 2. REARMAMENTO ULTRA-RÁPIDO: O DMA já volta a trabalhar enquanto calculamos
  dma_channel_set_trans_count(dma_channel, NUM_SENSORS, true);

  // 3. Loop de processamento para cada sensor
  for (size_t i = 0; i < NUM_SENSORS; i++) {
    uint16_t current_val = raw_readings[i];

    // Filtro IIR (Amortecimento Digital)
    // 50% de peso para o histórico e 50% para a leitura nova
    filtered_readings[i] = (uint16_t)((filtered_readings[i] * 5 + current_val * 5) / 10);
    uint16_t val = filtered_readings[i];

    // Máquina de Estados com Histerese (Evita metralhadora de gatilhos)
    if (!my_sensors[i].inside_window) {
      // Se estava FORA, verifica se entrou com margem de segurança superior
      if (val >= (my_sensors[i].threshold_min + HYSTERESIS_MARGIN) &&
          val <= (my_sensors[i].threshold_max - HYSTERESIS_MARGIN)) {
        my_sensors[i].inside_window = true;  // Trava o estado

        if (multicore_fifo_wready()) {
          multicore_fifo_push_blocking((static_cast<uint32_t>(EV_ENTERED_WINDOW) << 16) | (i & 0xFFFF));
        }
      }
    } else {
      // Se estava DENTRO, só sai se o valor for para bem longe das bordas
      if (val < (my_sensors[i].threshold_min - HYSTERESIS_MARGIN) ||
          val > (my_sensors[i].threshold_max + HYSTERESIS_MARGIN)) {
        my_sensors[i].inside_window = false;  // Rearma o sensor
      }
    }
  }
}

void core1_main() {
  dma_channel_set_irq0_enabled(dma_channel, true);
  irq_set_exclusive_handler(DMA_IRQ_0, core1_dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  while (true) {
    __wfi();  // Dorme até o DMA terminar o próximo bloco de sensores
  }
}

// ============================================================================
// [CORE 0] SETUP DO SISTEMA ANALÓGICO
// ============================================================================
void setup_flexible_analog_system() {
  // Força o regulador da Pico em modo PWM para limpar o ruído elétrico (Power Blindage)
  gpio_init(23);
  gpio_set_dir(23, GPIO_OUT);
  gpio_put(23, 1);

  adc_init();

  uint8_t rb_mask = 0;
  int first_ch = -1;

  for (size_t i = 0; i < NUM_SENSORS; i++) {
    adc_gpio_init(my_sensors[i].gpio_pin);
    uint ch = my_sensors[i].gpio_pin - 26;
    rb_mask |= (1 << ch);
    if (first_ch == -1) first_ch = ch;

    // --- SOLUÇÃO DE PRE-LOAD (Fim do gatilho falso no boot) ---
    // Faz uma leitura manual e imediata para carregar o filtro com o valor real
    adc_select_input(ch);
    uint16_t initial_val = adc_read();
    filtered_readings[i] = initial_val;
    raw_readings[i] = initial_val;

    // Se o valor inicial já estiver na janela, começa com a flag travada
    // Isso impede que o robô "trigue" ao ligar se já estiver em cima da linha
    if (initial_val >= my_sensors[i].threshold_min && initial_val <= my_sensors[i].threshold_max) {
      my_sensors[i].inside_window = true;
    }
  }

  adc_set_round_robin(rb_mask);
  adc_select_input(first_ch);
  adc_fifo_setup(true, true, 1, false, false);
  adc_hw->fcs = (adc_hw->fcs & ~(0x7 << 8)) | (4 << 8);  // Hardware Oversampling 16x
  adc_set_clkdiv(192);                                   // Taxa de 81.3 kHz
  adc_fifo_drain();

  // Configuração do DMA com Ring Buffer de 4 bytes (2 sensores)
  dma_channel = dma_claim_unused_channel(true);
  dma_cfg = dma_channel_get_default_config(dma_channel);
  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&dma_cfg, false);
  channel_config_set_write_increment(&dma_cfg, true);
  channel_config_set_dreq(&dma_cfg, DREQ_ADC);
  channel_config_set_ring(&dma_cfg, true, 2);

  dma_channel_configure(dma_channel, &dma_cfg, (void*)raw_readings, &adc_hw->fifo, NUM_SENSORS, true);

  multicore_fifo_clear_irq();
  irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_sio_fifo_irq_handler);
  irq_set_enabled(SIO_IRQ_PROC0, true);

  multicore_launch_core1(core1_main);
  adc_run(true);
}

// ============================================================================
// PROGRAMA PRINCIPAL (PID, ESTRATÉGIA E MOTORES)
// ============================================================================
int main() {
  stdio_init_all();
  sleep_ms(5000);

  printf("Sistema Iniciado. Taxa: 81.3 kHz. Boot Clean: Ativo.\n");
  setup_flexible_analog_system();

  while (true) {
    // Core 0 está totalmente livre para rodar seu PID de motores aqui!
    printf("Leituras: S0: %u | S1: %u\n", filtered_readings[0], filtered_readings[1]);
    sleep_ms(1000);
  }
}