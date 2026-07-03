/**
 * @file actuator.cpp
 * @brief Implementação do cliente de atuação. Conecta-se de forma
 * passiva ao Gerenciador para aguardar e executar comandos de
 * mudança de estado (LIGAR/DESLIGAR).
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
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// --- Variáveis Globais ---
uint8_t my_device_id;
DeviceType my_type;
bool is_on = false; // Estado físico atual

// --- Protótipos ---

/**
 * Recebe ip e porta do servidor.
 * Retorna descritor do socket (sucesso) ou -1 (erro)
 */
int connect_to_manager(const char* ip, int port);

/**
 * Envia a mensagem HELLO (0x00) e aguarda o HELLO_ACK (0x01).
 */
bool register_actuator(int socket_fd);

/**
 * Envia SET_ACTUATOR_ACK (0x04) para o gerenciador.
 */
void send_ack(int socket_fd, bool success);

/**
 * Altera o estado físico (simulado com prints no terminal).
 */
bool execute_command(uint8_t command);

/**
 * Função auxiliar para enviar erros na função connect_to_manager
 */
int socket_error(const char* msg, int sock = -1);

/**
 * Função auxiliar para criar um header GCP
 */
GCPHeader create_header(MessageType type);

// --- Função Principal ---
int main(int argc, char* argv[]) {
    // Inicialização e validação de argumentos
    if (argc != 5) {
        cerr << "Uso: " << argv[0] << " <IP> <PORTA> <DEVICE_ID> <TIPO_DISPOSITIVO>\n";
        cerr << "Tipos validos: 3 (Aquecedor), 4 (Resfriador), 5 (Irrigacao), 6 (Injetor CO2)\n";
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    my_device_id = static_cast<uint8_t>(atoi(argv[3]));

    int tipo_input = atoi(argv[4]);
    if (tipo_input < 3 || tipo_input > 6) {
        cerr << "[ACTUATOR] Tipo invalido para um atuador! Deve ser entre 3 e 6.\n";
        return 1;
    }
    my_type = static_cast<DeviceType>(tipo_input);
    
    // Conexão e handshake
    int sock = connect_to_manager(ip, port);
    if (sock < 0) return 1;

    if (!register_actuator(sock)) {
        close(sock);
        return 1;
    }

    // Loop principal de atuação
    cout << "[ACTUATOR] Inicializado com ID " << (int)my_device_id 
         << " e Tipo " << tipo_input << ". Aguardando comandos...\n";

    while (true) {
        GCPHeader header;
        int bytes_rec = recv(sock, &header, sizeof(header), 0);
        
        if (bytes_rec <= 0) {
            cout << "[ACTUATOR] Gerenciador desconectado. Encerrando.\n";
            break;
        }

        if (header.magic[0] != GCP_MAGIC[0] || header.magic[1] != GCP_MAGIC[1]) {
            cerr << "[ACTUATOR] Pacote malformado recebido. Ignorando lixo de rede.\n";
            continue;
        }

        if (header.msg_type == static_cast<uint8_t>(MessageType::SET_ACTUATOR)) {
            uint8_t command;
            int bytes_cmd = recv(sock, &command, sizeof(command), 0);
            if (bytes_cmd <= 0) {
                cerr << "[ACTUATOR] Erro: Comando nao recebido por completo.\n";
                break;
            }
            bool success = execute_command(command);
            send_ack(sock, success);
        } else {
            cerr << "[ACTUATOR] Mensagem GCP inesperada recebida (" << (int)header.msg_type << "). Ignorando.\n";
        }
    }

    close(sock);
    return 0;
}

// --- Implementação ---

int connect_to_manager(const char* ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return socket_error("[ACTUATOR] Erro ao criar socket.", sock);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
        return socket_error("[ACTUATOR] Endereço IP inválido.", sock);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        return socket_error("[ACTUATOR] Falha ao conectar ao Gerenciador.", sock);
    return sock;
}

bool register_actuator(int socket_fd) {
    GCPHeader header = create_header(MessageType::HELLO);

    if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
        cerr << "[ACTUATOR] Erro ao enviar cabeçalho.\n";
        return false;
    }
    
    uint8_t type_val = static_cast<uint8_t>(my_type);
    if (send(socket_fd, &type_val, sizeof(type_val), 0) != sizeof(type_val)) {
        cerr << "[ACTUATOR] Erro ao enviar payload.\n";
        return false;
    }

    // Aguarda HELLO_ACK
    GCPHeader ack_header;
    int bytes_rec = recv(socket_fd, &ack_header, sizeof(ack_header), 0);
    
    if (bytes_rec <= 0) {
        cerr << "[ACTUATOR] Falha no registro: Nenhuma resposta do servidor.\n";
        return false;
    }

    // Verifica se respondeu com lixo
    if (ack_header.magic[0] != GCP_MAGIC[0] || ack_header.magic[1] != GCP_MAGIC[1]) {
        cerr << "[ACTUATOR] Falha no registro: Servidor respondeu com formato invalido.\n";
        return false;
    }

    if (ack_header.msg_type == static_cast<uint8_t>(MessageType::HELLO_ACK)) {
        cout << "[ACTUATOR] Registrado com sucesso no Gerenciador!\n";
        return true;
    }
    cerr << "[ACTUATOR] Falha no registro: Mensagem inesperada recebida.\n";
    return false;
}

void send_ack(int socket_fd, bool success) {
    GCPHeader header = create_header(MessageType::SET_ACTUATOR_ACK);
    uint8_t status = success ? 0x00 : 0x01;

    if (send(socket_fd, &header, sizeof(header), 0) != sizeof(header)) {
        cerr << "[ACTUATOR] Erro ao enviar cabeçalho.\n";
        return;
    }

    if (send(socket_fd, &status, sizeof(status), 0) != sizeof(status)) {
        cerr << "[ACTUATOR] Erro ao enviar status.\n";
        return;
    }
}

bool execute_command(uint8_t command) {
    // Simula execução de comando (só mensagem no terminal)
    if (command == 0x01) {
        is_on = true;
        cout << ">>> [ACTUATOR] Atuador LIGADO (ON).\n";
        return true;
    } else if (command == 0x00) {
        is_on = false;
        cout << ">>> [ACTUATOR] Atuador DESLIGADO (OFF).\n";
        return true;
    } else {
        cerr << ">>> [ACTUATOR] Comando desconhecido recebido pelo hardware.\n";
        return false;
    }
}

// TO-DO: as funções abaixo poderiam estar em um protocol_utils.cpp

int socket_error(const char* msg, int sock) {
    cerr << msg << '\n';
    if (sock >= 0) close(sock);
    return -1;
}

GCPHeader create_header(MessageType type) {
    GCPHeader h{};
    h.magic[0] = GCP_MAGIC[0];
    h.magic[1] = GCP_MAGIC[1];
    h.msg_type = static_cast<uint8_t>(type);
    h.device_id = my_device_id;
    return h;
}