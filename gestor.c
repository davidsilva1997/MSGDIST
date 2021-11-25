#include "gestor.h"

/* Descritores de ficheiros (pipes) */
int s_fifo_fd, c_fifo_fd;
int filtro_palavras = 0; /* 0 - desativado 1 - ativado */
int maxnot;         /* Número de palavras proibidas autorizadas numa mensagem */
char wordsnot[128]; /* Ficheiro de palavras proibidas */
int num_mensagens = 0, maxmsg;
pid_t pid_programa;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pgestor gestor_unico  = NULL;
static ptopico topicos = NULL;


int main(int argc, char **argv){
  pthread_t       threadPedidosClientes, threadEncerramentoCliente, threadAdministrador, threadMensagens, threadMensagensExpiradas;

  setbuf(stdout, NULL);

  /* Trata o sinal SIGINT */
  if (signal(SIGINT, trata_sinal) == SIG_ERR)
    sair("Não foi possível configurar o sinal SIGINT!\n");

  signal(SIGKILL, trata_sinal_sigkill);

  /* Verifica o número de argumentos passados */
  if (argc != 1)
    sair("Número de argumentos inválido!\n");

  /* Verifica se existe outro processo gestor em execução */
  if (verifica_processo_gestor() > 1)
    sair("Outro processo gestor em execução!\n");

  /* Obtem o número máximo de palavras proíbidas autorizadas */
  const char * num_palavras_proibidas = getenv("MAXNOT");
  if (num_palavras_proibidas == NULL)
    sair("Não foi possível obter o valor da variável de ambiente [MAXNOT]\n");
  else
    maxnot = atoi(num_palavras_proibidas);    /* isdigit ? */


  /* Obtem o nome do ficheiro das palavras proibidas */
  const char * words_not = getenv("WORDSNOT");
  if (words_not == NULL)
    sair("Não foi possível obter o valor da variável de ambiente [WORDSNOT]\n");
  else
    strcpy(wordsnot, words_not);

  /* Obtem o número máximo de mensagens */
  const char * max_mensagens = getenv("MAXMSG");
  if (max_mensagens == NULL)
    sair("Não foi possível obter o valor da variável de ambiente [MAXMSG]\n");
  else {
    maxmsg = atoi(max_mensagens);
    /*if (!isdigit(maxmsg))
      sair("O valor da variável de ambiente [MAXMSG] não é um número\n");*/
  }

  /* Cria o gestor */
  gestor_unico = cria_gestor();

  /* Cria o FIFO do servidor */
  cria_FIFO_servidor();
  /* Abre o FIFO do servidor */
  abre_FIFO_servidor(&s_fifo_fd);

  /*topicos = cria_topico(topicos, "arte");
  //vetor_topicos = cria_topico(vetor_topicos, "arte");
  topicos = cria_topico(topicos, "desporto");
  topicos = cria_topico(topicos, "moda");
  topicos = cria_topico(topicos, "jogos");

  pnova_mensagem msg;
  msg = malloc(sizeof(nova_mensagem));
  strcpy(msg->topico, "desporto");
  strcpy(msg->titulo, "Bola de Ouro");
  strcpy(msg->corpo, "Cristiano Ronaldo não venceu a bola de ouro este ano...");
  msg->duracao = 800;

  //cria_mensagem(topicos, msg);
  adiciona_mensagem(topicos, msg);

  pnova_mensagem msg2;
  msg2 = malloc(sizeof(nova_mensagem));
  strcpy(msg2->topico, "moda");
  strcpy(msg2->titulo, "Gucci");
  strcpy(msg2->corpo, "Novo casaco exclusivo ...");
  msg2->duracao = 600;
  //cria_mensagem(topicos, msg2);
  adiciona_mensagem(topicos, msg2);

  pnova_mensagem msg3;
  msg3 = malloc(sizeof(nova_mensagem));
  strcpy(msg3->topico, "moda");
  strcpy(msg3->titulo, "Louis Vuitton");
  strcpy(msg3->corpo, "Novas calças exclusivas ...");
  msg3->duracao = 900;

  //cria_mensagem(topicos, msg3);
  adiciona_mensagem(topicos, msg3);*/


  /* Criação do thread de pedidos dos clientes */
  if (pthread_create(&threadPedidosClientes, NULL, thread_pedidos_clientes, NULL) == -1)
    sair("Erro na criação do thread dos pedidos dos clientes!");

  /* Criação do thread que verifica se o cliente encerrou o processo dele sem avisar */
  if (pthread_create(&threadEncerramentoCliente, NULL, thread_encerramento_cliente, NULL) == -1)
    sair("Erro na criação do thread do encerramento dos clientes!");

  /* Criação do thread dos comandos do administrador */
  if (pthread_create(&threadAdministrador, NULL, thread_administrador, NULL) == -1)
    sair("Erro na criação do thread dos comandos de administrador!");

  /* Criação do thread que atualiza a duração das mensagens */
  if (pthread_create(&threadMensagens, NULL, thread_mensagens, NULL) == -1)
    sair("Erro na criação do thread que atualiza a duração das mensagens!");

  /* Criação do thread que remove as mensagens expiradas */
  if (pthread_create(&threadMensagensExpiradas, NULL, thread_mensagens_expiradas, NULL) == -1)
    sair("Erro na criação do thread que verifica se as mensagens expiraram!");


  /* Espera pelo fim do thread */
  if (pthread_join(threadPedidosClientes, NULL))
    sair("Erro na espera do fim do thread dos pedidos dos clientes!");
  if (pthread_join(threadEncerramentoCliente, NULL))
    sair("Erro na espera do fim do thread do encerramento dos clientes!");
  if (pthread_join(threadAdministrador, NULL))
    sair("Erro na espera do fim do thread dos comandos de administrador!");
  if (pthread_join(threadMensagens, NULL))
    sair("Erro na espera do fim do thread que atualiza a duração das mensagens!");
  if (pthread_join(threadMensagensExpiradas, NULL))
    sair("Erro na espera do fim do thread que verifica se as mensagens expiraram!");


  return 0;
}

/* Trata o sinal SIGINT */
void trata_sinal(int i){
  fprintf(stderr, "O gestor vai terminar (interrompido via SIGINT)!\n");
  close(s_fifo_fd);
  unlink(SERVER_FIFO);
  exit(EXIT_SUCCESS);
}

/* Trata o sinal SIGKILL */
void trata_sinal_sigkill(int i){
  fprintf(stderr, "O gestor vai terminar (interrompido via SHUTDOWN)!\n");
  close(s_fifo_fd);
  unlink(SERVER_FIFO);
  exit(EXIT_SUCCESS);
}

/* Cria o gestor */
pgestor cria_gestor(){
  pgestor novo;
  novo = malloc(sizeof(gestor));
  if (novo == NULL)
    sair("Falha na alocação de mem´ria para criar o gestor!\n");
  novo->pid_gestor = getpid();
  novo->lista_clientes = NULL;
  return novo;
}

/* Cria o FIFO do servidor */
void cria_FIFO_servidor(){
  if ((mkfifo(SERVER_FIFO, 0777)) == -1) /* 0777 - rwxrwxrwx */
    sair("Erro na criação do FIFO do servidor!\n");
}

/* Abre o FIFO do servidor read+write */
void abre_FIFO_servidor(int *s_fifo_fd){
  *s_fifo_fd = open(SERVER_FIFO, O_RDWR);
  if (*s_fifo_fd == -1)
    sair("Erro ao abrir o FIFO do servidor (RDWR/Blocking)!\n");
}

/* Thread para gerir os pedidos dos clientes */
void * thread_pedidos_clientes(){
  pedido_server pedido;
  resposta_server resposta;
  char  cliente_fifo_fname[TAM_FIFO_FNAME];

  /* Pedidos dos clientes */
  while (1){
    /* Obtem o pedido */
    if ((read(s_fifo_fd, &pedido, sizeof(pedido))) < sizeof(pedido)){
      continue; /* Não responde ao cliente */
    }

    fprintf(stderr, "\ncliente [%d] efetuou o pedido [%s] [%s]\n\n==>", pedido.pid_cliente, pedido.palavra, pedido.extra);

    strcpy(resposta.palavra, "Comando inválido!");
    strcpy(resposta.extra, "");

    if (!strcasecmp(pedido.palavra, "login")){
      pthread_mutex_lock(&mutex);
      char username[TAM_USERNAME];
      if (adiciona_cliente(gestor_unico, pedido.pid_cliente, pedido.extra) == 0){
        strcpy(resposta.palavra, "Bem vindo ao MSGDIST ");
        strcpy(resposta.extra, get_username_atribuido(gestor_unico, username));
      }
      else {
        strcpy(resposta.palavra, "Falha no login!\n");
        strcpy(resposta.extra, "");
      }
      memset(username, '\0', TAM_USERNAME);
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "logout")){
      pthread_mutex_lock(&mutex);
      gestor_unico->lista_clientes = logout_cliente(pedido.pid_cliente);
      strcpy(resposta.palavra, "Logout efetuado com sucesso!");
      strcpy(resposta.extra, "Até breve");
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "topics")){
      pthread_mutex_lock(&mutex);
      char lista_topicos[TAM_MAX_RESPOSTA];
      strcpy(resposta.palavra, "Listagem de tópicos");
      strcpy(resposta.extra, get_lista_topicos(topicos, lista_topicos));
      memset(lista_topicos, '\0', TAM_MAX_RESPOSTA);
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "titles")){
      pthread_mutex_lock(&mutex);
      char titles[TAM_MAX_RESPOSTA];
      char msg[TAM_MAX_RESPOSTA];
      strcpy(msg, "Listagem dos títulos do tópico ");
      strcat(msg, pedido.extra);

      strcpy(resposta.palavra, msg);
      strcpy(resposta.extra, get_lista_titulos(topicos, pedido.extra, titles));
      memset(titles, '\0', TAM_MAX_RESPOSTA);
      memset(msg, '\0', TAM_MAX_RESPOSTA);
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "msg_of_topic")){
      pthread_mutex_lock(&mutex);
      char msg[TAM_MAX_RESPOSTA];
      strcpy(resposta.palavra, "Mensagem");
      strcpy(resposta.extra, get_mensagem_topico(topicos, pedido.extra, msg));
      memset(msg, '\0', TAM_MAX_RESPOSTA);
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "sub")){
      strcpy(resposta.palavra, "Pedido de subscrição");
      pthread_mutex_lock(&mutex);
      int sub = subscreve_topico(gestor_unico, topicos, pedido.pid_cliente, pedido.extra);
      pthread_mutex_unlock(&mutex);
      switch(sub){
        case -1:
          strcpy(resposta.extra, "Nenhum tópico existente!"); break;
        case -2:
          strcpy(resposta.extra, "Tópico inexistente!"); break;
        case -3:
          strcpy(resposta.extra, "Tópico já subscrito!"); break;
        case -4:
          strcpy(resposta.extra, "Erro...!"); break;
        case 1:
          strcpy(resposta.extra, "Subscrição efetuada com sucesso!"); break;
      }
    }
    else if (!strcasecmp(pedido.palavra, "unsub")){
      pthread_mutex_lock(&mutex);
      strcpy(resposta.palavra, "Pedido de cancelamento de subscrição");
      int unsub = cancela_sub(gestor_unico, pedido.pid_cliente, pedido.extra);
      pthread_mutex_unlock(&mutex);
      switch(unsub){
        case -1:
          strcpy(resposta.extra, "Cliente não encontrado!"); break;
        case -2:
          strcpy(resposta.extra, "Tópico inexistente ou não subscrito!"); break;
        case 1:
          strcpy(resposta.extra, "Cancelamento efetuado com sucesso!"); break;
      }
    }
    else if (!strcasecmp(pedido.palavra, "show_subs")){
      pthread_mutex_lock(&mutex);
      char subs[TAM_MAX_RESPOSTA];
      strcpy(resposta.palavra, "Tópicos subscritos");
      strcpy(resposta.extra, get_subs(gestor_unico, pedido.pid_cliente, subs));
      memset(subs, '\0', TAM_MAX_RESPOSTA);
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "show")){
      pthread_mutex_lock(&mutex);
      char mensagem[TAM_MAX_RESPOSTA];
      strcpy(resposta.palavra, "Mensagem");
      strcpy(resposta.extra, get_mensagem(topicos, pedido.extra, mensagem));
      memset(mensagem, '\0', TAM_MAX_RESPOSTA);
      pthread_mutex_unlock(&mutex);
    }
    else if (!strcasecmp(pedido.palavra, "msg")){
      pnova_mensagem msg = malloc(sizeof(nova_mensagem));
      strcpy(msg->topico, pedido.topico);
      strcpy(msg->titulo, pedido.titulo);
      strcpy(msg->corpo, pedido.corpo);
      msg->duracao = pedido.duracao;

      if (filtro_palavras == 0 && num_mensagens < maxmsg){
        if (verifica_topico_existente(topicos, msg->topico) == 0)
          topicos = cria_topico(topicos, msg->topico);
        adiciona_mensagem(topicos, msg);
        char aviso[200];
        sprintf(aviso, "Nova mensagem [%s] do tópico [%s] disponível durante [%d]", msg->titulo, msg->topico, msg->duracao);
        avisa_clientes_nova_mensagem(gestor_unico, topicos, msg->topico, aviso);
        strcpy(resposta.palavra, "Nova mensagem");
        strcpy(resposta.extra, "Mensagem adicionada com sucesso!");
        num_mensagens++;
        fprintf(stderr, "Armazenamento de mensagens [%d/%d]\n", num_mensagens, maxmsg);
      }
      else if (filtro_palavras == 1 && num_mensagens < maxmsg){
        int inputfd, outputfd, num_palavras_proibidas;
        char output[10];

        if (verifica_mensagem(&inputfd, &outputfd) <= 0)
          printf("Erro ao executar o verificador");

        memset(output, 0x0, sizeof(output));

        /* Escreve para o stdin do programa verificador */
        write(inputfd, msg->corpo, sizeof(msg->corpo));
        write(inputfd, "##MSGEND##\n", 11);
        close(inputfd);

        /* Le do stdout */
        read(outputfd, output, 10);
        strtok(output, "\n");
        num_palavras_proibidas = atoi(output);

        printf("\nPalavras proibidas [%d/%d]\n", num_palavras_proibidas, maxnot);

        if (num_palavras_proibidas > maxnot){
          strcpy(resposta.palavra, "Nova mensagem");
          strcpy(resposta.extra, "Mensagem não adicionada porque o ultrapassa o número de palavras proibidas!");
        }
        else {
          if (verifica_topico_existente(topicos, msg->topico) == 0)
            topicos = cria_topico(topicos, msg->topico);
          adiciona_mensagem(topicos, msg);
          char aviso[200];
          sprintf(aviso, "Nova mensagem [%s] do tópico [%s] disponível durante [%d]", msg->titulo, msg->topico, msg->duracao);
          avisa_clientes_nova_mensagem(gestor_unico, topicos, msg->topico, aviso);
          strcpy(resposta.palavra, "Nova mensagem");
          strcpy(resposta.extra, "Mensagem adicionada com sucesso!");
          num_mensagens++;
          fprintf(stderr, "Armazenamento de mensagens [%d/%d]\n", num_mensagens, maxmsg);
        }
      }
      else {
        strcpy(resposta.palavra, "Nova mensagem");
        strcpy(resposta.extra, "Mensagem não adicionada porque o sistema atingiu o limite de mensagens!");
      }

      free(msg);
    }

    /* Obtem o filename do FIFO para a resposta */
    sprintf(cliente_fifo_fname, CLIENTE_FIFO, pedido.pid_cliente);
    /* Abre o FIFO do cliente para write */
    c_fifo_fd = open(cliente_fifo_fname, O_WRONLY);
    if (c_fifo_fd == -1)
      perror("Erro ao abrir o FIFO cliente, ninguém quis a resposta!\n");
    else {
      /* Envia a resposta */
      if((write(c_fifo_fd, &resposta, sizeof(resposta))) != sizeof(resposta))
        perror("Falha na escrita da resposta!\n");
      /* Fecha o FIFO do cliente */
      close(c_fifo_fd);
    }
  }

  /* Remove o FIFO do servidor */
  close(s_fifo_fd);
  unlink(SERVER_FIFO);
  pthread_exit(NULL);
}

/* Thread verifica encerramento dos clientes */
void * thread_encerramento_cliente(){
  pcliente aux;

  while (1){
    sleep(10);
    aux = gestor_unico->lista_clientes;
    if (aux == NULL){
      //printf("Nenhum utilizador\n");
      continue;
    }
    else {
      while (aux != NULL){
        if (verifica_processo(aux->pid_cliente) != 2){
          printf("O cliente [%d] encerrou!\n", aux->pid_cliente);
          gestor_unico->lista_clientes = logout_cliente(aux->pid_cliente);
        }
        aux = aux->prox_cliente;
      }
    }
  }
  pthread_exit(NULL);
}

/* thread que atualiza a duracao das mensagens */
void * thread_mensagens(){
  ptopico t;
  pmensagem aux;
  while (1){
    t = topicos;
    sleep(10);
    while (t != NULL){
      aux = t->lista_mensagens;
      while (aux != NULL){
        if (aux->duracao > 0)
          aux->duracao -= 10;
        aux = aux->prox_mensagem;
      }
      t = t->prox_topico;
    }
  }
}

/* Thread que verifica se uma mensagem expirou */
void * thread_mensagens_expiradas(){
  ptopico t;
  pmensagem atual, anterior;
  while (1){
    t = topicos;
    sleep(10);
    while (t != NULL){
      atual = t->lista_mensagens;
      while(atual != NULL){
        if (atual->duracao <= 0)
          remove_mensagem(t, atual);
        atual = atual->prox_mensagem;
      }
      t = t->prox_topico;
    }

  }
  return t;
}

/* Remove uma mensagem */
void remove_mensagem(ptopico t, pmensagem m){
  ptopico aux_t;
  pmensagem atual, anterior = NULL;

  aux_t = topicos;

  while (aux_t != NULL && aux_t != t)
    aux_t = aux_t->prox_topico;

  atual = aux_t->lista_mensagens;
  /* Primeiro nó */
  if (atual != NULL && atual == m){
    aux_t->lista_mensagens = atual->prox_mensagem;
    fprintf(stderr, "Mensagem [%s] expirou!\n", atual->titulo);
    num_mensagens--;
    free(atual);
    return;
  }

  while (atual != NULL && atual != m){
    anterior = atual;
    atual = atual->prox_mensagem;
  }

  if (atual == NULL)
    return;

  anterior->prox_mensagem = atual->prox_mensagem;
  fprintf(stderr, "Mensagem [%s] expirou!\n", atual->titulo);
  free(atual);

}

/* Adiciona um cliente */
int adiciona_cliente(pgestor g, int pid_cliente, char username[TAM_USERNAME]){
  pcliente  novo, aux;
  char      i_string[5], temp[TAM_USERNAME];
  int       i = 1;

  if (g == NULL)    // Nenhum gestor criado
    return -1;

  novo = malloc(sizeof(cliente));
  if (novo == NULL)
    return -2;      // Falha na alocação de memória

  /* Verifica se já existe um cliente com o mesmo username */
  aux = g->lista_clientes;
  if (aux == NULL){
    preenche_dados_cliente(novo, pid_cliente, username, g);
    return 0;
  }

  while (aux != NULL && strcmp(aux->username, username) != 0)
    aux = aux->prox_cliente;

    if (aux == NULL){
      preenche_dados_cliente(novo, pid_cliente, username, g);
      return 0;
    }
    else {
      while (1){
        strcpy(temp, username);
        sprintf(i_string, "%d", i);   /* Converte para string */
        strcat(temp, i_string);
        aux = g->lista_clientes;

        while (aux != NULL && strcmp(aux->username, temp) != 0)
          aux = aux->prox_cliente;

        if (aux == NULL){
          preenche_dados_cliente(novo, pid_cliente, temp, g);
          return 0;
        }
        i++;
        strcpy(i_string, "");
      } // while
    }
}

/* Remove um cliente (logout) e as suas subscrições */
pcliente logout_cliente(int pid_cliente){
  pcliente c = gestor_unico->lista_clientes;
  pcliente atual = c, anterior = NULL;

  /* Remove as subs do cliente */
  remove_todas_subs(pid_cliente);

  /* Se encontrar no primeiro nó */
  while (atual != NULL && atual->pid_cliente == pid_cliente){
    c = atual->prox_cliente;
    free(atual);
    return c;
  }

  /* Remove sem ser o primeiro elemento */
  while (atual != NULL){
    /* Procura o cliente a remover */
    while (atual != NULL && atual->pid_cliente != pid_cliente){
      anterior = atual;
      atual = atual->prox_cliente;
    }
    /* Se nao encontrar */
    if (atual == NULL)
      return c;

    anterior->prox_cliente = atual->prox_cliente;
    free(atual);
    atual = anterior->prox_cliente;
  }
  return c;
}

/* Remove todas as subscrições do cliente */
void remove_todas_subs(int pid_cliente){
  pcliente aux = gestor_unico->lista_clientes;
  psubscricao atual, proximo = NULL;

  while (aux != NULL){
    if (aux->pid_cliente == pid_cliente)
      break;
    aux = aux->prox_cliente;
  }

  atual = aux->lista_subs;
  if (atual == NULL)
    return;

  while (atual != NULL){
    proximo = atual->prox_sub;
    //printf("Cancela [%s]\n", atual->subscricao);
    free(atual);
    atual = proximo;
  }

  aux->lista_subs = NULL;

}

/* Preenche os dados do cliente */
void preenche_dados_cliente(pcliente novo, int pid, char username[TAM_USERNAME], pgestor g){
  novo->pid_cliente = pid;
  strcpy(novo->username, username);
  novo->lista_subs = NULL;
  novo->prox_cliente = g->lista_clientes;
  g->lista_clientes = novo;
}

/* Subscrever a determinado tópico */
int subscreve_topico(pgestor g, ptopico t, int pid_cliente, char topico[TAM_TOPICO]){
  pcliente    aux;
  ptopico     aux_t;
  psubscricao nova, aux_s;

  if (g == NULL)
    sair("Nenhum gestor criado!\n");

  nova = malloc(sizeof(subscricao));
  if (nova == NULL){
    printf("Falha na alocação de memória para efetuar a subscrição!\n");
    return -1;
  }

  /* Verifica se o cliente existe */
  aux = g->lista_clientes;
  while (aux != NULL && (aux->pid_cliente != pid_cliente))
    aux = aux->prox_cliente;

  if (t == NULL)
    return -1;

  /* Verifica se o tópico existe */
  aux_t = t;
  while (aux_t != NULL){
    if (!strcasecmp(aux_t->topico, topico))
      break;
    aux_t = aux_t->prox_topico;
  }

  if (aux_t == NULL)
    return -2;
  else {
    /* Verifica se já está subscrito a esse tópico */
    aux_s = aux->lista_subs;
    while (aux_s != NULL){
      if (!strcasecmp(aux_s->subscricao, topico))
        return -3;
      aux_s = aux_s->prox_sub;
    }

    //aux_s = aux->lista_subs;
    if (aux_s == NULL){
      /* Adiciona a subscrição */
      strcpy(nova->subscricao, topico);
      nova->prox_sub = aux->lista_subs;
      aux->lista_subs = nova;
      return 1;
    }
    else
      return -4;
  }
}

/* Cancela subscrição */
int cancela_sub(pgestor g, int pid_cliente, char *topico){
  pcliente aux = g->lista_clientes;
  psubscricao atual, anterior = NULL;

  /* Procura o cliente */
  while (aux != NULL && aux->pid_cliente != pid_cliente)
    aux = aux->prox_cliente;

  if (aux == NULL)
    return -1;

  atual = aux->lista_subs; // Ptr p início da lista de subs

  if (atual == NULL)
    return -2;

  /* Se for o primeiro nó */
  if (!strcasecmp(atual->subscricao, topico)){
    aux->lista_subs = atual->prox_sub;
    free(atual);
    atual = aux->lista_subs;
    return 1;
  }

  /* Sem ser no início */
  while (atual != NULL){
    /* Procura o nó */
    if (!strcasecmp(atual->subscricao, topico))
      break;
    anterior = atual;
    atual = atual->prox_sub;
  }

  if (atual == NULL)
    return -2;
  else {
    anterior->prox_sub = atual->prox_sub;
    free(atual);
    atual = anterior->prox_sub;
    return 1;
  }
}

/* Avisa um cliente que algo occureu */
void avisa_cliente(int pid_cliente, char *aviso){
  resposta_server resposta;
  char  cliente_fifo_fname[TAM_FIFO_FNAME];

  strcpy(resposta.palavra, "Aviso");
  strcpy(resposta.extra, aviso);
  /* Obtem o filename do FIFO para a resposta */
  sprintf(cliente_fifo_fname, CLIENTE_FIFO, pid_cliente);
  /* Abre o FIFO do cliente para write */
  c_fifo_fd = open(cliente_fifo_fname, O_WRONLY);
  if (c_fifo_fd == -1)
    perror("Erro ao abrir o FIFO cliente, ninguém quis a resposta!\n");
  else {
    /* Envia a resposta */
    if((write(c_fifo_fd, &resposta, sizeof(resposta))) != sizeof(resposta))
      perror("Falha na escrita da resposta!\n");
    /* Fecha o FIFO do cliente */
    close(c_fifo_fd);
  }
}

/* Avisa os clientes que existe uma nova mensagem */
void avisa_clientes_nova_mensagem(pgestor g, ptopico t, char *topico, char *aviso){
  pcliente aux;
  psubscricao sub;

  aux = g->lista_clientes;

  while (aux != NULL){
    sub = aux->lista_subs;
    while (sub != NULL){
      if (!strcasecmp(sub->subscricao, topico)){
        avisa_cliente(aux->pid_cliente, aviso);
      }
      sub = sub->prox_sub;
    }
    aux = aux->prox_cliente;
  }
}

/* Obtem o username atribuido */
const char * get_username_atribuido(pgestor g, char *str){
  pcliente aux;
  aux = g->lista_clientes;
  strcpy(str, aux->username);
  return str;
}

/* Obtem a lista dos tópicos */
const char * get_lista_topicos(ptopico t, char *str){
  ptopico aux = t;
  if (aux == NULL)
    strcpy(str, "Nenhum tópico encontrado!");
  else{
    while (aux != NULL){
      strcat(str, aux->topico);
      if (aux->prox_topico != NULL)
        strcat(str, " - ");
      aux = aux->prox_topico;
    }
  }
  return str;
}

/* Obtem a lista de títulos de determinado tópico */
const char * get_lista_titulos(ptopico t, char *topico, char *str){
  pmensagem aux;
  if (t == NULL){
    strcpy(str, "Nenhum título encontrado!");
    return str;
  }

  while(t != NULL){
    if (!strcasecmp(t->topico, topico)){
      aux = t->lista_mensagens;
      if (aux == NULL){
        strcpy(str, "Nenhum título encontrado!");
        return str;
      }
      while (aux != NULL){
        strcat(str, aux->titulo);
        if (aux->prox_mensagem != NULL)
          strcat(str, " - ");
        aux = aux->prox_mensagem;
      }
      return str;
    }
    else {
      if (t->prox_topico == NULL)
        strcpy(str, "Tópico inexistente!");
      t = t->prox_topico;
    }
  }
  return str;
}

/* Obtem a mensagem de determinado tópico */
const char * get_mensagem_topico(ptopico t, char *topico, char *str){
  pmensagem aux;

  if (t == NULL){
    strcpy(str, "Nenhum tópico encontrado!");
    return str;
  }

  while (t != NULL){
    if (!strcasecmp(t->topico, topico)){
      aux = t->lista_mensagens;
      if (aux == NULL){
        strcpy(str, "Nenhuma mensagem encontrada!");
        return str;
      }
      else {
        strcat(str, "Título: "); strcat(str, aux->titulo); strcat(str, "\n");
        strcat(str, "Corpo:  "); strcat(str, aux->corpo); strcat(str, "\n");
      }
      return str;
    }
    else {
      if (t->prox_topico == NULL)
        strcpy(str, "Tópico inexistente!");
      t = t->prox_topico;
    }
  }
  return str;
}

/* Obtem a lista de subscrições do cliente */
const char * get_subs(pgestor g, int pid_cliente, char *str){
  pcliente aux = g->lista_clientes;
  psubscricao aux_s;

  while (aux != NULL && aux->pid_cliente != pid_cliente)
    aux = aux->prox_cliente;

  aux_s = aux->lista_subs;
  if (aux_s == NULL){
    strcpy(str, "Nenhum tópico subscrito!");
    return str;
  }
  while (aux_s != NULL){
    strcat(str, aux_s->subscricao);
    if (aux_s->prox_sub != NULL)
      strcat(str, " - ");
    aux_s = aux_s->prox_sub;
  }
  return str;
}

/* Obtem uma mensagem */
const char * get_mensagem(ptopico t, char *titulo, char *str){
  ptopico aux;
  pmensagem msg;
  aux = t;
  while (aux != NULL){
    msg = aux->lista_mensagens;
    while (msg != NULL){
      if (!strcasecmp(msg->titulo, titulo)){
        strcpy(str, msg->corpo);
        return str;
      }
      msg = msg->prox_mensagem;
    }
    aux = aux->prox_topico;
  }
  return "Mensagem não encontrada!\n";
}

/* Remove uma mensagem pelo seu título */
void delete_mensagem(ptopico t, char *titulo){
  ptopico aux = t;
  pmensagem msg;

  while (aux != NULL){
    msg = aux->lista_mensagens;
    while (msg != NULL){
      if (!strcasecmp(msg->titulo, titulo)){
        remove_mensagem(t, msg);
        return;
      }
      msg = msg->prox_mensagem;
    }
    aux = aux->prox_topico;
  }
  printf("Mensagem não encontrada!\n");
  return;
}

/* Thread administrador */
void * thread_administrador(){
  char  input[TAM_MAX_PEDIDO], comando[TAM_MAX_PEDIDO], extra[TAM_MAX_PEDIDO];
  int i = 0;
  while (1){
    printf("\n==>");
    fgets(input, sizeof(input), stdin);

    /* Faz split ao input */
    char * token = strtok(input, " ");
    while(token != NULL){
       if (i == 0){
         strcpy(comando, token);
         i++;
       }
       else
          strncpy(extra, token, strlen(token)-1); /* Copia sem o \0 */
    token = strtok(NULL, " ");
   }
    pthread_mutex_lock(&mutex);
    if (!strcasecmp(comando, "filtro")){
      if (!strcasecmp(extra, "on")){
        ativa_filtro();
        printf("Filtro de palavras ativado!\n");
      }
      else if (!strcasecmp(extra, "off")){
        desativa_filtro();
        printf("Filtro de palavras desativado!\n");
      }
      else
        printf("Comando inválido\n");
    }
    else if (!strcasecmp(comando, "users\n"))
      lista_clientes(gestor_unico);
    else if (!strcasecmp(comando, "topics\n"))
      lista_topicos(topicos);
    else if (!strcasecmp(comando, "msg\n"))
      lista_topicos_mensagens(topicos);
    else if (!strcasecmp(comando, "topic"))
      lista_mensagens_topico(topicos, extra);
    else if (!strcasecmp(comando, "del"))
      delete_mensagem(topicos, extra);
    else if (!strcasecmp(comando, "kick")){
      int kick = exclui_utilizador(extra);
      switch (kick) {
        case -1:
          printf("Cliente não encontrado!"); break;
        case 1:
          printf("Cliente [%s] excluido!", extra); break;
      }
    }
    else if (!strcasecmp(comando, "shutdown\n")){
      shutdown(gestor_unico, topicos);
      unlink(SERVER_FIFO);
      kill(pid_programa, SIGKILL);
    }
    else if (!strcasecmp(comando, "prune\n"))
      topicos = elimina_topicos_sem_mensagens(topicos, gestor_unico);
    else if (!strcasecmp(comando, "clear\n")){
      system("clear");
      printf("==>");
    }
    else if (!strcasecmp(comando, "capacity\n")){
      printf("Armazenamento de mensagens [%d/%d]\n", num_mensagens, maxmsg);
    }
    else if (!strcasecmp(comando, "help\n")){
      printf("\nComandos do administrador:\n");
      printf("filtro on\tAtiva o filtro de palavras\n");
      printf("filtro off\tDesativa o filtro de palavras\n");
      printf("users\t\tLista os clientes\n");
      printf("topics\t\tLista os tópicos\n");
      printf("msg\t\tLista os tópicos e as mensagens\n");
      printf("topic topico\tLista as mensagens do tópico\n");
      printf("kick cliente\tExclui um cliente\n");
      printf("shutdown\tTermina o gestor e os clientes\n");
      printf("prune\t\tRemove os tópicos sem mensagens\n");
      printf("clear\t\tLimpa a consola\n");
      printf("capacity\tMostra a capacidade de armazenamento de mensagens\n");
      printf("help\t\tMostra os comandos do administrador\n");
    }
    else {
      printf("Comando inválido\n");
    }
    pthread_mutex_unlock(&mutex);
    i = 0;
    memset(input, '\0', TAM_MAX_PEDIDO);
    memset(comando, '\0', TAM_MAX_PEDIDO);
    memset(extra, '\0', TAM_MAX_PEDIDO);
  }
}

/* COMANDOS ADMINISTRADOR */
void ativa_filtro(){
  filtro_palavras = 1;
}

void desativa_filtro(){
  filtro_palavras = 0;
}

/* Lista os clientes */
void lista_clientes(pgestor g){
  pcliente aux = g->lista_clientes;
  if (g == NULL){
    printf("Nenhum gestor criado!\n");
    return;
  }
  aux = g->lista_clientes;
  if (aux == NULL){
    printf("Nenhum cliente está a usar o MSGDIST!\n");
    return;
  }

  while (aux != NULL){
    printf("Cliente [%d] [%s]\n", aux->pid_cliente, aux->username);
    aux = aux->prox_cliente;
  }
}

/* Lista os tópicos */
void lista_topicos(ptopico t){
  if (t == NULL)
    printf("Nenhum tópico existente!\n");
  else {
    while (t != NULL){
      printf("%s", t->topico);
      if (t->prox_topico != NULL)
        printf(" - ");
      if (t->prox_topico == NULL)
        printf("\n");
      t = t->prox_topico;
    }
  }
}

/* Lista os tópicos todos e as mensagens contidas */
void lista_topicos_mensagens(ptopico t){
  pmensagem aux;

  if (t == NULL){
    printf("Nenhum tópico/mensagem!\n");
    return;
  }

  while (t != NULL){
    printf("%s\n", t->topico);
    aux = t->lista_mensagens;
    while (aux != NULL){
      printf("\t%s\n\t%s\n\t%d\n\n", aux->titulo, aux->corpo, aux->duracao);
      aux = aux->prox_mensagem;
    }
    t = t->prox_topico;
  }
}

/* Lista as mensagens de determinado tópico */
void lista_mensagens_topico(ptopico t, char *topico){
  pmensagem aux;
  if (t == NULL){
    printf("Nenhum tópico existente!\n");
    return;
  }

  while (t != NULL){
    if (!strcasecmp(t->topico, topico)){
      aux = t->lista_mensagens;
      if (aux == NULL){
        printf("Nenhuma mensagem nesse tópico!\n");
        return;
      }
      while (aux != NULL){
        printf("\t%s\n", aux->titulo);
        printf("\t%s\n", aux->corpo);
        aux = aux->prox_mensagem;
      }
      return;
    }
    else {
      if (t->prox_topico == NULL){
        printf("Tópico inexistente!\n");
        return;
      }
      t = t->prox_topico;
    }
  }
}

/* Elimina os tópicos sem mensagens */
ptopico elimina_topicos_sem_mensagens(ptopico t, pgestor g){
  ptopico atual = t, anterior = NULL;
  pcliente aux;
  psubscricao subs;
  char *temp;
  char *aviso;

  /* Se o primeiro nó não tiver mensagens */
  while (atual != NULL && atual->lista_mensagens == NULL){
    t = atual->prox_topico;
    printf("Tópico [%s] removido!\n", atual->topico);
    /* Remover a subscrição dos clientes */
    aux = g->lista_clientes;
    while (aux != NULL){
      subs = aux->lista_subs;
      while (subs != NULL){
        if (!strcasecmp(subs->subscricao, atual->topico)){
          if (cancela_sub(g, aux->pid_cliente, atual->topico) == 1){
            printf("A subscrição ao tópico [%s] do cliente [%s] foi removida\n", atual->topico, aux->username);
            sprintf(aviso, "Subscrição ao tópico [%s] cancelada porque o tópico foi removido!", atual->topico);
            avisa_cliente(aux->pid_cliente, aviso);
            //envia_sinal(aux, SIGUSR1);
          }
          //sprintf(temp, "Subscrição ao tópico [%s] cancelada pelo administrador!", atual->topico);
          //avisa_cliente(aux->pid_cliente, temp);
        }
        subs = subs->prox_sub;
      }
      aux = aux->prox_cliente;
    } /* fim remover a subscrição */
    free(atual);
    atual = t;
  }

  /* Remove sem ser no início */
  while (atual != NULL){
    /* Procura o nó */
    while (atual != NULL && atual->lista_mensagens != NULL){
      anterior = atual;
      atual = atual->prox_topico;
    }

    /* Se não houver nós a remover */
    if (atual == NULL)
      return t;

    anterior->prox_topico = atual->prox_topico;
    printf("Tópico [%s] removido!\n", atual->topico);
    /* Remover a subscrição dos clientes */
    aux = g->lista_clientes;
    while (aux != NULL){
      subs = aux->lista_subs;
      while (subs != NULL){
        if (!strcasecmp(subs->subscricao, atual->topico)){
          if (cancela_sub(g, aux->pid_cliente, atual->topico) == 1){
            printf("A subscrição ao tópico [%s] do cliente [%s] foi removida\n", atual->topico, aux->username);
            sprintf(aviso, "Subscrição ao tópico [%s] cancelada porque o tópico foi removido!", atual->topico);
            avisa_cliente(aux->pid_cliente, aviso);
          }

          //sprintf(temp, "Subscrição ao tópico [%s] cancelada pelo administrador!", atual->topico);
          //avisa_cliente(aux->pid_cliente, temp);
        }
        subs = subs->prox_sub;
      }
      aux = aux->prox_cliente;
    } /* fim remover a subscrição */
    free(atual);
    atual = anterior->prox_topico;
  }
  return t;
}

/* Exclui um utilizador (comando KICK username) */
int exclui_utilizador(char *username){
  pcliente aux = gestor_unico->lista_clientes;

  while (aux != NULL){
    if (!strcasecmp(aux->username, username))
      break;
    aux = aux->prox_cliente;
  }

  if (aux == NULL)
    return -1;

  avisa_cliente(aux->pid_cliente, "Excluido pelo administrador!");
  gestor_unico->lista_clientes = logout_cliente(aux->pid_cliente);
  envia_sinal(aux, SIGKILL);
  return 1;
}

/* Shudown */
void shutdown(pgestor g, ptopico t){
  pcliente aux = g->lista_clientes;
  ptopico atual, prox = NULL;
  pmensagem m_atual, m_prox = NULL;
  /* Remove os clientes da lista ligada
     Remove as subscricoes
     Termina os processos cliente */
  while (aux != NULL){
    printf("Remoção do utilizador [%s]\n", aux->username);
    exclui_utilizador(aux->username);
    aux = aux->prox_cliente;
  }

  /* Remove os tópicos e mensagens */
  atual = t;
  while (atual != NULL){
    prox = atual->prox_topico;
    printf("Remoção do tópico %s\n", atual->topico);
    m_atual = atual->lista_mensagens;
    while (m_atual != NULL){
      m_prox = m_atual->prox_mensagem;
      printf("\tRemoção da mensagem %s\n", m_atual->titulo);
      free(m_atual);
      m_atual = m_prox;
    }
    free(atual);
    atual = prox;
  }

}

/* Envia um sinal ao cliente */
void envia_sinal(pcliente cliente, int signal){
  kill(cliente->pid_cliente, signal);
}

/* Verifica o conteúdo de uma mensagem */
pid_t verifica_mensagem(int * ppg, int * ppv){
  int p_stdin[2], p_stdout[2];
  pid_t pid;

  if (pipe(p_stdin) != 0)
    printf("Erro na criação do pipe");
  if (pipe(p_stdout) != 0)
    printf("Erro na criação do pipe");

  pid = fork();
  if (pid < 0)
    printf("Erro no fork()");
  else if (pid == 0){
    /* Filho */
    close(p_stdin[WRITE_PIPE]);
    dup2(p_stdin[READ_PIPE], READ_PIPE);
    close(p_stdout[READ_PIPE]);
    dup2(p_stdout[WRITE_PIPE], WRITE_PIPE);
    execl("verificador", "verificador", wordsnot, NULL);
    perror("execl");
    exit(1);
  }

  if (ppg == NULL)
    close(p_stdin[WRITE_PIPE]);
  else
    *ppg = p_stdin[WRITE_PIPE];

  if (ppv == NULL)
    close(p_stdout[READ_PIPE]);
  else
    *ppv = p_stdout[READ_PIPE];

  return pid;
}
