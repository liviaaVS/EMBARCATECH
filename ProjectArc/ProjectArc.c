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

// ------------------------------------------------------------
// Macros e Definições de Constantes
// ------------------------------------------------------------
int pontoDisplay = 3;

// tempo de atualização do display
#define DISPLAY_UPDATE_INTERVAL 10000

// LED
const uint LED_PIN = 12;

// Wi-Fi e ThingSpeak
#define WIFI_SSID "brisa-4217471"
#define WIFI_PASS "1enxcbxm"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define API_KEY "LKQA003F1TG3E65T" // API Key do ThingSpeak

// UART
#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// I2C para SSD1306
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;


// buzzer
#define BUZZER_PIN 21  // Defina o pino do buzzer (ajuste conforme necessário)
#define BUZZER_FREQUENCY 100  // Frequência do som do buzzer em Hz

// ------------------------------------------------------------
// Variáveis e Estruturas Globais
// ------------------------------------------------------------

// variaveis de tempo
uint64_t tempo_inicio;
uint64_t tempo_fim;
uint64_t tempo_gasto;

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


// Dados das rotas
const int distanciaTotalRota1 = 35000; // metros
const int distanciaTotalRota2 = 30000; // metros
const int distanciaTotalRota3 = 28000; // metros

const int ponto1Rota1[] = {0, 1800, 4300, 7200, 9600, 12300, 15200, 17700, 20900, 23700, 26800, 30500, 34000, 35000};
const int ponto1Rota2[] = {0, 1300, 3900, 6800, 9900, 12900, 15800, 18300, 21500, 24400, 27300, 30000};
const int ponto1Rota3[] = {0, 1100, 3600, 5900, 8700, 11900, 14700, 17500, 20200, 23400, 26300, 28000};

// Estrutura que representa uma linha (rota) de ônibus
typedef struct
{
    int pontos;      // Número de paradas (excetuando a repetição do ponto inicial)
    int distancia;   // Distância total da rota (metros)
    const int *rota; // Vetor de posições dos pontos (metros)
} LinhaOnibus;

// Vetor global com as linhas de ônibus
const LinhaOnibus linhas[] = {
    {14, distanciaTotalRota1, ponto1Rota1},
    {12, distanciaTotalRota2, ponto1Rota2},
    {11, distanciaTotalRota3, ponto1Rota3}};

// Pontos de ônibus comuns (para referência)
const int pontosOnibus[] = {3, 5, 9};

// Estrutura que representa um ônibus
typedef struct onibus
{
    int linha;                // Índice da linha no vetor "linhas"
    int ponto;                // Ponto atual (índice no vetor de pontos da linha)
    int distancia_percorrida; // Distância percorrida na rota (metros)
    int velocidade;           // Velocidade em m/s
    int distancia_total;      // Distância total da rota (metros)
    int tempo;                // Tempo estimado para chegar a um ponto (segundos)
    int wait_time;            // Tempo de espera (em segundos)
    int parou_qtd;            // Indica quantas vezes o ônibus está parou
    int parou;                // Indica se o ônibus está parado
    int proximo_ponto;        // Próximo ponto de parada
    bool terminal;            // Indica se o ônibus está no terminal

} onibus;

// Cria um ônibus na linha 1 (índice 1 no vetor "linhas")
struct onibus onibus1 = {
    .linha = 0,
    .ponto = 0,
    .distancia_percorrida = 0,
    .velocidade = 0, // Velocidade aleatória entre 1 e 25 m/s
    .distancia_total = 0,
    .tempo = 0,
    .wait_time = 0,
    .parou = 0,
    .parou_qtd = 0,
    .proximo_ponto = 0,
    .terminal = true

};

struct onibus onibus2 = {
    .linha = 0,
    .ponto = 0,
    .distancia_percorrida = 0,
    .velocidade = 0, // Velocidade aleatória entre 1 e 25 m/s
    .distancia_total = 0,
    .tempo = 0,
    .wait_time = 0,
    .parou = 0,
    .parou_qtd = 0,
    .proximo_ponto = 0,
    .terminal = true

};
static struct onibus *onibus_list[] = {&onibus1, &onibus2};

int intervalo_aceleracao = 5; // Intervalo de aceleração (em segundos)

// Variáveis globais para conexão TCP com ThingSpeak
struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;

// ------------------------------------------------------------
// Protótipos das Funções
// ------------------------------------------------------------
bool repeating_timer_callback(struct repeating_timer *t);




#define INITIAL_SIZE 10  // Tamanho inicial do array

int *velocidades = NULL; // Ponteiro para armazenar as velocidades dinamicamente
int cont_reads_velocidade = 0;
int capacidade_velocidades = INITIAL_SIZE; // Capacidade atual do array

void read_velocidade();
void updateTime(struct onibus *onibus);
void acelerar(struct onibus *onibus);
int64_t terminal_alarm_callback(alarm_id_t id, void *user_data);
int64_t led_off_callback(alarm_id_t id, void *user_data);
int64_t buzzer_off_callback(alarm_id_t id, void *user_data);
void printDebug();
void read_velocidade();
void send_command(const char *cmd);
void clear_screen(void);
void sendMensage(char *line1, char *line2, char *line3);
float read_temperature(void);
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
int calculate_dist_onibus_ponto(struct onibus onibus);
double calculate_time_onibus_ponto(struct onibus onibus);
int random_range(int min, int max);

// ------------------------------------------------------------
// Implementação das Funções
// ------------------------------------------------------------

void printDebug()
{
    printf("Ponto: %d\n", pontoDisplay);
    for(int i = 0; i < 5; i++)
    {
        printf("Velocidade %d: %d\n", i, velocidades[i]);
    }
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

void updateTime(struct onibus *onibus) // Agora recebe um ponteiro
{
    double time_seconds = calculate_time_onibus_ponto(*onibus);
    // Converte para minutos e arredonda para cima
    double time_minutes = ceil(time_seconds / 60.0);
    onibus->tempo = (int)time_minutes;
}
void acelerar(struct onibus *onibus)
{

    int old_dist = onibus->distancia_percorrida;
    int bus_stop_distance = onibus->proximo_ponto;

    onibus->distancia_percorrida += onibus->velocidade * 10;

    if (old_dist < bus_stop_distance && onibus->distancia_percorrida >= bus_stop_distance)
    {
        onibus->parou = 1;
        onibus->wait_time = 30;
        onibus->parou_qtd++;

        printf("Ônibus parou no ponto %d!\n", onibus->ponto);

        if (onibus->ponto + 1 == linhas[onibus->linha].pontos - 1)
        {
            // Se chegou ao último ponto, volta direto para o primeiro
            onibus->ponto = 0;
            onibus->distancia_percorrida = linhas[onibus->linha].rota[0];
            onibus->proximo_ponto = linhas[onibus->linha].rota[1];
            onibus->terminal = true;

            printf("Ônibus retornou ao início da linha!\n");
            add_alarm_in_ms(5000, terminal_alarm_callback, &onibus1, true);
        }
        else
        {
            // Caso contrário, avança normalmente
            onibus->ponto++;
            onibus->distancia_percorrida = linhas[onibus->linha].rota[onibus->ponto];
            onibus->proximo_ponto = linhas[onibus->linha].rota[onibus->ponto + 1];
            onibus->terminal = false;
        }

        updateTime(onibus);
        return;
    }

    if (intervalo_aceleracao == 0)
    {
        onibus->velocidade = random_range(10, 15);
        intervalo_aceleracao = 5;
        updateTime(onibus);
    }
    else
    {
        intervalo_aceleracao--;
    }
}



void read_velocidade(){
    // Inicializa o array na primeira chamada
    if (velocidades == NULL) {
        velocidades = (int *)malloc(capacidade_velocidades * sizeof(int));
    }

    // Se atingir a capacidade, dobra o tamanho do array
    if (cont_reads_velocidade >= capacidade_velocidades) {
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

    if (onibus1.wait_time > 0)
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

// Callback para receber a resposta HTTP do ThingSpeak
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

// Callback quando a conexão TCP é estabelecida
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK)
    {
        printf("Erro na conexão TCP\n");
        return err;
    }
    printf("Conectado ao ThingSpeak!\n");

    int sum, media;
    for (int i = 0; i < cont_reads_velocidade; i++)
    {
        if (velocidades[i] != 0)
        {
            sum += velocidades[i];

        }
    }
    media = sum / cont_reads_velocidade;
    velocidades = NULL;
    cont_reads_velocidade = 0;
    capacidade_velocidades = INITIAL_SIZE;
    int qtd_paradas = onibus1.parou_qtd;
    int tempo_final_minutos = tempo_gasto;

    char request[256];
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field1=%d&field2=%d&field3=%d HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             API_KEY, media, qtd_paradas, tempo_final_minutos , THINGSPEAK_HOST);
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, http_recv_callback);

    return ERR_OK;
}

// Callback para resolução de DNS
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr)
    {
        printf("Endereço IP do ThingSpeak: %s\n", ipaddr_ntoa(ipaddr));
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT, http_connected_callback);
    }
    else
    {
        printf("Falha na resolução de DNS\n");
    }
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

// Calcula o tempo estimado (em segundos) para o ônibus chegar até o ponto desejado
double calculate_time_onibus_ponto(struct onibus onibus)
{
    int dist = calculate_dist_onibus_ponto(onibus);
    printf("Distância até o ponto %d: %d metros\n", pontoDisplay, dist);
    // Converte para double para que a divisão seja em ponto flutuante
    return ((double)dist) / ((double)onibus.velocidade);
}

// Retorna um número aleatório entre min (inclusivo) e max (exclusivo)
int random_range(int min, int max)
{
    return min + rand() % (max - min);
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





// ------------------------------------------------------------
// Função Principal
// ------------------------------------------------------------

void buzzer_tocar() {
    gpio_put(BUZZER_PIN, 1); // Liga o buzzer

}

int64_t buzzer_off_callback(alarm_id_t id, void *user_data) {
    gpio_put(BUZZER_PIN, 0); // Desliga o buzzer
    return 0;                // Retorna 0 para não reagendar o alarme
}

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
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    int cont = 1;
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0) {
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
    add_repeating_timer_ms(DISPLAY_UPDATE_INTERVAL, repeating_timer_callback, NULL, &timer);

    printf("Wi-Fi conectado!\n");

    onibus1.velocidade = random_range(10, 15);
    onibus1.distancia_total = linhas[onibus1.linha].distancia;
    onibus1.proximo_ponto = linhas[onibus1.linha].rota[13];
    onibus1.ponto = 12;
    onibus1.distancia_percorrida = 34900;
    onibus1.terminal = false;

    // Inicia o temporizador
    tempo_inicio = time_us_64();

    // Calcula o tempo para o ônibus chegar no ponto indicado
    onibus1.tempo = calculate_time_onibus_ponto(onibus1) / 60;
    printDebug();

    while (true) {
        char mensagem[20];
        if (onibus1.terminal) {
            sprintf(mensagem, "N 29: NO TERMINAL");
            tempo_gasto = (tempo_fim - tempo_inicio) / 1000000; // converte para segundos
            dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
        }
        else if (onibus1.tempo == 0) {
            if (onibus1.distancia_percorrida < linhas[onibus1.linha].rota[pontoDisplay]) {
                sprintf(mensagem, "N 29: CHEGANDO!");
            }
            else if (onibus1.parou == 1) {
                sprintf(mensagem, "N 29: CHEGOU! ");
                buzzer_tocar(); // Toca o buzzer quando o ônibus chegou
                add_alarm_in_ms(5000, buzzer_off_callback, NULL, false); // agenda o desligamento do buzzer para daqui a 5 segundos
            }
            else {
                sprintf(mensagem, "N 29: SAINDO...");
            }
        }
        else {
            sprintf(mensagem, "N 29: %d min", onibus1.tempo + 7); // adiciona 7 minutos de margem de erro
        }
        clear_screen();
        sendMensage("P. de onibus 01", mensagem, "");
        sleep_ms(DISPLAY_UPDATE_INTERVAL + 100);
    }

    return 0;
}
