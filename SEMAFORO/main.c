#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define RED 12
#define YELLOW 11
#define GREEN 9
#define PINK 7
#define BUTTON_PIN 6
#define BUZZER_PIN 19
#define BUZZER_FREQUENCY 8000

int value = 0;
bool pedestre = false;

void pwm_init_buzzer(uint pin) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(pin);

  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096));
  pwm_init(slice_num, &config, true);
  pwm_set_gpio_level(pin, 0);
}

void beep(uint pin, uint duration_ms) {
  uint slice_num = pwm_gpio_to_slice_num(pin);
  pwm_set_gpio_level(pin, 2048);
  sleep_ms(duration_ms);
  pwm_set_gpio_level(pin, 0);
  sleep_ms(100);
}

void rotinaSecond() {
  gpio_put(GREEN, 0);
  gpio_put(RED, 0);

  gpio_put(YELLOW, 1);
  sleep_ms(5000);
  gpio_put(YELLOW, 0);
  gpio_put(RED, 1);
  gpio_put(PINK, 1);
  beep(BUZZER_PIN, 15000);
  gpio_put(PINK, 0);
  gpio_put(RED, 0);
  value = -1;
}

void watchButton(int time) {
  for (int i = 0; i < time; i++) {
    sleep_ms(10);
    if (gpio_get(BUTTON_PIN) == 0) {
      rotinaSecond();
      return;
    }
  }
}

int main() {
  stdio_init_all();
  gpio_init(RED);
  gpio_init(YELLOW);
  gpio_init(GREEN);
  gpio_init(PINK);
  gpio_init(BUTTON_PIN);
  gpio_init(BUZZER_PIN);

  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_set_dir(BUZZER_PIN, GPIO_OUT);
  gpio_pull_up(BUTTON_PIN);

  gpio_set_dir(RED, GPIO_OUT);
  gpio_set_dir(YELLOW, GPIO_OUT);
  gpio_set_dir(GREEN, GPIO_OUT);
  gpio_set_dir(PINK, GPIO_OUT);
  pwm_init_buzzer(BUZZER_PIN);

  while (true) {
    switch (value) {
      case 0:
        gpio_put(GREEN, 1);
        watchButton(800);
        gpio_put(GREEN, 0);
        break;

      case 1:
        gpio_put(YELLOW, 1);
        watchButton(200);
        gpio_put(YELLOW, 0);
        break;

      case 2:
        gpio_put(RED, 1);
        watchButton(1000);
        gpio_put(RED, 0);
        break;
    }

    value = (value + 1) % 3;
    sleep_ms(100);
  }

  return 0;
}
