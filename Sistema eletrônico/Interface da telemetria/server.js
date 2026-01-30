const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');
const cors = require('cors');  // Importante!

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Middleware necessário
app.use(cors());  // Libera requisições de qualquer origem
app.use(express.json());  // Permite ler JSON do body
app.use(express.static(path.join(__dirname, 'public')));  // Serve os arquivos do frontend

// Dados recebidos do ESP32
let sensorData = {
  temp_motor: 0,
  temp_cvt: 0,
  combustivel: 0,
  combustivel_consumido: 0,
  velocidade: 0,
  rpm: 0,
  temp_motor_array: [],
  temp_cvt_array: [],
  time_array: [],
};

// Rota para receber dados do ESP32 via POST
app.post('/update', (req, res) => {
  const data = req.body;

  // Atualiza os dados existentes com os novos recebidos
  sensorData = { ...sensorData, ...data };

  // Envia os dados atualizados para todos os clientes WebSocket conectados
  wss.clients.forEach(client => {
  if (client.readyState === WebSocket.OPEN) {
    client.send(JSON.stringify({
      ...sensorData,
      temp_motor_array: sensorData.temp_motor_array || [],
      temp_cvt_array: sensorData.temp_cvt_array || [],
      time_array: sensorData.time_array || []
    }));
  }
});


  res.sendStatus(200);
});

// WebSocket: quando um cliente se conecta
wss.on('connection', ws => {
  console.log('Cliente conectado via WebSocket');

  // Envia os dados atuais assim que o cliente se conecta
  ws.send(JSON.stringify(sensorData));

  ws.on('close', () => {
    console.log('Cliente desconectado');
  });
});

// Inicia o servidor na porta do Render ou 3000 localmente
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Servidor rodando na porta ${PORT}`);
});
