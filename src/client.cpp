/**
 * @file client.cpp
 * @brief Interface interativa de linha de comando (CLI) para o usuário
 * externo monitorar os sensores e configurar os limites mínimo e máximo
 * (thresholds) da estufa.
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
#include <limits>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

// --- Protótipos ---

/**
 * Recebe ip e porta do servidor.
 * Retorna descritor do socket (sucesso) ou -1 (erro)
 */
int connect_to_manager(const char *ip, int port);

/**
 * Envia CLIENT_GET (0x05) para um sensor_id e imprime o CLIENT_GET_ACK (0x06).
 */
void request_sensor_data(int socket_fd, uint8_t sensor_id);

/**
 * Envia CLIENT_SET_THRESHOLD (0x07) e verifica o CLIENT_SET_THRESHOLD_ACK
 * (0x08).
 */
void set_sensor_threshold(int socket_fd, uint8_t sensor_id, float min_val,
                          float max_val);

/**
 * Exibe o menu interativo no terminal.
 */
void print_menu();

/**
 * Função auxiliar para tratar erros de conexão.
 */
int socket_error(const char *msg, int sock = -1);

/**
 * Função auxiliar para criar um header GCP.
 * Nota: Para o cliente, o device_id no cabeçalho indica o ALVO (Sensor).
 */
GCPHeader create_header(MessageType type, uint8_t target_device_id);

// --- Função Principal ---
int main(int argc, char *argv[]) {
  // Inicialização e Validação de Argumentos
  if (argc != 3) {
    cerr << "Uso: " << argv[0] << " <IP> <PORTA>\n";
    return 1;
  }

  const char *ip = argv[1];
  int port = atoi(argv[2]);

  // Conexão com o Gerenciador
  int sock = connect_to_manager(ip, port);
  if (sock < 0)
    return 1;

  cout << "[CLIENT] Conectado ao Gerenciador da Estufa Inteligente.\n";

  // Loop Principal de Interface
  int choice;
  while (true) {
    print_menu();

    // Proteção contra input malformado
    if (!(cin >> choice)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cerr << "[CLIENT] Entrada invalida. Digite um numero.\n";
      continue;
    }

    if (choice == 1) {
      int id;
      cout << "Digite o ID do Sensor: ";
      cin >> id;
      request_sensor_data(sock, static_cast<uint8_t>(id));
    } else if (choice == 2) {
      int id;
      float min_v, max_v;
      cout << "Digite o ID do Sensor: ";
      cin >> id;
      cout << "Valor Minimo: ";
      cin >> min_v;
      cout << "Valor Maximo: ";
      cin >> max_v;
      set_sensor_threshold(sock, static_cast<uint8_t>(id), min_v, max_v);
    } else if (choice == 3) {
      cout << "[CLIENT] Encerrando conexao com a Estufa...\n";
      break;
    } else {
      cout << "[CLIENT] Opcao invalida.\n";
    }
  }

  close(sock);
  return 0;
}

// --- Implementação ---

int connect_to_manager(const char *ip, int port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return socket_error("[CLIENT] Erro ao criar socket.", sock);

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    return socket_error("[CLIENT] Endereco IP invalido.", sock);

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    return socket_error("[CLIENT] Falha ao conectar ao Gerenciador.", sock);

  return sock;
}

void request_sensor_data(int socket_fd, uint8_t sensor_id) {
  GCPHeader header = create_header(MessageType::CLIENT_GET, sensor_id);

  // Envia cabeçalho (sem payload extra)
  if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
    cerr << "[CLIENT] Erro ao enviar requisicao.\n";
    return;
  }

  // Aguarda resposta (CLIENT_GET_ACK)
  GCPHeader ack_header;
  int bytes_rec = recv(socket_fd, &ack_header, sizeof(ack_header), 0);

  // Conexão caiu?
  if (bytes_rec <= 0) {
    cerr << "[CLIENT] Erro: Gerenciador desconectado ou sem resposta.\n";
    return;
  }

  // Lixo na rede?
  if (ack_header.magic[0] != GCP_MAGIC[0] ||
      ack_header.magic[1] != GCP_MAGIC[1]) {
    cerr << "[CLIENT] Erro: Resposta malformada (Magic Number invalido).\n";
    return;
  }

  // Tudo certo?
  if (ack_header.msg_type ==
      static_cast<uint8_t>(MessageType::CLIENT_GET_ACK)) {
    float reading;
    if (recv(socket_fd, &reading, sizeof(reading), 0) <= 0) {
      cerr << "[CLIENT] Erro ao ler payload da leitura.\n";
      return;
    }
    if (reading == -10000.0f) {
      cout << "Nenhuma leitura disponível (sensor desligado ou id inválido)\n";
    } else {
      cout << ">>> [CLIENT] Leitura do Sensor ID " << (int)sensor_id << ": "
           << reading << "\n";
    }
  } else {
    cerr << "[CLIENT] Erro: Resposta inesperada do Gerenciador (Opcode: "
         << (int)ack_header.msg_type << ").\n";
  }
}

void set_sensor_threshold(int socket_fd, uint8_t sensor_id, float min_val,
                          float max_val) {
  GCPHeader header =
      create_header(MessageType::CLIENT_SET_THRESHOLD, sensor_id);

  if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
    cerr << "[CLIENT] Erro ao enviar cabecalho.\n";
    return;
  }

  // Envia payloads separadamente e os valida
  if (send(socket_fd, &min_val, sizeof(min_val), 0) != sizeof(min_val)) {
    cerr << "[CLIENT] Erro ao enviar Valor Minimo.\n";
    return;
  }
  if (send(socket_fd, &max_val, sizeof(max_val), 0) != sizeof(max_val)) {
    cerr << "[CLIENT] Erro ao enviar Valor Maximo.\n";
    return;
  }

  // Aguarda resposta (CLIENT_SET_THRESHOLD_ACK)
  GCPHeader ack_header;
  int bytes_rec = recv(socket_fd, &ack_header, sizeof(ack_header), 0);

  if (bytes_rec <= 0) {
    cerr << "[CLIENT] Erro: Gerenciador desconectado ou sem resposta.\n";
    return;
  }

  if (ack_header.magic[0] != GCP_MAGIC[0] ||
      ack_header.magic[1] != GCP_MAGIC[1]) {
    cerr << "[CLIENT] Erro: Resposta malformada (Magic Number invalido).\n";
    return;
  }

  if (ack_header.msg_type ==
      static_cast<uint8_t>(MessageType::CLIENT_SET_THRESHOLD_ACK)) {
    uint8_t status;
    if (recv(socket_fd, &status, sizeof(status), 0) <= 0) {
      cerr << "[CLIENT] Erro ao ler payload de status.\n";
      return;
    }

    if (status == 0x00) {
      cout << ">>> [CLIENT] Limites do Sensor ID " << (int)sensor_id
           << " atualizados!\n";
    } else {
      cout << ">>> [CLIENT] Falha ao configurar limites (Valores "
              "invalidos/recusados pelo servidor).\n";
    }
  } else {
    cerr << "[CLIENT] Erro: Mensagem inesperada do Gerenciador (Opcode: "
         << (int)ack_header.msg_type << ").\n";
  }
}

void print_menu() {
  cout << "\n==================================\n";
  cout << "    PAINEL ESTUFA INTELIGENTE\n";
  cout << "==================================\n";
  cout << "1. Obter leitura de um Sensor\n";
  cout << "2. Configurar Limites (Thresholds)\n";
  cout << "3. Sair\n";
  cout << "Escolha uma opcao: ";
}

// TO-DO: as funções abaixo poderiam estar em um protocol_utils.cpp

int socket_error(const char *msg, int sock) {
  cerr << msg << '\n';
  if (sock >= 0)
    close(sock);
  return -1;
}

GCPHeader create_header(MessageType type, uint8_t target_device_id) {
  GCPHeader h{};
  h.magic[0] = GCP_MAGIC[0];
  h.magic[1] = GCP_MAGIC[1];
  h.msg_type = static_cast<uint8_t>(type);
  h.device_id = target_device_id;
  return h;
}
