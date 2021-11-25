#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "mensagem.h"

/* Nome do FIFO do servidor */
#define SERVER_FIFO "/tmp/server_fifo"

/* Nome do FIFO cada cliente. %d será substituído pelo PID do cliente */
#define CLIENTE_FIFO "/tmp/resp_%d_fifo"

/* Tamanho máximo de um pedido ou resposta */
#define TAM_MAX_PEDIDO 100
#define TAM_MAX_RESPOSTA 500
#define TAM_FIFO_FNAME  50

/* Estrutura da mensagem correspondente a um pedido cliente->servidor */
typedef struct {
  pid_t   pid_cliente;
  char    palavra[TAM_MAX_PEDIDO];
  char    extra[TAM_MAX_PEDIDO];
  //pnova_mensagem mensagem;
  char    topico[TAM_TOPICO];
  char    titulo[TAM_TITULO];
  char    corpo[TAM_CORPO];
  int     duracao;
} pedido_server;

/* Estrutura da mensagem correspondente a uma resposta servidor->cliente */
typedef struct {
  char  palavra[TAM_MAX_RESPOSTA];
  char  extra[TAM_MAX_RESPOSTA];
} resposta_server;
