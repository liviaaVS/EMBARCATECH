### Semáforo com Buzzer e Botão no Raspberry Pi Pico

Este projeto implementa um semáforo usando o Raspberry Pi Pico, com LEDs representando as cores de um semáforo, um botão para interagir com o estado do semáforo, e um buzzer que emite som durante determinadas etapas. O programa utiliza a biblioteca de SDK padrão do Pico para controle de hardware.

---

#### 📥 **Baixar e Simular**
- Você pode baixar o código fonte diretamente do Wokwi:
  [Projeto no Wokwi](https://wokwi.com/projects/417938395227730945)

- Simule o projeto no emulador do Wokwi:
  [Clique aqui para simular](https://wokwi.com/projects/417938395227730945)

---

### ⚙️ **Requisitos**
- **Hardware**:
  - Raspberry Pi Pico
  - 4 LEDs:
    - Vermelho (Red) conectado ao pino GPIO 12
    - Amarelo (Yellow) conectado ao pino GPIO 11
    - Verde (Green) conectado ao pino GPIO 9
    - Rosa (Pink) conectado ao pino GPIO 7
  - Um botão conectado ao pino GPIO 6
  - Um buzzer conectado ao pino GPIO 19
  - Resistores para LEDs e botão (para limitar a corrente e evitar problemas no circuito)
- **Software**:
  - Raspberry Pi Pico SDK configurado no ambiente
  - Compilador C (como GCC)
  - Biblioteca `pico/stdlib` para GPIO e PWM

---

### 📖 **Descrição do Projeto**
Este projeto é um simulador de semáforo com as seguintes funcionalidades:

1. **Controle de LEDs**:
   - O semáforo alterna entre os estados Verde, Amarelo e Vermelho.
   - Um LED verde claro adicional é ativado durante a sinalização para pedestres.

2. **Buzzer**:
   - O buzzer emite um som contínuo enquanto o LED verde claro está ligado, indicando passagem segura para pedestres.

3. **Botão de Pedestre**:
   - O botão conectado ao pino GPIO 6 interrompe o ciclo normal do semáforo e ativa a rotina de pedestres:
     - O LED amarelo pisca por 5 segundos.
     - O LED vermelho acende.
     - O LED verde claro (indicação para pedestres) e o buzzer são ativados por 15 segundos.

---

### 🔧 **Configuração do Circuito**
Conecte os componentes como descrito abaixo:

| Componente | GPIO (Pino) | Função |
|------------|-------------|--------|
| LED Vermelho (RED) | 12 | Saída |
| LED Amarelo (YELLOW) | 11 | Saída |
| LED Verde (GREEN) | 9 | Saída |
| LED Rosa (PINK) | 7 | Saída |
| Botão | 6 | Entrada (com pull-up interno) |
| Buzzer | 19 | Saída PWM |

---

2. **Configurar o SDK do Pico**:
   Siga as instruções na [Documentação do Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).



### 📂 **Estrutura do Projeto**
```plaintext
├── CMakeLists.txt      # Configuração do build
├── main.c              # Código principal do semáforo
├── README.md           # Este arquivo
```

---

### 🚦 **Ciclo do Semáforo**
1. **Estado Verde**:
   - O LED verde acende por 8 segundos.
   - Observa o botão durante esse tempo.
2. **Estado Amarelo**:
   - O LED amarelo acende por 2 segundos.
   - Observa o botão durante esse tempo.
3. **Estado Vermelho**:
   - O LED vermelho acende por 10 segundos.
   - Observa o botão durante esse tempo.

---

### 📞 **Interrupção para Pedestres**
- Quando o botão é pressionado, o ciclo normal do semáforo é interrompido:
  1. LED amarelo por 5 segundos.
  2. LED vermelho e LED verde claro por 5 segundos, enquanto o buzzer emite som.

---

### 🛡️ **Licença**
Este projeto está licenciado sob a licença MIT. Sinta-se à vontade para usar, modificar e distribuir.