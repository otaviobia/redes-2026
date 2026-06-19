/**
 * @file protocol.hpp
 * @brief Definições do Greenhouse Control Protocol (GCP), contendo 
 * as estruturas do cabeçalho, constantes (Magic Number) e enumerações
 * de dispositivos e mensagens.
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
#pragma once

#include <cstdint>
#include <string>

// Magic Number do GCP
const char GCP_MAGIC[2] = {'G', 'C'};

// Tipos de Dispositivos (Sensores e Atuadores)
enum class DeviceType : uint8_t {
    TEMP_SENSOR      = 0x00,
    HUMIDITY_SENSOR  = 0x01,
    CO2_SENSOR       = 0x02,
    HEATER           = 0x03,
    COOLER           = 0x04,
    SPRINKLER        = 0x05,
    CO2_INJECTOR     = 0x06
};

// Tipos de Mensagens do GCP
enum class MessageType : uint8_t {
    HELLO                      = 0x00,
    HELLO_ACK                  = 0x01,
    DATA_REPORT                = 0x02,
    SET_ACTUATOR               = 0x03,
    SET_ACTUATOR_ACK           = 0x04,
    CLIENT_GET                 = 0x05,
    CLIENT_GET_ACK             = 0x06,
    CLIENT_SET_THRESHOLD       = 0x07,
    CLIENT_SET_THRESHOLD_ACK   = 0x08
};

// Estrutura do Cabeçalho GCP (4 bytes)
#pragma pack(push, 1) // evita que o compilador adicione padding
struct GCPHeader {
    char magic[2];
    uint8_t msg_type;
    uint8_t device_id;
};
#pragma pack(pop)