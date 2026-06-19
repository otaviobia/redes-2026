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
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// --- Assinaturas de Funções ---

int connect_to_manager(const std::string& ip, int port);

/**
 * Envia CLIENT_GET (0x05) para um sensor_id e imprime o CLIENT_GET_ACK (0x06).
 */
void request_sensor_data(int socket_fd, uint8_t sensor_id);

/**
 * Envia CLIENT_SET_THRESHOLD (0x07) e verifica o CLIENT_SET_THRESHOLD_ACK (0x08).
 */
void set_sensor_threshold(int socket_fd, uint8_t sensor_id, float min_val, float max_val);

/**
 * Exibe o menu interativo no terminal.
 */
void print_menu();

// --- Função Principal ---
int main(int argc, char* argv[]) {
    // 1. connect_to_manager()
    // 2. Loop de interface com usuário (CLI):
    //    a. print_menu()
    //    b. Ler input do std::cin
    //    c. Chamar request_sensor_data() ou set_sensor_threshold() baseado na escolha
    
    std::cout << "[CLIENT] Bem-vindo ao painel da Estufa Inteligente.\n";
    
    return 0;
}