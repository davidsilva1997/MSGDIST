#ifndef mensagem_h
#define mensagem_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Tamanho máximo de cada campo que constitui uma mensagem */
#define TAM_TOPICO 50
#define TAM_TITULO 50
#define TAM_CORPO  900
#define DURACAO_MSG 500

typedef struct topico topico, *ptopico;
typedef struct mensagem mensagem, *pmensagem;
typedef struct nova_mensagem nova_mensagem, *pnova_mensagem;

/*
(TÓPICO)->(TÓPICO)->(TÓPICO)
    |                   |
  (msg)               (msg)
    |
  (msg)
*/

/* Estrutura do tópico */
struct topico {
  char      topico[TAM_TOPICO];
  pmensagem lista_mensagens;
  ptopico   prox_topico;
};

/* Estrutura da mensagem */
struct mensagem {
  char      titulo[TAM_TITULO];
  char      corpo[TAM_CORPO];
  int       duracao;
  pmensagem prox_mensagem;
};

/* Estrutura da nova mensagem */
struct nova_mensagem {
  char      topico[TAM_TOPICO];
  char      titulo[TAM_TITULO];
  char      corpo[TAM_CORPO];
  int       duracao;
};

/* Criar um tópico */
ptopico cria_topico(ptopico t, char *topico);

/* Verifica se um tópico já existe,
  retorna 1 se sim, 0 se não */
int verifica_topico_existente(ptopico t, char *nome);

/* Cria mensagem */
void cria_mensagem(ptopico t, pnova_mensagem mensagem);

void adiciona_mensagem(ptopico t, pnova_mensagem mensagem);

#endif
