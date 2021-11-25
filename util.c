#include "util.h"

/* Verifica se existe um processo gestor em execução */
int verifica_processo_gestor(){
  char  string[100];
  int   numero;
  FILE * f;

  f = popen("pgrep gestor | wc -l", "r");
  fgets(string, sizeof(string), f);   /* Obtem o output */
  numero = atoi(string);              /* Converte para int */
  pclose(f);
  return numero;
}

/* Mensagem em caso de erro */
void sair(char *erro){
  perror(erro);
  exit(EXIT_FAILURE);
}

/* Verifica se um processo está em execução */
int verifica_processo(int pid){
  char  string[100], output[10];
  int   numero;
  FILE * f;

  sprintf(string, "ps -a %d | wc -l", pid);

  f = popen(string, "r");
  fgets(output, sizeof(output), f);
  numero = atoi(output);
  pclose(f);
  return numero;
}
