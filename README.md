Telemetria Veicular com ESP32, Display UnicViewAD e Envio de Dados Remoto

Projeto de telemetria automotiva utilizando ESP32 , com leitura de velocidade , RPM , consumo de combustível , temperatura do motor e da CVT , exibição em painel UnicViewAD , armazenamento de histórico em arrays , e envio de dados para Google Sheets e servidor Node.js via HTTP.

O sistema foi desenvolvido com foco em aplicações off-road / BAJA SAE , priorizando robustez , multitarefa (FreeRTOS) e suavização visual dos indicadores do painel.

📌 Funcionalidades

✅ Leitura de velocidade via sensor Hall

✅ Leitura de RPM do motor com interrupção por hardware

✅ Cálculo de consumo de combustível por pulsos

✅ Monitoramento de temperatura do motor e da CVT (MAX6675)

✅ Exibição de dados em Display UnicViewAD (LCM)

✅ Suavização visual de RPM e velocidade no painel

✅ Armazenamento de histórico de temperatura em matrizes

✅ Envio periódico de dados para:

📊 Planilhas Google (Google Apps Script)

🌐 Servidor Node.js (JSON via HTTP POST)

✅ Multitarefa com FreeRTOS (Tacômetro em Core dedicado)

✅ Reconexão automática ao Wi-Fi

🧠 Arquitetura do Sistema

O projeto utiliza uma arquitetura orientada a eventos , baseada em:

Interrupções por hardware para sensores críticos

FreeRTOS para separar o cálculo de RPM do loop principal

Central de retorno de chamada para atualização periódica do sistema

Comunicação serial dedicada com o painel gráfico

🔌 Sensores Utilizados
Sensor	‐
Sensor Hall (Roda)	Velocidade e distância percorrida
Sensor Hall (Motor)	RPM e consumo
MAX6675 (Motor)	Temperatura do motor
MAX6675 (CVT)	Temperatura da CVT
🖥️ Exibir UnicViewAD (LCM)

O painel obtém dados através da biblioteca UnicViewAD , utilizando variáveis ​​associadas a IDs de componentes gráficos .

Variáveis ​​do Painel
Variável	‐
VEL	velocidade
GASOLINA	Nível do livro (%)
KMRODADO	Quilo
TEMPMOTOR	Temperatura do motor
Hora/Min	Tempo de funcionamento
InteiroRpm	Parte inteira do RPM (x1000)
DecimalRpm	Parte decimal do RPM
⚙️ Multitarefa com FreeRTOS
Tacômetro de Tarefa (Núcleo 1)

Executa a cada 1 segundo

Calcular o RPM com base nos pulsos do sensor

Atualiza o painel com animação progressiva

Evita variações bruscas sem display
```
xTaskCreatePinnedToCore(
  TaskTacometro,
  "TacometroTask",
  2048,
  NULL,
  1,
  &TaskTacometroHandle,
  1
);
```
🎯 Suavização Visual (RPM e Velocidade)

Para melhorar a leitura no painel:

Pequenas variações → incremento suave

Grandes variações → incremento em passos maiores

Evita “saltos” bruscos nos indicadores

Essa lógica é aplicada tanto para:

RPM

velocidade

⛽ Consumo de Combustível

Baseado na contagem de pulsos do sensor

Conversão de pulsos → litros consumidos

Cálculo do percentual restante no tanque

```
combustivel = int(((capacidadeMaxTanque - litros) / capacidadeMaxTanque) * 100);
```

🌡️ Temperatura e Histórico

- Leitura a cada 5 segundos

- Armazenamento a cada 1 minuto em arrays:

 - temp_motor_array

 - temp_cvt_array

 - time_array

Esses dados são enviados ao servidor para gráficos históricos.

🌐 Envio de Dados
📊 Planilhas Google

Comunicação via HTTP GET

Utilize o Google Apps Script

Intervalo: 10 segundos

Dados enviados:

velocidade

RPM

Temperatura do motor

Temperatura da CVT

Combustível consumido

🌐 Servidor Node.js

Envio via HTTP POST (JSON)

Inclui dados instantâneos e históricos

Exemplo de carga útil:
```
{
  "temp_motor": 85.2,
  "temp_cvt": 72.1,
  "combustivel": 65,
  "combustivel_consumido": 1230.5,
  "velocidade": 42,
  "rpm": 3800,
  "temp_motor_array": [...],
  "temp_cvt_array": [...],
  "time_array": [...]
}
```
📡 Conectividade Wi-Fi

Reconexão automática em caso de queda

Modo WIFI_STA

Comunicação segura com Google Scripts ( WiFiClientSecure)

🔁 Princípio do Loop

O loop()mantém o sistema leve, delegando responsabilidades:
```
void loop() {
  manterConexaoWiFi();
  Callback();
  LocalDate();
}
```
🚗 Aplicação

Projeto desenvolvido para telemetria veicular em protótipos off-road , com foco em:

BAJA SAE

Monitoramento em tempo real

Análise do teste

Segurança e confiabilidade do sistema
