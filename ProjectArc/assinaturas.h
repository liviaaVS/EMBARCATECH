#ifndef FUNCOES_H
#define FUNCOES_H

#include "pico/stdlib.h"
#include "pico/time.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"


// ------------------------------------------------------------
// Definição de estruturas
// ------------------------------------------------------------



typedef struct onibus {
    int velocidade;
    int linha;
    int ponto;
    int distancia_total;
    int distancia_percorrida;
    int proximo_ponto;
    int terminal;
    int tempo;
    int parou;
} onibus;

// ------------------------------------------------------------
// Assinatura dos metodos
// ------------------------------------------------------------


bool repeating_timer_callback(struct repeating_timer *t, void *user_data);
void read_velocidade();
void updateTime(struct onibus *onibus);
void acelerar(struct onibus *onibus);
int64_t terminal_alarm_callback(alarm_id_t id, void *user_data);
int64_t led_off_callback(alarm_id_t id, void *user_data);
int64_t buzzer_off_callback(alarm_id_t id, void *user_data);
void printDebug();
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

#endif // FUNCOES_H
