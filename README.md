# Estufa Inteligente - GCP (Greenhouse Control Protocol)

Implementação prática de um protocolo de camada de aplicação utilizando Sockets em C++ para o trabalho de Redes de Computadores.

## Requisitos de Sistema
* Sistema Operacional Unix/Linux (ou WSL)
* Compilador G++ (suporte a C++17)
* GNU Make

## Compilação

Na raiz do projeto, execute:
```bash
make
```

Os executáveis serão gerados dentro do diretório bin/.

### Ordem de Execução

Abra múltiplos terminais e siga a ordem abaixo para iniciar o sistema:

1. Iniciar o Gerenciador (Servidor):

```bash
./bin/manager <porta>
```

2. Iniciar os Atuadores:

```bash
./bin/actuator <ip_gerenciador> <porta_gerenciador> <device_id> <tipo>
```

3. Iniciar os Sensores:

```bash
./bin/sensor <ip_gerenciador> <porta_gerenciador> <device_id> <tipo>
```

4. Acessar via Cliente Externo:

```bash
./bin/client <ip_gerenciador> <porta_gerenciador>
```

## Sobre a Implementação

- **Gerenciador:** Utiliza std::thread para processar cada dispositivo conectado simultaneamente.

- **Comunicação:** O envio e recebimento via sockets deve respeitar o empacotamento da struct GCPHeader (4 bytes fixos) seguido pelo payload variável dependendo do msg_type.

## Autores

Caio Capocasali - 12541733

Otávio Biagioni Melo - 15482604