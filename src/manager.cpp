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
};

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
