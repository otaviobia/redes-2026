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

// --- Variáveis Globais ---
uint8_t my_device_id;
DeviceType my_type;
bool is_on = false; // Estado físico atual

// --- Assinaturas de Funções ---

int connect_to_manager(const std::string& ip, int port);

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

// --- Função Principal ---
int main(int argc, char* argv[]) {
    // 1. Fazer parse dos argumentos (ID do atuador, IP, Porta)
    // 2. connect_to_manager()
    // 3. register_actuator()
    // 4. Loop infinito:
    //    a. recv() aguardando SET_ACTUATOR (0x03)
    //    b. bool success = execute_command(payload)
    //    c. send_ack(socket_fd, success)
    
    std::cout << "[ACTUATOR] Inicializando e aguardando comandos...\n";
    
    return 0;
}