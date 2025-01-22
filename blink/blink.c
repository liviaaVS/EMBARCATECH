#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Define os pinos dos LEDs, do botão e do buzzer, além da frequência do buzzer
#define RED 12       // LED vermelho
#define YELLOW 11    // LED amarelo
#define GREEN 9      // LED verde
#define GREENP 17    // LED verde do pedestre
#define BUTTON_PIN 6 // Botão para ativar o modo pedestre
#define BUZZER_PIN 19 // Pino do buzzer
#define BUZZER_FREQUENCY 2000 // Frequência do som do buzzer em Hz

int value = 0; // Variável que controla o estado do semáforo (0: verde, 1: amarelo, 2: vermelho)
bool pedestre = false; // Variável para indicar se o modo pedestre está ativo

// Inicializa o PWM para o buzzer
void pwm_init_buzzer(uint pin) {
  gpio_set_function(pin, GPIO_FUNC_PWM); // Configura o pino para função PWM
  uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o "slice" do PWM associado ao pino

  pwm_config config = pwm_get_default_config(); // Obtém a configuração padrão de PWM
  pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Ajusta a frequência do PWM
  pwm_init(slice_num, &config, true); // Inicializa o PWM
  pwm_set_gpio_level(pin, 0); // Inicia o PWM com nível baixo (buzzer desligado)
}

// Emite um bip no buzzer por uma duração específica
void beep(uint pin, uint duration_ms) {
  uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o "slice" do PWM
  pwm_set_gpio_level(pin, 2048); // Define um nível médio para o PWM (som ativado)
  sleep_ms(duration_ms); // Mantém o som por `duration_ms` milissegundos
  pwm_set_gpio_level(pin, 0); // Desliga o som
  sleep_ms(100); // Pausa de 100 ms antes de permitir outro bip
}

// Rotina para ativar o modo pedestre
void rotinaSecond() {
  gpio_put(GREEN, 0); // Desliga o LED verde (carros)
  if (gpio_get(RED) == 0) { // Verifica se o LED vermelho está desligado por que não há 
    // sentido acender o led amarelo com o sinal já fechado
    gpio_put(YELLOW, 1); // Acende o LED amarelo
    sleep_ms(5000); // Aguarda 5 segundos
    gpio_put(YELLOW, 0); // Desliga o LED amarelo
    gpio_put(RED, 1); // Acende o LED vermelho (carros parados)
  }
  gpio_put(GREENP, 1); // Acende o LED verde dos pedestres
  beep(BUZZER_PIN, 15000); // Ativa o buzzer por 15 segundos
  gpio_put(GREENP, 0); // Desliga o LED verde dos pedestres
  gpio_put(RED, 0); // Desliga o LED vermelho
  value = -1; // Reseta o estado do semáforo para recomeçar no verde
}

// Observa o botão por um tempo e, se pressionado, ativa a rotina pedestre
void watchButton(int time) {
  for (int i = 0; i < time; i++) {
    sleep_ms(10); // Verifica o botão a cada 10 ms
    if (gpio_get(BUTTON_PIN) == 0) { // Se o botão for pressionado
      rotinaSecond(); // Ativa a rotina do pedestre
      return; // Sai da função
    }
  }
}

// Função principal
int main() {
  stdio_init_all(); // Inicializa as funções de entrada/saída padrão (e.g., printf)
  gpio_init(RED); gpio_init(YELLOW); gpio_init(GREEN); gpio_init(GREENP);
  gpio_init(BUTTON_PIN); gpio_init(BUZZER_PIN);

  // Configura os pinos como entrada ou saída
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_set_dir(BUZZER_PIN, GPIO_OUT);
  gpio_pull_up(BUTTON_PIN); // Ativa o pull-up no botão para evitar flutuações

  gpio_set_dir(RED, GPIO_OUT);
  gpio_set_dir(YELLOW, GPIO_OUT);
  gpio_set_dir(GREEN, GPIO_OUT);
  gpio_set_dir(GREENP, GPIO_OUT);

  pwm_init_buzzer(BUZZER_PIN); // Inicializa o PWM do buzzer

  // Loop principal do semáforo
  while (true) {
    switch (value) {
      case 0: // Estado verde (carros podem passar)
        gpio_put(GREEN, 1); // Acende o LED verde
        watchButton(800); // Observa o botão por 8 segundos
        gpio_put(GREEN, 0); // Desliga o LED verde
        break;

      case 1: // Estado amarelo (atenção)
        gpio_put(YELLOW, 1); // Acende o LED amarelo
        watchButton(200); // Observa o botão por 2 segundos
        gpio_put(YELLOW, 0); // Desliga o LED amarelo
        break;

      case 2: // Estado vermelho (carros parados)
        gpio_put(RED, 1); // Acende o LED vermelho
        gpio_put(GREENP, 1); // Acende o LED verde dos pedestres
        watchButton(1000); // Observa o botão por 10 segundos
        gpio_put(RED, 0); // Desliga o LED vermelho
        gpio_put(GREENP, 0); // Desliga o LED verde dos pedestres
        break;
    }

    value = (value + 1) % 3; // Alterna entre os estados (0 -> 1 -> 2 -> 0)
  }

  return 0; 
}