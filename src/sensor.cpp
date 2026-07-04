/**
 * @file sensor.cpp
 * @brief Implementação do cliente de sensoriamento. Registra-se
 * no Gerenciador e envia periodicamente (a cada 1s) as leituras
 * de dados simulados do ambiente.
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
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace std;
// --- Variáveis Globais ---
uint8_t my_device_id;
DeviceType my_type;

// --- Assinaturas de Funções ---

/**
 * Estabelece conexão TCP com o Gerenciador.
 * Retorna o file descriptor do socket.
 */
int connect_to_manager(const std::string &ip, int port);

/**
 * Envia a mensagem HELLO (0x00) e aguarda o HELLO_ACK (0x01).
 * Retorna true se registrado com sucesso.
 */
bool register_sensor(int socket_fd);

/**
 * Gera uma leitura simulada do ambiente baseada no tipo do sensor.
 */
float generate_mock_reading();

/**
 * Envia DATA_REPORT (0x02) com a leitura atual.
 */
void send_data_report(int socket_fd, float reading);

// --- Função Principal ---
int main(int argc, char *argv[]) {
  if (argc != 5) {
    cerr << "Uso: " << argv[0] << " <IP> <PORTA> <DEVICE_ID> <TIPO_SENSOR>\n";
    cerr << "Tipos válidos: 0 (Temperatura), 1 (Umidade), 2 (CO2)\n";
    return 1;
  }

  string ip = argv[1];
  int port = atoi(argv[2]);
  my_device_id = static_cast<uint8_t>(atoi(argv[3]));

  int type_input = atoi(argv[4]);
  if (type_input < 0 || type_input > 2) {
    cerr << "[SENSOR]: Tipo inválido";
    return 1;
  }
  my_type = static_cast<DeviceType>(type_input);

  cout << "[SENSOR]: Inicializando com o ID " << (int)my_device_id << "e tipo "
       << type_input;

  // connect_to_manager()
  int socket_fd = connect_to_manager(ip, port);
  if (socket_fd < 0) {
    return 1;
  }

  // register_sensor()
  if (!register_sensor(socket_fd)) {
    close(socket_fd);
    return 1;
  }

  // Incializa a random seed
  srand(time(NULL));

  cout << "[SENSOR] Inicializando leitura ...";

  // Envia dados a cada 1 segundo
  while (true) {
    // gera valor aleatório para os sensores (similação)
    float reading = generate_mock_reading();

    // envia pela rede para o manager
    send_data_report(socket_fd, reading);

    // Espera por 1 segundo
    this_thread::sleep_for(chrono::seconds(1));
  }

  close(socket_fd);
  return 0;
}

int connect_to_manager(const string &ip, int port) {
  // Cria file descriptor do socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    cerr << "[SENSOR] Erro ao criar socket \n";
    return -1;
  }

  // Configuração da esturuta do endereço
  struct sockaddr_in serv_addr = {};
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Converte IP de string para o formato binário
  if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
    cerr << "[SENSOR]: Endereço de IP inválido";
    close(sock);
    return -1;
  }

  // Faz a conexão
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    cerr << "[SENSOR] Falha ao conectar\n";
    close(sock);
    return -1;
  }

  return sock;
}

bool register_sensor(int socket_fd) {

  // TODO: Modularizar com o que está em client
  GCPHeader header{};
  header.magic[0] = 'G';
  header.magic[1] = 'C';
  header.msg_type = static_cast<uint8_t>(MessageType::HELLO); // HELLO
  header.device_id = my_device_id;

  // Envia Header
  if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
    cerr << "[SENSOR]: Erro ao enviar o HEADER HELLO.\n";
    return false;
  }

  uint8_t type_value = static_cast<uint8_t>(my_type);
  if (send(socket_fd, &type_value, sizeof(type_value), 0) !=
      sizeof(type_value)) {
    cerr << "[SENSOR]: Erro ao enviar o payload de HELLO\n";
    return false;
  }

  // Aguarda o HELLO_ACK do MANAGER
  GCPHeader ack_header;
  int bytes_rec = recv(socket_fd, &ack_header, sizeof(ack_header), 0);

  if (bytes_rec <= 0) {
    cerr << "[SENSOR]: MANEGER não respondeu ou conectou\n ";
    return false;
  }

  if (ack_header.magic[0] != 'G' || ack_header.magic[1] != 'C') {
    cerr << "[SENSOR] Falha no registro: servidor não respondeu (formato "
            "inválido)";
    return false;
  }

  if (ack_header.msg_type == static_cast<uint8_t>(MessageType::HELLO_ACK)) {
    std::cout << "[SENSOR] Registrado com sucesso no MANAGER\n";
    return true;
  }

  std::cerr << "[SENSOR] Falha no registro: Mensagem inesperada recebida ("
            << (int)ack_header.msg_type << ")\n";
  return false;
}

void send_data_report(int socket_fd, float reading) {

  GCPHeader header{};
  header.magic[0] = 'G';
  header.magic[1] = 'C';
  header.msg_type = static_cast<uint8_t>(MessageType::DATA_REPORT);
  header.device_id = my_device_id;

  // Envia header
  if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
    cerr << "[SENSOR] Falha ao enviar o Header";
    return;
  }

  // Envia o payload
  if (send(socket_fd, &reading, sizeof(reading), 0) != sizeof(reading)) {
    cerr << "[SENSOR]: Falha ao enviar leitura de dados";
    return;
  }

  cout << "[SENSOR]: Leitura enviada: " << reading << endl;
}

float generate_mock_reading() {
  uint8_t type_value = static_cast<uint8_t>(my_type);

  if (type_value == static_cast<uint8_t>(DeviceType::TEMP_SENSOR)) {
    // Temperatura
    return 20.0f + (rand() % 5);

  } else if (type_value == static_cast<uint8_t>(DeviceType::HUMIDITY_SENSOR)) {
    // Umidade
    return 50.0f + (rand() % 10);
  } else if (type_value == static_cast<uint8_t>(DeviceType::CO2_SENSOR)) {
    // CO2
    return 800.0f + (rand() % 100);
  }

  return 0.0f;
}
