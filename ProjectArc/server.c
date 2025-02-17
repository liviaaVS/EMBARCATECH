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



// Variáveis globais para conexão TCP com ThingSpeak
struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;

// variaveis de tempo
uint64_t tempo_inicio;
uint64_t tempo_fim;
uint64_t tempo_gasto;



// Dados das rotas
const int distanciaTotalRota1 = 35000; // metros
const int distanciaTotalRota2 = 30000; // metros
const int distanciaTotalRota3 = 28000; // metros

const int ponto1Rota1[] = {0, 1800, 4300, 7200, 9600, 12300, 15200, 17700, 20900, 23700, 26800, 30500, 34000, 35000};
const int ponto1Rota2[] = {0, 1300, 3900, 6800, 9900, 12900, 15800};
const int ponto1Rota3[] = {0, 1100, 3600, 5900 };

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


#define INITIAL_SIZE 10  // Tamanho inicial do array de velocidades

int *velocidades = NULL; // Ponteiro para armazenar as velocidades dinamicamente
int cont_reads_velocidade = 0;
int capacidade_velocidades = INITIAL_SIZE; // Capacidade atual do array

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


// Wi-Fi e ThingSpeak
#define WIFI_SSID "brisa-4217471"
#define WIFI_PASS "1enxcbxm"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define API_KEY "LKQA003F1TG3E65T" // API Key do ThingSpeak

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

void resetData(){
        // Reseta o contador de leituras de velocidade
        cont_reads_velocidade = 0;
        capacidade_velocidades = INITIAL_SIZE;
    
        // Atualiza os dados do ônibus para o estado inicial
        onibus1.parou_qtd = 0;
        onibus1.parou = 0;
        onibus1.terminal = true;
        onibus1.distancia_percorrida = 0;
        onibus1.ponto = 0;
        onibus1.proximo_ponto = linhas[onibus1.linha].rota[3];
}

// Callback para conexão TCP com o ThingSpeak
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK)
    {
        printf("Erro na conexão TCP\n");
        return err;
    }
    printf("Conectado ao ThingSpeak!\n");

    int sum = 0;  // Inicializa a soma corretamente
    int media = 0;

    if (cont_reads_velocidade > 0)  // Evita divisão por zero
    {
        for (int i = 0; i < cont_reads_velocidade; i++)
        {
            sum += velocidades[i];
        }
        media = sum / cont_reads_velocidade;
    }

    // Calcula a quantidade de paradas e o tempo final
    int qtd_paradas = onibus1.parou_qtd;
    int tempo_final_minutos = tempo_gasto;
    
    // Reseta os dados para a próxima coleta
    resetData();

    char request[256];
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field1=%d&field2=%d&field3=%d HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             API_KEY, media, tempo_final_minutos, qtd_paradas, THINGSPEAK_HOST);

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

