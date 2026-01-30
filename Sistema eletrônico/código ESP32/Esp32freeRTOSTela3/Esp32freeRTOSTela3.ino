#include <UnicViewAD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "max6675.h"

// WiFi Configuração
const char* ssid = "WifidoBAJA";
const char* password = "#7355608Tr";
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbz6IqDX-IFlaIGtdK53P2bHRpDCEOLWGykY-QTOJcuwpAKan5_Dc9Oo55o4eWZ5B9zGlg/exec";
const char* servidorNode = "https://reposit-rio.onrender.com/update";

// Sensores
int PinoHall = 4;
int PinoMotor = 15;
int thermoDO = 19;
int thermoCLK = 18;
int thermoCS1 = 5;
int thermoCS2 = 12;

// Define a taxa de transmissão de dados do Display com o ESP32
const long lcmBaudrate = 115200;
LCM Lcm(Serial);

// Cria os objetos para comunicação com a tela, os parâmetros são definidos com base nos respectivos Icons do projeto da tela
LcmVar VEL(500);     
//LcmVar RPM(501);
LcmVar GASOLINA(502);
LcmVar Min(503);
LcmVar KMRODADO(504);
LcmVar TEMPMOTOR(505);
LcmVar Hora(506);
LcmVar InteiroRpm(511);
LcmVar DecimalRpm(512);

std::vector<float> temp_motor_array;
std::vector<float> temp_cvt_array;
std::vector<float> time_array;

MAX6675 thermoMotor(thermoCLK, thermoCS1, thermoDO);
MAX6675 thermoCVT(thermoCLK, thermoCS2, thermoDO);

// Variáveis
volatile unsigned long VoltasHall = 0;
volatile unsigned long PulsosMotor = 0;
volatile unsigned long PulsosComb = 0;
volatile unsigned long PulsosReal = 0;

unsigned long LastArrayTime = 0, lastTime = 0, lastTempTime = 0, lastTimeVel = 0, lastTimeRpm = 0, LastLoggerTime = 0, LastServerTime = 0;

float temp_motor = 0, temp_cvt = 0, litros = 0, kmRodado = 0;
int velocidade = 0, rpm = 0, combustivel = 0, velocidadeAtual = 0, rpmAtual = 0;
const int capacidadeMaxTanque = 5670;

// FreeRTOS e multitarefa
TaskHandle_t TaskTacometroHandle;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Interrupções
void IRAM_ATTR ContarVoltasHall() {
  velocidade = (1.7898 / (millis() - lastTimeVel)) * 3600;
  kmRodado += 1.7898 / 1000;
  lastTimeVel = millis();
}

void IRAM_ATTR ContarPulsosMotor() {
  unsigned long currentTime = millis();
  if (currentTime - lastTimeRpm > 10) {
    portENTER_CRITICAL_ISR(&mux);
    PulsosMotor++;
    PulsosComb++;
    portEXIT_CRITICAL_ISR(&mux);
    lastTimeRpm = currentTime;
  }
}

// Task rodando no Core 1 (APP_CPU_NUM)
void TaskTacometro(void* parameter) {
  static unsigned long ultimoTempo = 0;

  for (;;) {
    if (millis() - ultimoTempo >= 1000) {
      ultimoTempo = millis();

      portENTER_CRITICAL(&mux);
      unsigned long pulsos = PulsosMotor;
      PulsosMotor = 0;
      portEXIT_CRITICAL(&mux);

      rpm = pulsos * 60;

      Serial.print("Pulsos qtd: ");
      Serial.println(pulsos);

      int novoRPM = rpm;
      int diferenca = abs(novoRPM - rpmAtual);

      if (novoRPM > rpmAtual) {
        // Caso 1: diferença <= 1000 (sobe de 100 em 100)
        if (diferenca <= 1000) {
          for (int i = rpmAtual; i <= novoRPM; i+= 100) {
            float rpmFmt = i * 0.001;
            int inteiroRpm = int(rpmFmt);
            float decimalRpm = (rpmFmt - inteiroRpm) * 10;
            if (decimalRpm >= 9) {
              decimalRpm = 9;
            }
            InteiroRpm.write(inteiroRpm);
            DecimalRpm.write(decimalRpm);
          }
        }
        // Caso 2: diferença > 1000 (sobe de 500 em 500)
        else {
          for (int i = rpmAtual; i <= novoRPM; i += 500) {
            float rpmFmt = i * 0.001;
            int inteiroRpm = int(rpmFmt);
            float decimalRpm = (rpmFmt - inteiroRpm) * 10;
            if (decimalRpm >= 9) {
              decimalRpm = 9;
            }
            InteiroRpm.write(inteiroRpm);
            DecimalRpm.write(decimalRpm);
          }
        }
      } 
      else if (novoRPM < rpmAtual) {
        // Caso 3: diferença <= 1000 (desce de 100 em 100)
        if (diferenca <= 10) {
          for (int i = rpmAtual; i >= novoRPM; i-= 100) {
            float rpmFmt = i * 0.001;
            int inteiroRpm = int(rpmFmt);
            float decimalRpm = (rpmFmt - inteiroRpm) * 10;
            if (decimalRpm >= 9) {
              decimalRpm = 9;
            }
            InteiroRpm.write(inteiroRpm);
            DecimalRpm.write(decimalRpm);
          }
        }
        // Caso 4: diferença > 1000 (desce de 500 em 500)
        else {
          for (int i = rpmAtual; i >= novoRPM; i -= 500) {
            float rpmFmt = i * 0.001;
            int inteiroRpm = int(rpmFmt);
            float decimalRpm = (rpmFmt - inteiroRpm) * 10;
            if (decimalRpm >= 9) {
              decimalRpm = 9;
            }
            InteiroRpm.write(inteiroRpm);
            DecimalRpm.write(decimalRpm);
          }
        }
      } 
      else {
        // Caso 5: valores iguais
        float rpmFmt = novoRPM * 0.001;
        int inteiroRpm = int(rpmFmt);
        float decimalRpm = (rpmFmt - inteiroRpm) * 10;
        if (decimalRpm >= 9) {
          decimalRpm = 9;
          }
        InteiroRpm.write(inteiroRpm);
        DecimalRpm.write(decimalRpm);
      }

      // Atualiza o valor de referência
      rpmAtual = novoRPM;

      /*float rpmFmt = rpm * 0.001;
      int inteiroRpm = int(rpmFmt);
      float decimalRpm = (rpmFmt - inteiroRpm) * 10;
      if (decimalRpm >= 9) {
        decimalRpm = 9;
      }

      InteiroRpm.write(inteiroRpm);
      DecimalRpm.write(decimalRpm);*/

      Serial.print("rpm: ");
      Serial.println(rpm);
      /*Serial.print("rpm formatado p/ painel: ");
      Serial.print(inteiroRpm);
      Serial.print(".");
      Serial.println(decimalRpm);*/
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Funções auxiliares
void Velocidade() {
  int novaVelocidade = velocidade;
  int diferenca = abs(novaVelocidade - velocidadeAtual);

  if (novaVelocidade > velocidadeAtual) {
    // Caso 1: diferença <= 10 (sobe de 1 em 1)
    if (diferenca <= 10) {
      for (int i = velocidadeAtual; i <= novaVelocidade; i++) {
        VEL.write(i);
      }
    }
    // Caso 2: diferença > 10 (sobe de 10 em 10)
    else {
      for (int i = velocidadeAtual; i <= novaVelocidade; i += 10) {
        VEL.write(i);
      }
    }
  } 
  else if (novaVelocidade < velocidadeAtual) {
    // Caso 3: diferença <= 10 (desce de 1 em 1)
    if (diferenca <= 10) {
      for (int i = velocidadeAtual; i >= novaVelocidade; i--) {
        VEL.write(i);
      }
    }
    // Caso 4: diferença > 10 (desce de 10 em 10)
    else {
      for (int i = velocidadeAtual; i >= novaVelocidade; i -= 10) {
        VEL.write(i);
      }
    }
  } 
  else {
    // Caso 5: valores iguais
    VEL.write(novaVelocidade);
  }

  velocidadeAtual = novaVelocidade;

  //VEL.write(velocidade);
  KMRODADO.write(int(kmRodado));

  Serial.print("velocidade: ");
  Serial.println(velocidade);
  Serial.print("km rodado: ");
  Serial.println(kmRodado);
  velocidade = 0;
}

void Temperatura() {
  if (millis() - lastTempTime >= 5000) {
    temp_motor = thermoMotor.readCelsius();
    temp_cvt = thermoCVT.readCelsius();
  } else {
    temp_motor = 0;
    temp_cvt = 0;
  }

  if (millis() - LastArrayTime >= 60000) {
    temp_motor_array.push_back(temp_motor);
    temp_cvt_array.push_back(temp_cvt);
    time_array.push_back(millis() / 60000.0);
    LastArrayTime = millis();
  }

  TEMPMOTOR.write((int)temp_motor);

  Serial.print("motor: ");
  Serial.println(temp_motor);
  Serial.print("cvt: ");
  Serial.println(temp_cvt);
}

void Consumo() {
  portENTER_CRITICAL(&mux);
  unsigned long pulsos = PulsosComb;
  portEXIT_CRITICAL(&mux);

  PulsosReal = pulsos / 2;
  litros = map(PulsosReal, 0, 235750, 0, 2500);
  combustivel = int(((capacidadeMaxTanque - litros) / capacidadeMaxTanque) * 100);

  GASOLINA.write(combustivel);

  Serial.print("litros consumidos: ");
  Serial.println(litros);
  Serial.print("tanque: ");
  Serial.print(combustivel);
  Serial.println("%");
}

void enviarDadosGoogleSheets() {
  if (millis() - LastLoggerTime >= 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      String url = String(googleScriptUrl) +
                   "?velocidade=" + velocidade +
                   "&tempCVT=" + temp_cvt +
                   "&tempMotor=" + temp_motor +
                   "&rpm=" + rpm +
                   "&gasolina=" + litros;
      http.begin(client, url);
      http.GET();
      http.end();
    }
    LastLoggerTime = millis();
  }
}

String arrayToJson(const std::vector<float>& arr) {
  String json = "[";
  for (size_t i = 0; i < arr.size(); i++) {
    json += String(arr[i], 2);
    if (i < arr.size() - 1) json += ",";
  }
  json += "]";
  return json;
}

void enviarDadosServidor() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(servidorNode);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"temp_motor\":" + String(temp_motor, 2) + ",";
    json += "\"temp_cvt\":" + String(temp_cvt, 2) + ",";
    json += "\"combustivel\":" + String(combustivel) + ",";
    json += "\"combustivel_consumido\":" + String(litros, 2) + ",";
    json += "\"velocidade\":" + String(velocidade) + ",";
    json += "\"rpm\":" + String(rpm) + ",";
    json += "\"temp_motor_array\":" + arrayToJson(temp_motor_array) + ",";
    json += "\"temp_cvt_array\":" + arrayToJson(temp_cvt_array) + ",";
    json += "\"time_array\":" + arrayToJson(time_array);
    json += "}";

    int httpCode = http.POST(json);

    if (httpCode > 0) {
      Serial.print("Servidor: OK - Código ");
      Serial.println(httpCode);
    } else {
      Serial.print("Erro servidor: ");
      Serial.println(httpCode);
    }
    http.end();
    LastServerTime = millis();
  }
}

void manterConexaoWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    for (int n = 0; n < 5; n++) {
      Serial.print('.');
      delay(1000);
    }
  }
}

void LocalDate() {
  int minutosTotais = millis() / 60000;
  int horas = minutosTotais / 60;
  int minutosRestantes = minutosTotais % 60;
  Min.write(minutosRestantes); 
  Hora.write(horas);
}

void Callback() {
  if (millis() - lastTime >= 500) {
    Consumo();
    // Tacometro();  -> Removido! Agora roda como task
    Velocidade();
    Temperatura();
    enviarDadosGoogleSheets();
    enviarDadosServidor();
    lastTime = millis();

    temp_motor = 0;
    temp_cvt = 0;
  }
}

void setup() {
  //Serial.begin(115200);
  Lcm.begin();         //  Inicia e configura o baudrate definido no LCM
  Serial.begin(lcmBaudrate);
  WiFi.mode(WIFI_STA);
  manterConexaoWiFi();
  Serial.println('.');
  Serial.println(WiFi.localIP());

  pinMode(PinoHall, INPUT_PULLUP);
  pinMode(PinoMotor, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PinoHall), ContarVoltasHall, RISING);
  attachInterrupt(digitalPinToInterrupt(PinoMotor), ContarPulsosMotor, FALLING);

  // Cria task Tacômetro no núcleo 1
  xTaskCreatePinnedToCore(
    TaskTacometro,         // Função
    "TacometroTask",       // Nome da task
    2048,                  // Tamanho da stack
    NULL,                  // Parâmetro
    1,                     // Prioridade
    &TaskTacometroHandle,  // Handle da task
    1                      // Core 1 (APP_CPU_NUM)
  );
}

void loop() {
  manterConexaoWiFi();
  Callback();
  LocalDate();
}
