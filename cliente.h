#ifndef cliente_h
#define cliente_h


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <ncurses.h>
#include "util.h"
#include "server.h"
#include "mensagem.h"

#define NUM_OPCAO 7

typedef struct thread_args thread_args, *pthread_args;

struct thread_args {
  char            username[TAM_USERNAME];
  char            c_fifo_fname[TAM_FIFO_FNAME];
  int             * c_fifo_fd;
  int             * s_fifo_fd;
  pedido_server   * pedido;
  resposta_server * resposta;
};

/* Cria o FIFO do cliente */
void cria_FIFO_cliente(pedido_server *, char c_fifo_fname[TAM_FIFO_FNAME]);

/* Remove o FIFO do cliente */
void remove_FIFO_cliente(char c_fifo_fname[TAM_FIFO_FNAME]);

/* Abre o FIFO do servidor */
void abre_FIFO_servidor(int *, char c_fifo_fname[TAM_FIFO_FNAME]);

/* Abre o FIFO do cliente para read+write */
void abre_FIFO_cliente(int *, int *, char c_fifo_fname[TAM_FIFO_FNAME]);

/* Envia um pedido ao gestor (servidor) */
void envia_pedido(char palavra[TAM_MAX_PEDIDO], char extra[TAM_MAX_PEDIDO], pedido_server *pedido, int *s_fifo_fd);

/* Envia um pedido de nova mensagem */
void envia_pedido_mensagem(char palavra[TAM_MAX_PEDIDO], pnova_mensagem msg, pedido_server *pedido, int *s_fifo_fd);

/* Obtem as respostas do gestor (servidor)*/
void obtem_resposta(resposta_server *resposta, int *c_fifo_fd);

/* Thread para enviar pedidos ao gestor */
void * thread_pedidos(void *);

/* Thread das respostas do gestor */
void * thread_respostas(void *);

/* Thead verifica encerramento do gestor */
void * thread_encerramento_gestor(void *);

/* Preenche os dados da mensagem */
pnova_mensagem preenche_nova_mensagem();

/* Mostra o menu feito com ncurses */
void mostra_menu(char *username);

#endif
