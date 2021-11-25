#ifndef gestor_h
#define gestor_h

#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include "util.h"
#include "server.h"
#include "mensagem.h"

#define WRITE_PIPE 1
#define READ_PIPE  0

typedef struct gestor gestor, *pgestor;
typedef struct cliente cliente, *pcliente;
typedef struct subscricao subscricao, *psubscricao;
typedef struct thread_cliente thread_cliente, *pthread_cliente;

/* Estrutura do gestor */
struct gestor {
  int         pid_gestor;     /* PID do gestor */
  pcliente    lista_clientes; /* Lista ligada de clientes */
};

/* Estrutura dos clientes */
struct cliente {
  int         pid_cliente;
  char        username[TAM_USERNAME];
  psubscricao lista_subs;
  pcliente    prox_cliente;
};

/* Estrutura das subscrições aos tópicos */
struct subscricao {
  char        subscricao[TAM_TOPICO];
  psubscricao prox_sub;
};

/* Trata o sinal SIGINT */
void trata_sinal(int);

/* Trata o sinal SIGKILL */
void trata_sinal_sigkill(int);

/* Cria o gestor */
pgestor cria_gestor();

/* Cria o FIFO do servidor */
void cria_FIFO_servidor();

/* Abre o FIFO do servidor read+write */
void abre_FIFO_servidor(int *);

/* Thread para gerir os pedidos dos clientes */
void * thread_pedidos_clientes();

/* Thread verifica encerramento dos clientes */
void * thread_encerramento_cliente();

/* Thread administrador */
void * thread_administrador();

/* thread que atualiza a duracao das mensagens */
void * thread_mensagens();

/* Thread que verifica se uma mensagem expirou */
void * thread_mensagens_expiradas();

/* Adiciona um cliente */
int adiciona_cliente(pgestor g, int pid_cliente, char username[TAM_USERNAME]);

/* Remove um cliente (logout) */
pcliente logout_cliente(int pid_cliente);

/* Remove todas as subscrições do cliente */
void remove_todas_subs(int pid_cliente);

/* Preenche os dados do cliente */
void preenche_dados_cliente(pcliente c, int pid, char username[TAM_USERNAME], pgestor);

/* Subscrever a determinado tópico */
int subscreve_topico(pgestor g, ptopico t, int pid_cliente, char topico[TAM_TOPICO]);

/* Cancela subscrição */
int cancela_sub(pgestor g, int pid_cliente, char *topico);

/* Avisa um cliente que algo occureu */
void avisa_cliente(int pid_cliente, char *aviso);

/* Avisa os clientes que existe uma nova mensagem */
void avisa_clientes_nova_mensagem(pgestor g, ptopico t, char *topico, char *aviso);

/* Obtem o username atribuido */
const char * get_username_atribuido(pgestor g, char *str);

/* Obtem a lista dos tópicos */
const char * get_lista_topicos(ptopico t, char *str);

/* Obtem a lista de títulos de determinado tópico */
const char * get_lista_titulos(ptopico t, char *topico, char *str);

/* Obtem a mensagem de determinado tópico */
const char * get_mensagem_topico(ptopico t, char *topico, char *str);

/* Obtem a lista de subscrições do cliente */
const char * get_subs(pgestor g, int pid_cliente, char *str);

/* Ativa o filtro de palavras */
void ativa_filtro();

/* Desativa o filtro de palavras */
void desativa_filtro();

/* Lista os clientes */
void lista_clientes(pgestor g);

/* Lista os tópicos */
void lista_topicos(ptopico t);

/* Lista os tópicos todos e as mensagens contidas */
void lista_topicos_mensagens(ptopico t);

/* Lista as mensagens de determinado tópico */
void lista_mensagens_topico(ptopico t, char *topico);

/* Elimina os tópicos sem mensagens */
ptopico elimina_topicos_sem_mensagens(ptopico t, pgestor g);

/* Shudown */
void shutdown(pgestor g, ptopico t);

/* Exclui um utilizador (comando KICK username) */
int exclui_utilizador(char *username);

void remove_mensagem(ptopico t, pmensagem m);

/* Envia um sinal ao cliente */
void envia_sinal(pcliente cliente, int signal);

/* Obtem o valor duma variável de ambiente */
const char * obtem_valor_variavel_ambiente(char *var, char *str);

/* Obtem uma mensagem */
const char * get_mensagem(ptopico t, char *titulo, char *str);

/* Remove uma mensagem pelo seu título */
void delete_mensagem(ptopico t, char *titulo);

/* Verifica o conteúdo de uma mensagem */
pid_t verifica_mensagem(int * ppg, int * ppv);

#endif
