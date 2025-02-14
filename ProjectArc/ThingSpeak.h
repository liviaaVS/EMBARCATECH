#include <WiFi.h>
#include <HTTPClient.h>

#define THINGSPEAK_URL "api.thingspeak.com"
#define THINGSPEAK_PORT_NUMBER 80
#define THINGSPEAK_HTTPS_PORT_NUMBER 443

class ThingSpeakClass {
  public:
    ThingSpeakClass() {
        resetWriteFields();
        this->lastReadStatus = TS_OK_SUCCESS;
    }

    bool begin() {
        // Inicializa o cliente Wi-Fi
        WiFi.begin("SSID", "PASSWORD");
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            printf("Connecting to WiFi...\n");
        }
        printf("Connected to WiFi\n");
        return true;
    }

    int writeField(unsigned long channelNumber, unsigned int field, int value, const char * writeAPIKey) {
        char valueString[10];
        itoa(value, valueString, 10);
        return writeField(channelNumber, field, valueString, writeAPIKey);
    }

    int writeField(unsigned long channelNumber, unsigned int field, const char * value, const char * writeAPIKey) {
        HTTPClient http;
        char url[100];
        snprintf(url, sizeof(url), "http://%s/update?api_key=%s&field%d=%s", THINGSPEAK_URL, writeAPIKey, field, value);

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            printf("Response: %s\n", payload.c_str());
        } else {
            printf("Error on HTTP request\n");
        }

        http.end();
        return httpCode;
    }

  private:
    void resetWriteFields() {
        // Implemente a lógica para resetar os campos
    }

    int lastReadStatus;
};

ThingSpeakClass ThingSpeak;

void setup() {
    ThingSpeak.begin();
    ThingSpeak.writeField(123456, 1, 42, "YOUR_API_KEY");
}

void loop() {
    // Loop principal
}