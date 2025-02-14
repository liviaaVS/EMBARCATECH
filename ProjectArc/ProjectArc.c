#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/apps/http_client.h"

#define WIFI_SSID "brisa-4217471"
#define WIFI_PASSWORD "1enxcbxm"

#define HTTP_SERVER "api.thingspeak.com"
#define HTTP_PORT 80
#define HTTP_PATH "/channels/2840147/feeds.json?api_key=3R8KJP37R6GV8PYP&results=2"

// Função de callback para a resposta HTTP
void http_request_callback(struct httpc_connection *connection, httpc_result_t result, const char *data, size_t data_len) {
    if (result == HTTPC_RESULT_OK) {
        printf("Resposta HTTP recebida:\n");
        printf("%.*s\n", (int)data_len, data); // Exibe os dados da resposta
    } else {
        printf("Erro ao fazer requisição HTTP: %d\n", result); // Exibe erro
    }
}

// Função que faz a requisição HTTP
void make_http_request() {
    struct httpc_connection *connection = NULL;
    err_t err;
    
    // Resolver nome de domínio para IP
    ip_addr_t server_ip;
    err = dns_gethostbyname(HTTP_SERVER, &server_ip, NULL, NULL);
    if (err != ERR_OK) {
        printf("Erro ao resolver DNS: %d\n", err);
        return;
    }

    printf("Servidor IP: %s\n", ipaddr_ntoa(&server_ip));

    // Iniciar conexão HTTP
    err = httpc_get_file(&server_ip, HTTP_PORT, HTTP_PATH, NULL, http_request_callback, NULL, &connection);
    if (err != ERR_OK) {
        printf("Erro ao iniciar requisição HTTP: %d\n", err);
        return;
    }
}

int main() {
    stdio_init_all();

    // Inicializar Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi!\n");
        return -1;
    }

    // Conectar ao Wi-Fi
    printf("Conectando ao Wi-Fi...\n");
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi!\n");
        return -1;
    }

    printf("Conectado! IP obtido.\n");

    // Fazer requisição HTTP GET
    make_http_request();

    // Manter o programa rodando
    while (true) {
        sleep_ms(1000);
    }

    return 0;
}
