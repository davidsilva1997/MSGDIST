#include "mensagem.h"
#include "util.h"

/* Criar um tópico */
ptopico cria_topico(ptopico t, char *nome_topico){
  ptopico novo, aux = t;

  /* Alocação de memória para o novo tópico */
  novo = malloc(sizeof(topico));
  if (novo == NULL){
    printf("Erro na alocação de memória para criar um tópico!\n");
    return t;
  }
  /* Verifica se o tópico já existe */
  while (aux != NULL){
    if (!strcasecmp(aux->topico, nome_topico)){
      printf("Tópico já inserido na lista!\n");
      return t;
    }
    aux = aux->prox_topico;
  }
  /* Adiciona o tópico */
  strcpy(novo->topico, nome_topico);
  novo->lista_mensagens = NULL;
  novo->prox_topico = t;
  t = novo;

  return t;
}

/* Verifica se um tópico já existe,
  retorna 1 se sim, 0 se não */
int verifica_topico_existente(ptopico t, char *nome){
  ptopico aux = t;

  /* Verifica se o tópico já existe */
  while (aux != NULL){
    if (!strcasecmp(aux->topico, nome))
      return 1;
    aux = aux->prox_topico;
  }
  return 0;
}

/* Cria mensagem */
void cria_mensagem(ptopico t, pnova_mensagem mensagem){
  ptopico aux = t;
  pmensagem nova;

  while (aux != NULL && strcmp(aux->topico, mensagem->topico) != 0)
    aux = aux->prox_topico;

  /* Caso o tópico existir */
  if (aux != NULL){
    nova = malloc(sizeof(mensagem));
    if (nova == NULL){
      printf("Erro na alocação de memória para criar a mensagem!\n");
      return;
    }

    strcpy(nova->titulo, mensagem->titulo);
    strcpy(nova->corpo, mensagem->corpo);
    nova->duracao = mensagem->duracao;
    nova->prox_mensagem = aux->lista_mensagens;

    aux->lista_mensagens = nova;
    printf("Mensagem adicionada com sucesso\n");
  }
  else {
    printf("Tópico não encontrado!\n");
  }
}

void adiciona_mensagem(ptopico t, pnova_mensagem mensagem){
  ptopico aux = t;
  pmensagem nova;

  if (aux == NULL){
    fprintf(stderr, "Nenhum tópico criado!\n");
    return;
  }

  while (aux != NULL){
    if (!strcasecmp(aux->topico, mensagem->topico))
      break;
    aux = aux->prox_topico;
  }

  if (aux == NULL){
    fprintf(stderr, "Tópico não encontrado!\n");
    return;
  }

  nova = malloc(sizeof(mensagem));
  if (nova == NULL){
    fprintf(stderr, "Erro na alocação de memória para a mensagem!\n");
    return;
  }

  strcpy(nova->titulo, mensagem->titulo);
  strcpy(nova->corpo, mensagem->corpo);
  nova->duracao = mensagem->duracao;
  nova->prox_mensagem = aux->lista_mensagens;

  aux->lista_mensagens = nova;

  fprintf(stderr, "Mensagem adicionada com sucesso!\n");
}
