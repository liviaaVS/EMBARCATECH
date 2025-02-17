#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "inc/ssd1306.h"
#include "math.h"
#include "pico/time.h"
#include "server.c"

// ------------------------------------------------------------
// Macros e Definições de Constantes
// ------------------------------------------------------------

// Começe por aqui, definindo o ponto da parada do ônibus em que o display foi instalado
int pontoDisplay = 3;

// tempo de atualização do display
#define DISPLAY_UPDATE_INTERVAL 10000

// LED
const uint LED_PIN = 12;

// UART
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// I2C para SSD1306
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// buzzer
#define BUZZER_PIN 21        // Defina o pino do buzzer (ajuste conforme necessário)
#define BUZZER_FREQUENCY 100 // Frequência do som do buzzer em Hz

// ------------------------------------------------------------
// Variáveis e Estruturas Globais
// ------------------------------------------------------------

// Buffer e área de renderização para o display SSD1306
uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area = {
    start_column : 0,
    end_column :
            ssd1306_width - 1,
    start_page : 0,
    end_page :
            ssd1306_n_pages - 1
};

int intervalo_aceleracao = 5; // Intervalo de aceleração (em segundos)

// ------------------------------------------------------------
// Implementação das Funções
// ------------------------------------------------------------

void printDebug()
{
    printf("Ponto: %d\n", pontoDisplay);
    printf("Velocidade: %d\n", onibus1.velocidade);
    printf("Quantidade de paradas: %d\n", onibus1.parou_qtd);
    printf("Tempo: %d\n", onibus1.tempo);
    printf("Proximo ponto: %d\n", onibus1.proximo_ponto);
    printf("Distancia ate o ponto: %d\n", onibus1.proximo_ponto - onibus1.distancia_percorrida);
}

int64_t terminal_alarm_callback(alarm_id_t id, void *user_data)
{
    struct onibus *bus = (struct onibus *)user_data;
    bus->terminal = false;
    printf("Terminal desativado após 5 segundos.\n");
    return 0; // Retorna 0 para não reagendar o alarme
}

int64_t led_off_callback(alarm_id_t id, void *user_data)
{
    gpio_put(LED_PIN, false); // Desliga o LED
    return 0;                 // Retorne 0 para não reagendar o alarme
}

// Calcula a distância entre a distância percorrida pelo ônibus e o ponto desejado
int calculate_dist_onibus_ponto(struct onibus onibus)
{
    int dist_percorrida = onibus.distancia_percorrida;
    int dist_ponto = linhas[onibus.linha].rota[pontoDisplay];
    // Se o ônibus já passou do ponto, calcula a distância restante da rota até o ponto
    if (dist_percorrida > dist_ponto)
    {
        return onibus.distancia_total - dist_percorrida + dist_ponto;
    }
    else
    {
        return dist_ponto - dist_percorrida;
    }
}

// Retorna um número aleatório entre min (inclusivo) e max (exclusivo)
int random_range(int min, int max)
{
    return (rand() % (max - min + 1)) + min;
}

// Calcula o tempo estimado (em segundos) para o ônibus chegar até o ponto desejado
double calculate_time_onibus_ponto(struct onibus onibus)
{
    int dist = calculate_dist_onibus_ponto(onibus);
    printf("Distância até o ponto %d: %d metros\n", pontoDisplay, dist);
    // Converte para double para que a divisão seja em ponto flutuante
    return ((double)dist) / ((double)onibus.velocidade);
}

void updateTime(struct onibus *onibus) // Agora recebe um ponteiro
{
    double time_seconds = calculate_time_onibus_ponto(*onibus);
    // Converte para minutos e arredonda para cima
    double time_minutes = ceil(time_seconds / 60.0);
    onibus->tempo = (int)time_minutes;
}
// Função que simula o andamento do ônibus
void acelerar(struct onibus *onibus)
{
    // Armazena a distância percorrida anteriormente pelo ônibus
    int old_dist = onibus->distancia_percorrida;

    // Armazena a distância até o próximo ponto de parada
    int bus_stop_distance = onibus->proximo_ponto;

    // Atualiza a distância percorrida pelo ônibus com base na sua velocidade
    onibus->distancia_percorrida += onibus->velocidade * 10;

    // Verifica se o ônibus passou do ponto de parada
    if (old_dist < bus_stop_distance && onibus->distancia_percorrida >= bus_stop_distance)
    {
        // Marca que o ônibus parou
        onibus->parou = 1;

        // Define o tempo de espera como 30 segundos
        onibus->wait_time = 30;

        // Incrementa o número de paradas feitas pelo ônibus
        onibus->parou_qtd++;

        // Exibe uma mensagem indicando que o ônibus parou no ponto atual
        printf("Ônibus parou no ponto %d!\n", onibus->ponto);

        // Verifica se o ônibus chegou ao último ponto da linha
        if (onibus->ponto + 1 == linhas[onibus->linha].pontos - 1)
        {
            // Se chegou ao último ponto, faz o ônibus voltar ao início da linha
            onibus->ponto = 0;
            onibus->distancia_percorrida = linhas[onibus->linha].rota[0];
            onibus->proximo_ponto = linhas[onibus->linha].rota[1];
            onibus->terminal = true;

            // Exibe uma mensagem informando que o ônibus retornou ao início da linha
            printf("Ônibus retornou ao início da linha!\n");

            // Define um alarme para ativar o terminal após 5 segundos
            add_alarm_in_ms(5000, terminal_alarm_callback, &onibus1, true);
        }
        else
        {
            // Caso contrário, o ônibus avança para o próximo ponto
            onibus->ponto++;
            onibus->distancia_percorrida = linhas[onibus->linha].rota[onibus->ponto];
            onibus->proximo_ponto = linhas[onibus->linha].rota[onibus->ponto + 1];
            onibus->terminal = false;
        }

        // Atualiza o tempo estimado para chegar ao próximo ponto
        updateTime(onibus);

        // Retorna da função pois o ônibus parou
        return;
    }

    // Verifica se é o momento de atualizar a velocidade do ônibus
    if (intervalo_aceleracao == 0)
    {
        // Gera uma nova velocidade aleatória para o ônibus
        onibus->velocidade = random_range(10, 15);

        // Reseta o intervalo de aceleração
        intervalo_aceleracao = 5;
    }
    else
    {
        // Caso contrário, diminui o intervalo de aceleração
        intervalo_aceleracao--;
    }

    // Atualiza o tempo estimado com a nova velocidade
    updateTime(onibus);
}


// Faz a leitura da velocidade do ônibus e armazena no array
void read_velocidade()
{
    // Inicializa o array na primeira chamada
    if (velocidades == NULL)
    {
        velocidades = (int *)malloc(capacidade_velocidades * sizeof(int));
    }

    // Se atingir a capacidade, dobra o tamanho do array
    if (cont_reads_velocidade >= capacidade_velocidades)
    {
        capacidade_velocidades *= 2;
        int *novo_array = (int *)realloc(velocidades, capacidade_velocidades * sizeof(int));

        velocidades = novo_array;
    }

    // Armazena a nova velocidade
    velocidades[cont_reads_velocidade] = onibus1.velocidade;
    cont_reads_velocidade++;
}

bool repeating_timer_callback(struct repeating_timer *t)
{
    printDebug();
    read_velocidade();

    gpio_put(LED_PIN, true); // Liga o LED
    add_alarm_in_ms(2000, led_off_callback, NULL, true);

    if (onibus1.wait_time > 0 && onibus1.parou == 1)
    {
        onibus1.wait_time -= 10;
        printf("Tempo de espera: %d segundos\n", onibus1.wait_time);
        return true;
    }
    else
    {
        if (onibus1.parou == 1)
        {
            onibus1.parou = 0;
            acelerar(&onibus1);
        }
        else
        {
            acelerar(&onibus1);
        }
    }

    return true;
}
// Envia um comando via UART (por exemplo, comandos AT)
void send_command(const char *cmd)
{
    uart_puts(UART_ID, cmd);
    uart_puts(UART_ID, "\r\n");
    sleep_ms(2000);
}

// Limpa o display SSD1306
void clear_screen()
{
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

// Exibe mensagens no display SSD1306
void sendMensage(char *line1, char *line2, char *line3)
{
    char *text[] = {
        "",
        line1,
        "",
        line2,
        "",
        line3,
    };
    int y = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

// Lê a temperatura do sensor interno (ADC canal 4)
float read_temperature()
{
    adc_select_input(4);
    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / (1 << 12);
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

void pwm_init_buzzer(uint pin)
{
    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(pin, 0);
}

// Definição de uma função para emitir um beep com duração especificada
void beep(uint pin, uint duration_ms)
{
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o duty cycle para 50% (ativo)
    pwm_set_gpio_level(pin, 2048);

    // Temporização
    sleep_ms(duration_ms);

    // Desativar o sinal PWM (duty cycle 0)
    pwm_set_gpio_level(pin, 0);

    // Pausa entre os beeps
    sleep_ms(500); // Pausa de 100ms
}

int64_t buzzer_off_callback(alarm_id_t id, void *user_data)
{
    gpio_put(BUZZER_PIN, 0); // Desliga o buzzer
    return 0;                // Retorna 0 para não reagendar o alarme
}

// ------------------------------------------------------------
// Função Principal
// ------------------------------------------------------------

int main()
{

    // Inicializa UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    sleep_ms(2000);

    stdio_init_all();
    sleep_ms(4000);

    // Inicializa I2C para o display SSD1306

    // Inicializa I2C e o display SSD1306
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();

    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Inicializa Wi-Fi
    if (cyw43_arch_init())
    {
        printf("Falha ao iniciar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    int cont = 1;
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0)
    {
        printf("Falha ao conectar ao Wi-Fi. Tentativa %d\n", cont);
        cont++;
        sleep_ms(1000);
    }

    // Configura o LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Configura o buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0); // Garante que começa desligado

    // Configura o temporizador repetitivo para alternar o LED
    struct repeating_timer timer;
    add_repeating_timer_ms(DISPLAY_UPDATE_INTERVAL, repeating_timer_callback, &onibus1, &timer);

    printf("Wi-Fi conectado!\n");

    pwm_init_buzzer(BUZZER_PIN);

    // caso queira que as interações aconteçam mais depressa, aumente os valores minimo e maximo de velocidade na linha 194 e nas configurações iniciais do onibus

    // configuração incial para testar o onibus chegando no ultimo ponto da rota e voltando ao terminal (inicio da rota)
    onibus1.velocidade = random_range(10, 15);
    onibus1.distancia_total = linhas[onibus1.linha].distancia;
    onibus1.proximo_ponto = linhas[onibus1.linha].rota[13];
    onibus1.ponto = 12;
    onibus1.distancia_percorrida = 34900;
    onibus1.parou_qtd = 12;
    onibus1.terminal = false;


    // configuração inicial para testar o onibus chegando no ponto em que o display foi instalado 
    // onibus1.velocidade = random_range(10, 15);
    // onibus1.distancia_total = linhas[onibus1.linha].distancia;
    // onibus1.proximo_ponto = linhas[onibus1.linha].rota[3];
    // onibus1.ponto = 2;
    // onibus1.distancia_percorrida = 7100;
    // onibus1.parou_qtd = 2;
    // onibus1.terminal = false;


    // configuração inicial para testar a rota completa do onibus
    // onibus1.velocidade = random_range(10, 15); 
    // onibus1.distancia_total = linhas[onibus1.linha].distancia;
    // onibus1.proximo_ponto = linhas[onibus1.linha].rota[1];
    // onibus1.ponto = 0;
    // onibus1.distancia_percorrida = 0;
    // onibus1.parou_qtd = 0;
    // onibus1.terminal = false;

    // Inicia o temporizador
    tempo_inicio = time_us_64();

    // Calcula o tempo para o ônibus chegar no ponto indicado
    onibus1.tempo = calculate_time_onibus_ponto(onibus1) / 60;
    printDebug();

    while (true)
    {
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo

        char mensagem[20];
        if (onibus1.terminal)
        {
            sprintf(mensagem, "N 29: NO TERMINAL");
            tempo_fim = time_us_64();
            tempo_gasto = (tempo_fim - tempo_inicio) / 1000000;
            dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
        }
        else if (onibus1.tempo == 0)
        {
            if (onibus1.distancia_percorrida < linhas[onibus1.linha].rota[pontoDisplay])
            {
                sprintf(mensagem, "N 29: CHEGANDO!");
            }
            else if (onibus1.parou == 1)
            {
                sprintf(mensagem, "N 29: CHEGOU! ");
                beep(BUZZER_PIN, 5000); // Emite um beep
            }
            
        }
        else
        {
            sprintf(mensagem, "N 29: %d min", onibus1.tempo + 7); // adiciona 7 minutos de margem de erro
        }
        clear_screen();
        sendMensage("P. de onibus 01", mensagem, "");
        sleep_ms(DISPLAY_UPDATE_INTERVAL + 100);
    }

    return 0;
}
