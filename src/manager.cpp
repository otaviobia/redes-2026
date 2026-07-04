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
bool send_hello_ack(int client_socket, GCPHeader header);
bool receive_hello(int client_socket, GCPHeader &header, uint8_t &sensor_type);
void process_client_get(int client_socket, GCPHeader &header);
void process_client_set_threshold(int client_socket, GCPHeader &header);

// --- Função Principal ---
int main(int argc, char *argv[]) {

  cout << "[MANAGER] Inicializando servidor da Estufa Inteligente" << endl;

  // Incia socket do servidor (porta 8080)
  int server_fd = setup_server(8080);

  if (server_fd == -1) {
    return 1;
  }

  // Loop para aceitar conexões
  while (true) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // aceita a conexão
    int client_socket =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket == -1) {
      cerr << "[MANAGER]: Erro ao aceitar conexão\n";
      continue;
    }

    // Leitura do primeiro pacote
    struct GCPHeader header;
    int bytes_peeked = recv(client_socket, &header, sizeof(GCPHeader), MSG_PEEK | MSG_WAITALL);

    if (bytes_peeked < (int)sizeof(GCPHeader)) {
      close(client_socket);
      continue;
    }

    // Identifica o tipo do dispositivo/cliente
    if (header.msg_type == static_cast<uint8_t>(MessageType::HELLO)) {
      // Mensagem de Hello
      uint8_t buffer[5];
      int peek = recv(client_socket, buffer, 5, MSG_PEEK | MSG_WAITALL);

      if (peek == 5) {
        uint8_t msg_type = buffer[4];

        if (msg_type <= 0x02) {
          thread(handle_sensor, client_socket, header.device_id).detach();

        } else {
          thread(handle_actuator, client_socket, header.device_id).detach();
        }
      } else {
        close(client_socket);
      }

    } else {
      // Se não for HELLO, é a mensagem do cliente mandando GET ou SET_THRESHOLD
      thread(handle_client, client_socket).detach();
    }
  }

  close(server_fd);

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

void handle_sensor(int client_socket, uint8_t device_id) {

  // Recebe os dados (HELLO)
  struct GCPHeader header;
  uint8_t sensor_type;

  if (!receive_hello(client_socket, header, sensor_type))
    return;

  // Envia o HELLO_ACK
  if (!send_hello_ack(client_socket, header))
    return;

  // Espera receber o DATA_REPORT
  while (true) {
    header = {};
    int bytes_rec = recv(client_socket, &header, sizeof(header), 0);

    if (bytes_rec <= 0) {
      cout << "[MANAGER] Conexão com sensor encerrada\n";
      close(client_socket);
      return;
    }

    if (header.msg_type == static_cast<uint8_t>(MessageType::DATA_REPORT)) {
      float data_report = {};
      recv(client_socket, &data_report, sizeof(float), 0);

      {
        lock_guard<mutex> lock(state_mutex);

        // Atualiza os valores recebidos
        sensor_readings[header.device_id] = data_report;

        // Evita poluição visual (deixar só pra debug)
        // cout << "[MANAGER] Sensor" << (int)header.device_id << ": " (float)data_report << endl;
      }

      evaluate_thresholds(header.device_id, data_report);
    }
  }
}

void handle_actuator(int client_socket, uint8_t device_id) {

  // Recebe os dados (HELLO)
  struct GCPHeader header;
  uint8_t sensor_type;

  if (!receive_hello(client_socket, header, sensor_type))
    return;
  if (!send_hello_ack(client_socket, header))
    return;

  // Registra o autador em connected_actuators
  {
    lock_guard<mutex> lock(state_mutex);
    connected_actuators[header.device_id] = client_socket;
  }

  // Espera receber o SET_ACTUATOR_ACK
  while (true) {
    header = {};
    int bytes_rec = recv(client_socket, &header, sizeof(header), 0);

    if (bytes_rec <= 0) {
      lock_guard<mutex> lock(state_mutex);
      cout << "[MANAGER] Conexão com atuador encerrada\n";
      connected_actuators.erase(header.device_id);
      close(client_socket);
      return;
    }

    if (header.msg_type ==
        static_cast<uint8_t>(MessageType::SET_ACTUATOR_ACK)) {
      uint8_t set_actuator = {};
      if (recv(client_socket, &set_actuator, sizeof(uint8_t), 0) !=
          sizeof(uint8_t)) {
        cerr << "[MANAGER] Erro ao recever ACK do atuador";
        close(client_socket);
        return;
      }

      cout << "[MANAGER] Sensor" << (int)header.device_id << ": "
           << (uint8_t)set_actuator << endl;
    }
  }

  return;
}

void handle_client(int client_socket) {
  struct GCPHeader header;

  // Mantendo a conexão aberta
  while (true) {
    int bytes_rec = recv(client_socket, &header, sizeof(GCPHeader), 0);

    if (bytes_rec <= 0) {
      lock_guard<mutex> lock(state_mutex);
      sensor_readings.erase(header.device_id);
      cout << "[MANAGER] Conexão com o cliente encerrada\n";
      return;
    }

    if (header.msg_type ==
        static_cast<uint8_t>(MessageType::CLIENT_GET)) { // CLIENT_GET
      process_client_get(client_socket, header);
    } else if (header.msg_type ==
               static_cast<uint8_t>(
                   MessageType::CLIENT_SET_THRESHOLD)) { // CLIENT_SET_THRESHOLD
      process_client_set_threshold(client_socket, header);
    }
  }
}

void evaluate_thresholds(uint8_t sensor_id, float reading) {
  float min_value = 0.0f;
  float max_value = 0.0f;
  bool has_thresholds = false;

  {
    lock_guard<mutex> lock(state_mutex);
    if (sensor_thresholds.find(sensor_id) != sensor_thresholds.end()) {
      min_value = sensor_thresholds[sensor_id].first;
      max_value = sensor_thresholds[sensor_id].second;
      has_thresholds = true;
    }
  }

  if (!has_thresholds)
    return;

  // Lambda Function
  auto send_command = [](uint8_t actuator_id, uint8_t state) {
    int act_socket = -1;

    // Busca o socket do atuador
    {
      lock_guard<mutex> lock(state_mutex);
      if (connected_actuators.find(actuator_id) != connected_actuators.end()) {
        act_socket = connected_actuators[actuator_id];
      }
    }

    // Se o autador estiver conectado, envia SET_ACTUATOR_ACK
    if (act_socket != -1) {
      uint8_t header[4] = {'G', 'C', 0x03, actuator_id};
      send(act_socket, header, 4, 0);
      send(act_socket, &state, sizeof(state), 0);

      cout << "[MANAGER] Comando SET_ACTUATOR enviado ao dispositivo"
           << (int)actuator_id
           << "--> Ação: " << (state == 0x01 ? "LIGAR" : "DESLIGAR") << endl;
    } else {
      cout << "[MANAGER] Aviso: Atuador" << (int)actuator_id << "offline"
           << endl;
    }
  };

  // Lógica do controle de histerese
  if (sensor_id == static_cast<uint8_t>(DeviceType::TEMP_SENSOR)) {
    if (reading < min_value) { // Temperatura
      send_command(static_cast<uint8_t>(DeviceType::HEATER),
                   0x01); // liga aquecedor
      send_command(static_cast<uint8_t>(DeviceType::COOLER),
                   0x00); // desliga resfriador

    } else if (reading > max_value) {
      send_command(static_cast<uint8_t>(DeviceType::COOLER),
                   0x01); // liga resfriador
      send_command(static_cast<uint8_t>(DeviceType::HEATER),
                   0x00); // desliga aqueceor
    } else {
      // desliga ambos
      send_command(static_cast<uint8_t>(DeviceType::HEATER), 0x00);
      send_command(static_cast<uint8_t>(DeviceType::COOLER), 0x00);
    }
  } else if (sensor_id == static_cast<uint8_t>(
                              DeviceType::HUMIDITY_SENSOR)) { // Umidade do solo
    if (reading < min_value) {
      send_command(static_cast<uint8_t>(DeviceType::SPRINKLER),
                   0x01); // liga irrigação
    } else if (reading > max_value) {
      send_command(static_cast<uint8_t>(DeviceType::SPRINKLER),
                   0x00); // desliga irrigação
    }
  }

  else if (sensor_id == static_cast<uint8_t>(DeviceType::CO2_SENSOR)) { // CO2
    if (reading < min_value) {
      send_command(static_cast<uint8_t>(DeviceType::CO2_INJECTOR),
                   0x01); // liga injetor
    } else if (reading > max_value) {
      send_command(static_cast<uint8_t>(DeviceType::CO2_INJECTOR),
                   0x00); // desliga injetor
    }
  }
}

void process_client_get(int client_socket, GCPHeader &header) {
  float current_reading = -10000.0f;

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
  if (current_reading != -10000.0f)
    cout << "[MANAGER] Valor do dispositivo " << header.device_id
         << " enviado\n";
}

void process_client_set_threshold(int client_socket, GCPHeader &header) {
  float min_value = 0.0f;
  float max_value = 0.0f;

  uint8_t received = 0x01;

  // Recebe os dados do payload
  int bytes_rec_min = recv(client_socket, &min_value, sizeof(min_value), 0);
  int bytes_rec_max = recv(client_socket, &max_value, sizeof(max_value), 0);

  if (bytes_rec_max <= 0 || bytes_rec_min <= 0) {
    cerr << "[MANAGER]: Erro no recebimento dos thresholds\n";
    close(client_socket);
    return;
  }

  {
    lock_guard<mutex> lock(state_mutex);
    // Verifica se id existe
    if (sensor_readings.find(header.device_id) == sensor_readings.end()) {
      received = 0x01;
    } else {

      // Muda os valores de tresholds
      sensor_thresholds[header.device_id].first = min_value;
      sensor_thresholds[header.device_id].second = max_value;
      received = 0x00;
    }
  }

  // Envia o CLIENT_SET_THRESHOLD
  // ENVIA O HEADER
  uint8_t threshold_ack[4] = {'G', 'C', 0x08, header.device_id};
  send(client_socket, &threshold_ack, sizeof(threshold_ack), 0);

  // Envia Payload
  send(client_socket, &received, sizeof(received), 0);

  // Debug
  if (received == 0x01)
    cerr << "[MANAGER]: Erro ao configurar o treshold\n";
  else {
    cout << "[MANAGER]: Threshold setados com sucesso\n";
  }

  // Pegar a última leitura salva para configurar os atuadores
  float current_reading = 0.0f;
  {
    lock_guard<mutex> lock(state_mutex);
    if (sensor_readings.find(header.device_id) != sensor_readings.end()) {
      current_reading = sensor_readings[header.device_id];
    }
  }
  evaluate_thresholds(header.device_id, current_reading);

  return;
}

// Recebe o HELLO
bool receive_hello(int client_socket, GCPHeader &header, uint8_t &sensor_type) {

  int bytes_rec = recv(client_socket, &header, sizeof(GCPHeader), 0);

  if (bytes_rec <= 0)
    return false;

  recv(client_socket, &sensor_type, sizeof(sensor_type), 0);

  cout << "[MANAGER] HELLO recebido pelo sensor do tipo" << (int)sensor_type
       << endl;

  return true;
}

// Envia o HELLO_ACK
bool send_hello_ack(int client_socket, GCPHeader header) {

  uint8_t hello_ack[4] = {'G', 'C', 0x01, header.device_id};
  if (send(client_socket, hello_ack, 4, 0) != sizeof(hello_ack)) {
    cerr << "[MANAGER] Erro no envio do ACK";
    close(client_socket);
    return false;
  }

  return true;
}
