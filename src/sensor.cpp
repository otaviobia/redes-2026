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
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

// --- Variáveis Globais ---
uint8_t my_device_id;
DeviceType my_type;

// --- Assinaturas de Funções ---

/**
 * Estabelece conexão TCP com o Gerenciador.
 * Retorna o file descriptor do socket.
 */
int connect_to_manager(const std::string& ip, int port);

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
int main(int argc, char* argv[]) {
    // 1. Fazer parse dos argumentos (ID do sensor, IP, Porta do manager)
    // 2. connect_to_manager()
    // 3. register_sensor()
    // 4. Loop infinito (enviando a cada 1 segundo):
    //    a. float reading = generate_mock_reading()
    //    b. send_data_report(socket_fd, reading)
    //    c. std::this_thread::sleep_for(std::chrono::seconds(1))
    
    std::cout << "[SENSOR] Inicializando...\n";
    
    return 0;
}