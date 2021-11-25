#ifndef util_h
#define util_h

#include <stdio.h>
#include <stdlib.h>

#define TAM_USERNAME 20   /* Tamanho máximo do username */

/* Verifica se existe um processo gestor em execução */
int verifica_processo_gestor();

/* Verifica se um processo está em execução */
int verifica_processo(int pid);

/* Mensagem em caso de erro */
void sair(char *erro);
#endif
