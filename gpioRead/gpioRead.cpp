#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

const uint S1 = 26;
const uint S2 = 22;
const uint S3 = 21;
const uint S4 = 11;
const uint S5 = 19;
const uint S6 = 18;
const uint S7 = 17;
const uint SM = 16;

// 7 primeiros pinos com Pull-Up, o último (SM) sem Pull-Up
const uint PINOS_SENSORES[8] = {S1, S2, S3, S4, S5, S6, S7, SM};

// Valor de contagem para o bloco de DMA resetar por hardware (Loop Infinito)
#define DMA_VALOR_RECARGA 50000

// Variável global de 32 bits que receberá a foto bruta dos pinos via DMA
volatile uint32_t sensor_gpios = 0;

int dma_chan;
uint pio_sm;

void setup_sensores_estaticos(PIO pio) {
    uint16_t instrucoes[4];
    uint idx = 0;

    // 1. Inicializa os 8 pinos
    for (int i = 0; i < 8; i++) {
        uint pino = PINOS_SENSORES[i];

        gpio_init(pino);
        gpio_set_dir(pino, GPIO_IN);

        // Ajuste preventivo para pinos do ADC (26 a 29) operarem no modo digital
        if (pino >= 26) {
            gpio_set_function(pino, GPIO_FUNC_SIO);
            gpio_set_input_enabled(pino, true);
        }

        // Regra de indexação: os 7 primeiros com Pull-Up, o último (SM) sem
        if (i == 7) {
            gpio_disable_pulls(pino);
        } else {
            gpio_pull_up(pino);
        }
    }

    // 2. PROGRAMA PIO: Tira a foto instantânea de 32 bits dos pinos
    instrucoes[idx++] = pio_encode_in(pio_pins, 32);   
    instrucoes[idx++] = pio_encode_push(false, true);  
    
    pio_program_t prg = {
        .instructions = reinterpret_cast<const uint16_t*>(instrucoes),
        .length = static_cast<uint8_t>(idx),
        .origin = -1
    };

    uint offset = pio_add_program(pio, &prg);
    pio_sm = pio_claim_unused_sm(pio, true);
    
    pio_sm_config config = pio_get_default_sm_config();
    sm_config_set_wrap(&config, offset, offset + idx - 1);
    sm_config_set_in_pins(&config, 0); 
    
    sm_config_set_in_shift(&config, true, false, 32);  // Shift para a direita

    // ========================================================================
    // TABELA DE CONFIGURAÇÃO DE FREQUÊNCIA DO PIO
    // ========================================================================
    // Comente/Descomente a taxa que deseja aplicar ao seu robô:
    
    // -> VELOCIDADE MÁXIMA ABSOLUTA (62.5 MHz / Atualiza a RAM a cada 16 ns)
    // sm_config_set_clkdiv(&config, 1.0f);

    // -> TAXA ULTRA RÁPIDA (25 MHz / Atualiza a RAM a cada 40 ns)
    // sm_config_set_clkdiv(&config, 2.5f);
    
    // -> TAXA DE ALTA PERFORMANCE (20 MHz / Atualiza a RAM a cada 50 ns)
    // sm_config_set_clkdiv(&config, 3.125f);

    // -> TAXA CRAVADA EM 10 MHz (Atualiza a RAM a cada 100 ns ou 0.1 µs)
    sm_config_set_clkdiv(&config, 6.25f);
    
    // -> TAXA PADRÃO ESTÁVEL (1 MHz / Atualiza a RAM a cada 1.0 µs)
    // sm_config_set_clkdiv(&config, 62.5f);
    
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
    
    // CONEXÃO DE HARDWARE: Quando o canal principal esgota suas transferências, 
    // ele ativa automaticamente o canal de recarga (Reloader)
    channel_config_set_chain_to(&c_main, dma_reloader);

    dma_channel_configure(
        dma_chan, &c_main,
        reinterpret_cast<volatile void*>(const_cast<uint32_t*>(&sensor_gpios)), 
        &pio->rxf[pio_sm],             
        DMA_VALOR_RECARGA, // Conta de 50.000 até 0 de forma decrescente                                                            
        false              // Não dispara imediatamente, aguarda o reloader
    );

    // CANAL RELOADER: Reescreve o registrador de contagem do canal principal
    dma_channel_config c_reload = dma_channel_get_default_config(dma_reloader);
    channel_config_set_transfer_data_size(&c_reload, DMA_SIZE_32);
    channel_config_set_read_increment(&c_reload, false);
    channel_config_set_write_increment(&c_reload, false);

    // Variável estática contendo o valor de reinicialização do contador
    static const uint32_t recarga_valores = DMA_VALOR_RECARGA;

    dma_channel_configure(
        dma_reloader, &c_reload,
        &dma_channel_hw_addr(dma_chan)->al1_transfer_count_trig, // Destino: Registrador de trigger do Principal
        &recarga_valores,                                        // Origem: O valor fixo 50.000
        1,                                                       // 1 escrita resolve o problema
        true                                                     // Starta o reloader para armar o sistema
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
    printf("Iniciando leitura de sensores com Motor de DMA Infinito...\n");
    
    setup_sensores_estaticos(pio0);

    while (true) {
        //Captura instantânea e atômica da variável atualizada pelo DMA na RAM
        uint32_t foto_bruta = sensor_gpios;

        uint8_t bit_S1 = (foto_bruta >> S1) & 1;
        uint8_t bit_S2 = (foto_bruta >> S2) & 1;
        uint8_t bit_S3 = (foto_bruta >> S3) & 1;
        uint8_t bit_S4 = (foto_bruta >> S4) & 1;
        uint8_t bit_S5 = (foto_bruta >> S5) & 1;
        uint8_t bit_S6 = (foto_bruta >> S6) & 1;
        uint8_t bit_S7 = (foto_bruta >> S7) & 1;
        uint8_t bit_SM = (foto_bruta >> SM) & 1;

        // Ordem visual no terminal: [SM][S7][S6][S5][S4][S3][S2][S1]
        uint8_t mascara_final = (bit_SM << 7) | (bit_S7 << 6) | (bit_S6 << 5) | 
                                (bit_S5 << 4) | (bit_S4 << 3) | (bit_S3 << 2) | 
                                (bit_S2 << 1) | (bit_S1 << 0);

        for (int i = 7; i >= 0; i--) {
            printf("%d", (mascara_final >> i) & 1);
        }
        printf("\n");

        sleep_ms(10);
    }
}