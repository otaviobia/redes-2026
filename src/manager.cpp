/**
 * @file manager.cpp
 * @brief Implementação do Gerenciador (Servidor central).
 * Responsável por aceitar conexões, armazenar leituras, rotear
 * requisições do cliente e aplicar o controle de histerese nos atuadores.
 *
 * @details
 * Trabalho Prático - Protocolo de Comunicação para Aplicações Smart
 * Implementação do Greenhouse Control Protocol (GCP)
 * Disciplina: SSC 0142 - Redes de Computadores
 * Instituição: ICMC - Universidade de São Paulo (USP)
 * Ano: 2026
 *
 * @authors
 * - Caio Capocasali (NUSP: 12541733)
 * - Otávio Biagioni Melo (NUSP: 15482604)
 */
#include "../include/protocol.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

// --- Variáveis Globais (Estado da Estufa) ---
std::mutex state_mutex; // Para proteger acesso concorrente das threads

// Armazena a última leitura de cada sensor (Key: device_id, Value:
// last_reading)
std::map<uint8_t, float> sensor_readings;

// Armazena os thresholds configurados pelo cliente (Key: device_id, Value:
// pair<min, max>)
std::map<uint8_t, std::pair<float, float>> sensor_thresholds;

// Mapeamento de sockets de atuadores conectados para enviar comandos (Key:
// device_id, Value: socket_fd)
std::map<uint8_t, int> connected_actuators;

// --- Assinaturas de Funções ---

/**
 * Configura o socket do servidor, faz o bind e o listen.
 * Retorna o file descriptor do socket servidor.
 */
int setup_server(int port);

/**
 * Thread para lidar com mensagens recebidas de um Sensor.
 * Trata HELLO e entra em loop esperando DATA_REPORT.
 */
void handle_sensor(int client_socket, uint8_t device_id);

/**
 * Thread para lidar com mensagens de um Atuador.
 * Trata HELLO e fica aguardando respostas SET_ACTUATOR_ACK.
 */
void handle_actuator(int client_socket, uint8_t device_id);

/**
 * Thread para lidar com comandos do Cliente Externo.
 * Trata CLIENT_GET e CLIENT_SET_THRESHOLD.
 */
void handle_client(int client_socket);

/**
 * Lógica de controle de histerese.
 * Verifica se a leitura exige ligar/desligar atuadores e envia SET_ACTUATOR.
 */
void evaluate_thresholds(uint8_t sensor_id, float reading);

// Protótipos de funções auxiliares
void send_hello_ack(int client_socket, GCPHeader header);
void receive_hello(int client_socket, GCPHeader &header, uint8_t &sensor_type);
void process_client_get(int client_socket, GCPHeader &header);
void process_client_set_threshold(int client_socket, GCPHeader &header);

// --- Função Principal ---
int main(int argc, char *argv[]) {
  // 1. Inicializar socket do servidor (ex: porta 8080)
  // 2. Loop infinito aceitando conexões:
  //    a. accept()
  //    b. Ler o primeiro pacote (HELLO ou mensagem de cliente)
  //    c. Identificar o tipo de dispositivo/cliente
  //    d. Disparar std::thread apropriada (handle_sensor, handle_actuator, ou
  //    handle_client)

  std::cout << "[MANAGER] Inicializando servidor da Estufa Inteligente...\n";

  return 0;
}

// FUNÇÕES PRINCIPAIS
int setup_server(int port) {
  int server_fd;
  struct sockaddr_in server_addr;

  // Criação do file descriptor do soclet
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd == -1) {
    cerr << "[MANAGER] Erro na criação do socket\n";
    return -1;
  }

  // Configurações do endereço
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  // Associa o socket à porta (bind)
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    cerr << "[MANAGER] Erro no bind da porta" << port << "\n";
    close(server_fd);
    return -1;
  }

  // Socket permanece em modo de escuta (listen)
  // O valor 10 dentro de listen representa o tamanho da fila de conexões
  // pendentes. Foi decidido de forma arbitrária
  if (listen(server_fd, 10) == -1) {
    cerr << "[MANAGER] Erro no listen\n";
    close(server_fd);
    return -1;
  }

  return server_fd;
}

// TODO:Tratamento dos lock guards
// TODO:Tratamento de erros
void handle_sensor(int client_socket, uint8_t device_id) {

  // Recebe os dados (HELLO)
  struct GCPHeader header;
  uint8_t sensor_type;

  receive_hello(client_socket, header, sensor_type);

  // Envia o HELLO_ACK
  send_hello_ack(client_socket, header);

  // Espera receber o DATA_REPORT
  while (true) {
    header = {};
    int bytes_rec = recv(client_socket, &header, sizeof(header), 0);

    if (bytes_rec <= 0) {
      cout << "[MANAGER] Conexão com sensor encerrada";
      close(client_socket);
      return;
    }

    if (header.msg_type == 0x02) {
      float data_report = {};
      recv(client_socket, &data_report, sizeof(float), 0);

      cout << "[MANAGER] Sensor" << (int)header.device_id << ": "
           << (float)data_report << endl;
    }
  }
}

// TODO: Tratamento dos lock guards
// TODO: Tratamento de erros
void handle_actuator(int client_socket, uint8_t device_id) {

  // Recebe os dados (HELLO)
  struct GCPHeader header;
  uint8_t sensor_type;

  receive_hello(client_socket, header, sensor_type);
  send_hello_ack(client_socket, header);

  // Espera receber o SET_ACTUATOR_ACK
  while (true) {
    header = {};
    int bytes_rec = recv(client_socket, &header, sizeof(header), 0);

    if (bytes_rec <= 0) {
      cout << "[MANAGER] Conexão com autador encerrada";
      close(client_socket);
      return;
    }

    if (header.msg_type == 0x04) {
      uint8_t set_actuator = {};
      recv(client_socket, &set_actuator, sizeof(uint8_t), 0);

      cout << "[MANAGER] Sensor" << (int)header.device_id << ": "
           << (uint8_t)set_actuator << endl;
    }
  }

  return;
}

/**
 * Thread para lidar com comandos do Cliente Externo.
 * Trata CLIENT_GET e CLIENT_SET_THRESHOLD.
 */
void handle_client(int client_socket) {
  struct GCPHeader header;

  // Mantendo a conexão aberta
  while (true) {
    int bytes_rec = recv(client_socket, &header, sizeof(GCPHeader), 0);

    if (bytes_rec <= 0) {
      cout << "[MANAGER] Conexão com o cliente encerrada\n";
      return;
    }

    if (header.msg_type == 0x05) { // CLIENT_GET
      process_client_get(client_socket, header);
    } else if (header.msg_type == 0x07) { // CLIENT_SET_THRESHOLD
      process_client_set_threshold(client_socket, header);
    }
  }
}

void process_client_get(int client_socket, GCPHeader &header) {
  float current_reading = 0.0f;

  {
    // Leitura do sensor
    lock_guard<mutex> lock(state_mutex);

    if (sensor_readings.find(header.device_id) != sensor_readings.end()) {
      current_reading = sensor_readings[header.device_id];
    } else {
      cout << "[MANAGER] Nenhuma leitura disponível" << endl;
    }
  }

  // Enviando a informação (CLIENT_GET_ACK) -> Header
  uint8_t client_get_ack[4] = {'G', 'C', 0x06, header.device_id};
  send(client_socket, client_get_ack, sizeof(client_get_ack), 0);

  // Enviando o payload -> Dado do sensor
  send(client_socket, &current_reading, sizeof(current_reading), 0);

  // DEBUG
  cout << "[MANAGER] Valor do dispositivo" << header.device_id << "enviado";
}

// TODO:Melhorar o tratamento de erros
void process_client_set_threshold(int client_socket, GCPHeader &header) {
  float min_value = 0.0f;
  float max_value = 0.0f;

  uint8_t received = 0x01;

  // Recebe os dados do payload
  int bytes_rec_min = recv(client_socket, &min_value, sizeof(min_value), 0);
  int bytes_rec_max = recv(client_socket, &max_value, sizeof(max_value), 0);

  if (bytes_rec_max <= 0 || bytes_rec_min <= 0) {
    cout << "[MANAGER]: Erro no recebimento dos thresholds";
    close(client_socket);
    return;
  }

  {
    lock_guard<mutex> lock(state_mutex);

    // Muda os valores de tresholds
    sensor_thresholds[header.device_id].first = min_value;
    sensor_thresholds[header.device_id].second = max_value;
    received = 0x00;
  }

  // Envia o CLIENT_SET_THRESHOLD
  // ENVIA O HEADER
  uint8_t threshold_ack[4] = {'G', 'C', 0x08, header.device_id};
  send(client_socket, &threshold_ack, sizeof(threshold_ack), 0);

  // Envia Payload
  send(client_socket, &received, sizeof(received), 0);

  // Debug
  if (received == 0x00)
    cout << "[MANAGER]: Erro ao configurar o treshold";
  else {
    cout << "[MANAGER]: Threshold setados com sucesso";
  }

  return;
}

// Recebe o HELLO
void receive_hello(int client_socket, GCPHeader &header, uint8_t &sensor_type) {

  int bytes_rec = recv(client_socket, &header, sizeof(GCPHeader), 0);

  if (bytes_rec <= 0)
    return;

  recv(client_socket, &sensor_type, sizeof(sensor_type), 0);

  cout << "[MANAGER] HELLO recebido pelo sensor do tipo" << (int)sensor_type
       << endl;
}

// Envia o HELLO_ACK
void send_hello_ack(int client_socket, GCPHeader header) {

  uint8_t hello_ack[4] = {'G', 'C', 0x01, header.device_id};
  send(client_socket, hello_ack, 4, 0);

  return;
}
