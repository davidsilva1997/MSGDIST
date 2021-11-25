#include "cliente.h"

int main (int argc, char **argv){
  int             s_fifo_fd;  /* Identificador do FIFO do servidor */
  int             c_fifo_fd;  /* Identificador do FIFO do cliente */
  char            c_fifo_fname[TAM_FIFO_FNAME]; /* Nome do FIFO deste cliente */
  pedido_server   pedido;
  resposta_server resposta;
  pthread_args    args;
  pthread_t       threadPedidos, threadRespostas, threadEncerramentoGestor;

  /* Verifica o número de argumentos */
  if (argc != 2)
    sair("Número de argumentos inválido!\n");

  /* Verifica o tamanho do username */
  if (strlen(argv[1]) > TAM_USERNAME)
    sair("O tamanho do username ultrapassa o limite!\n");

  /* Verifica se existe um processo gestor em execução */
  if (verifica_processo_gestor() == 0)
    sair("Nenhum processo gestor em execução!\n");

  setbuf(stdout, NULL);

  /* Cria o FIFO do cliente */
  cria_FIFO_cliente(&pedido, c_fifo_fname);

  /* Abre o FIFO do servidor */
  abre_FIFO_servidor(&s_fifo_fd, c_fifo_fname);

  /* Abre o FIFO do cliente */
  abre_FIFO_cliente(&c_fifo_fd, &s_fifo_fd, c_fifo_fname);

  /* Argumentos passados para os threads */
  args = malloc(sizeof(thread_args));
  strcpy(args->username, argv[1]);
  strcpy(args->c_fifo_fname, c_fifo_fname);
  args->c_fifo_fd = &c_fifo_fd;
  args->s_fifo_fd = &s_fifo_fd;
  args->pedido = &pedido;
  args->resposta = &resposta;

  /* Criação do thread dos pedidos */
  if (pthread_create(&threadPedidos, NULL, thread_pedidos, args) == -1)
    sair("Erro na criação do thread dos pedidos ao gestor!");
  /* Criação do thread de respostas do servidor */
  if (pthread_create(&threadRespostas, NULL, thread_respostas, args) == -1)
    sair("Erro na criação do thread das respostas do gestor!");
  /* Criação do thread de verificação de encerramento do gestor */
  if (pthread_create(&threadEncerramentoGestor, NULL, thread_encerramento_gestor, args) == -1)
    sair("Erro na criação do thread da verificação do encerramento do gestor!");

  //mostra_menu(argv[1]);

  /* Espera pelo fim do thread */
  if (pthread_join(threadPedidos, NULL))
    sair("Erro na espera do fim do thread dos pedidos!");
  if (pthread_join(threadRespostas, NULL))
    sair("Erro na espera do fim do thread das respostas!");
  if (pthread_join(threadEncerramentoGestor, NULL))
    sair("Erro na espera do fim do thread da verificalçao do encerramento do gestor!");

  remove_FIFO_cliente(c_fifo_fname);

  return 0;
}

/* Cria o FIFO do cliente */
void cria_FIFO_cliente(pedido_server *pedido, char c_fifo_fname[TAM_FIFO_FNAME]){
  pedido->pid_cliente = getpid();
  sprintf(c_fifo_fname, CLIENTE_FIFO, pedido->pid_cliente);
  if ((mkfifo(c_fifo_fname, 0777)) == -1){ /* 0777 - rwxrwxrwx */
    sair("Erro na criação do FIFO do cliente!\n");
  }
}

/* Remove o FIFO do cliente */
void remove_FIFO_cliente(char c_fifo_fname[TAM_FIFO_FNAME]){
  unlink(c_fifo_fname);
}

/* Abre o FIFO do servidor */
void abre_FIFO_servidor(int *s_fifo_fd, char c_fifo_fname[TAM_FIFO_FNAME]){
  *s_fifo_fd = open(SERVER_FIFO, O_WRONLY); /* bloqueante */
  if (*s_fifo_fd == -1){
    unlink(c_fifo_fname);
    sair("O servidor não está a correr!\n");
  }
}

/* Abre o FIFO do cliente para read+write
   Abertura read+write -> evita o comportamento de ficar
   bloqueado no open. A execução prossegue e as operações
   read/write (neste caso APENAS read) continuam bloqueante */
void abre_FIFO_cliente(int *c_fifo_fd, int *s_fifo_fd, char c_fifo_fname[TAM_FIFO_FNAME]){
  *c_fifo_fd = open(c_fifo_fname, O_RDWR);
  if (*c_fifo_fd == -1){
    close(*s_fifo_fd);
    unlink(c_fifo_fname);
    sair("Erro ao abrir o FIFO do cliente!\n");
  }
}

/* Obtem as respostas do gestor (servidor)*/
void obtem_resposta(resposta_server *resposta, int *c_fifo_fd){
  if ((read(*c_fifo_fd, resposta, sizeof(*resposta))) == sizeof(*resposta))
    printf("\n> %s\n> %s\n", resposta->palavra, resposta->extra);
  else
    printf("Sem resposta ou resposta incompleta!");
  printf("\n--->");
}

/* Envia um pedido ao gestor (servidor) */
void envia_pedido(char palavra[TAM_MAX_PEDIDO], char extra[TAM_MAX_PEDIDO], pedido_server *pedido, int *s_fifo_fd){
  strcpy(pedido->palavra, palavra);
  strcpy(pedido->extra, extra);
  strcpy(pedido->topico, "");
  strcpy(pedido->titulo, "");
  strcpy(pedido->corpo, "");
  pedido->duracao = 0;
  /* Escreve o pedido no FIFO do servidor */
  write(*s_fifo_fd, pedido, sizeof(*pedido));
}

/* Envia um pedido de nova mensagem */
void envia_pedido_mensagem(char palavra[TAM_MAX_PEDIDO], pnova_mensagem msg, pedido_server *pedido, int *s_fifo_fd){
  strcpy(pedido->palavra, palavra);
  strcpy(pedido->extra, "");
  strcpy(pedido->topico, msg->topico);
  strcpy(pedido->titulo, msg->titulo);
  strcpy(pedido->corpo, msg->corpo);
  pedido->duracao = msg->duracao;
  /* Escreve o pedido no FIFO do servidor */
  write(*s_fifo_fd, pedido, sizeof(*pedido));
}

/* Thread dos pedidos */
void * thread_pedidos(void * args){
  char  input[TAM_MAX_PEDIDO], comando[TAM_MAX_PEDIDO], extra[TAM_MAX_PEDIDO], temp[TAM_MAX_PEDIDO];
  int   i = 0;
  pthread_args dados = args;
  pedido_server * pedido = dados->pedido;
  int *s_fifo_fd = dados->s_fifo_fd;


  /* Envia um pedido ao gestor */
  envia_pedido("login", dados->username, pedido, s_fifo_fd);

  /* Pedidos */
  while (1){
    fgets(input, sizeof(input), stdin);
    /* Faz split ao input */
    char * token = strtok(input, " ");
    while(token != NULL){
       if (i == 0){
         strcpy(temp, token); /* comando */
         i++;
       }
       else
          strncpy(extra, token, strlen(token)-1); /* Copia sem o \0 */
      token = strtok(NULL, " ");
    }

   if (strlen(extra) == 0) /* Se não houver extra o comando fica com \n então é preciso remove-lo */
     strncpy(comando, temp, strlen(temp)-1);
   else
      strcpy(comando, temp);

    if (!strcasecmp(comando, "logout"))
      break;

    if (!strcasecmp(comando, "msg")){
      pnova_mensagem msg = preenche_nova_mensagem();
      envia_pedido_mensagem(comando, msg, pedido, s_fifo_fd);
    }
    else
      envia_pedido(comando, extra, pedido, s_fifo_fd);

    i = 0;
    memset(input, '\0', TAM_MAX_PEDIDO);
    memset(comando, '\0', TAM_MAX_PEDIDO);
    memset(extra, '\0', TAM_MAX_PEDIDO);
    memset(temp, '\0', TAM_MAX_PEDIDO);
  }

  /* Envia um pedido de logout */
  envia_pedido("logout", "", pedido, s_fifo_fd);
  remove_FIFO_cliente(dados->c_fifo_fname);
  exit(1);

  pthread_exit(NULL);
}

/* Thread das respostas */
void * thread_respostas(void * args){
  pthread_args dados = args;
  resposta_server * resposta = dados->resposta;
  int * c_fifo_fd = dados->c_fifo_fd;
  while(1){
    obtem_resposta(resposta, c_fifo_fd);
  }
  pthread_exit(NULL);
}

/* Thead verifica encerramento do gestor */
void * thread_encerramento_gestor(void * args){
  pthread_args dados = args;
  while (1){
    sleep(10);
    if (verifica_processo_gestor() == 0){
      printf("\n\nNenhum processo gestor em execução.\n");
      remove_FIFO_cliente(dados->c_fifo_fname);
      exit(-1);
    }
  }
  pthread_exit(NULL);
}

/* Preenche os dados da mensagem */
pnova_mensagem preenche_nova_mensagem(){
  pnova_mensagem msg;

  msg = malloc(sizeof(nova_mensagem));
  if (msg == NULL){
    printf("Erro na alocação de memória para a mensagem!\n");
    return NULL;
  }

  /* Menu NCURSES
  WINDOW *topico, *titulo, *corpo;
  initscr();

  clear();
  topico = subwin(stdscr, 4, COLS, 0, 0);
  titulo = subwin(stdscr, 4, COLS, 4, 0);
  corpo =  subwin(stdscr, LINES - 8, COLS, 8, 0);

  box(topico, 0,0);
  box(titulo, 0,0);
  box(corpo, 0,0);

  mvwprintw(topico, 1, 2, "Tópico");
  move(2, 2);
  wrefresh(topico);
  getnstr(msg->topico, TAM_TOPICO);

  mvwprintw(titulo, 1, 2, "Título");
  move(6, 2);
  wrefresh(titulo);
  getnstr(msg->titulo, TAM_TOPICO);

  mvwprintw(corpo, 1, 2, "Corpo");
  move(10, 2);
  wrefresh(corpo);
  getnstr(msg->corpo, TAM_CORPO);

  delwin(topico);
  delwin(titulo);
  delwin(corpo);
  endwin();

  msg->duracao = DURACAO_MSG;
  return msg;*/

  /* SEM NCURSES */
  printf("Insira o tópico da mensagem:\n");
  do {
    fgets(msg->topico, TAM_TITULO, stdin);
    if ((strlen(msg->topico) < 2) || (strlen(msg->topico) > (TAM_TOPICO - 1))){
      printf("O tópico deve conter entre 2 e 49 carateres!\n");
      memset(msg->topico, '\0', TAM_TOPICO);
    }
  } while ((strlen(msg->topico) < 2) || (strlen(msg->topico) > (TAM_TOPICO - 1)));

  printf("Insira o título da mensagem:\n");
  do {
    fgets(msg->titulo, TAM_TITULO, stdin);
    if ((strlen(msg->titulo) < 2) || (strlen(msg->titulo) > (TAM_TITULO - 1))){
      printf("O título deve conter entre 2 e 49 carateres!\n");
      memset(msg->titulo, '\0', TAM_TITULO);
    }
  } while ((strlen(msg->titulo) < 2) || (strlen(msg->titulo) > (TAM_TITULO - 1)));

  printf("Insira o corpo da mensagem:\n");
  do {
    fgets(msg->corpo, TAM_CORPO, stdin);
    if ((strlen(msg->corpo) < 2) || (strlen(msg->corpo) > (TAM_CORPO - 1))){
      printf("O corpo deve conter entre 2 e 899 carateres!\n");
      memset(msg->corpo, '\0', TAM_CORPO);
    }
  } while ((strlen(msg->corpo) < 2) || (strlen(msg->corpo) > (TAM_CORPO - 1)));

  // Remove o \n
  strtok(msg->topico, "\n");
  strtok(msg->titulo, "\n");
  strtok(msg->corpo, "\n");

  msg->duracao = DURACAO_MSG;

  return msg;
}

/* Mostra o menu feito com ncurses */
void mostra_menu(char *username){
  WINDOW  *cabecalho, *menu, *dados, *novidades;
  /* Cabeçalho */
  char    *welcome_msg = "Welcome to MSGDIST ";
  int     tamanho_welcome = strlen(welcome_msg) + strlen(username);
  /* Menu */
  char    *opcoes_menu[NUM_OPCAO] = {"Criar mensagem", "Consultar lista tópicos", "Consultar lista títulos",
    "Consultar mensagem tópico", "Subscrever tópico", "Cancelar subscrição tópico", "Sair"};
  int opcao_realcada = 0, opcao_escolhida;

  initscr();      // Inicialização da estrutura WINDOW

  do {
    clear();
    cabecalho = subwin(stdscr, 3, COLS, 0, 0);
    menu      = subwin(stdscr, LINES - 6, COLS / 2, 3, 0);
    dados     = subwin(stdscr, LINES - 6, COLS / 2, 3, COLS / 2);
    novidades = subwin(stdscr, 3, COLS, LINES - 3, 0);

    box(cabecalho, ACS_DIAMOND, 0);
    box(menu, 0, 0);
    box(dados, 0, 0);
    box(novidades, 0, 0);

    mvwprintw(cabecalho, 1, (COLS / 2) - (tamanho_welcome / 2), "%s%s",welcome_msg, username);
    mvwprintw(menu, 0, (COLS / 4) - 2, "MENU");
    mvwprintw(dados, 0, (COLS / 4) - 2, "DADOS");
    mvwprintw(novidades, 0, (COLS / 2) - 4, "NOVIDADES");

    wrefresh(cabecalho);
    wrefresh(menu);
    wrefresh(dados);
    wrefresh(novidades);

    // MENU
    keypad(menu, true);
    do {
      for (int i = 0; i < NUM_OPCAO; i++){
        // Realça a opção atual
        if (i == opcao_realcada){
          wattron(menu, A_STANDOUT);
        }
        mvwprintw(menu, i + 2, 1, opcoes_menu[i]);
        wattroff(menu, A_STANDOUT);
      }

      opcao_escolhida = wgetch(menu);

      switch(opcao_escolhida){
          case KEY_UP:
            if (opcao_realcada == 0)
              break;
            opcao_realcada--;
            break;
          case KEY_DOWN:
            if (opcao_realcada == (NUM_OPCAO - 1))
              break;
            opcao_realcada++;
            break;
          default:
            break;
        }

    } while (opcao_escolhida != 10);

    switch(opcao_realcada){
      case 0:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
      case 1:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
      case 2:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
      case 3:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
      case 4:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
      case 5:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
      case 6:
        mvwprintw(dados, 1, 1, "%s", opcoes_menu[opcao_realcada]);
        break;
    }

  } while (getch() == 410);

  /* Menu
  keypad(menu, true);      //Ativa as setas para mexer no menu
  while (1){
    wrefresh(menu);
    for (int i = 0; i < NUM_OPCAO; i++){
      //Realça a opçao atual
      if (i == opcao_realcada){
        wattron(menu, A_STANDOUT);
      }
      mvwprintw(menu, i + 2, 1, opcoes_menu[i]);
      wattroff(menu, A_STANDOUT);
    }

    opcao_escolhida = wgetch(menu);

    switch(opcao_escolhida){
        case KEY_UP:
          if (opcao_realcada == 0)
            break;
          opcao_realcada--;
          break;
        case KEY_DOWN:
          if (opcao_realcada == (NUM_OPCAO - 1))
            break;
          opcao_realcada++;
          break;
        default:
          break;
      }

       //Verifica se o utilizador carregou no ENTER
      if (opcao_escolhida == 10)
        break;
    }
*/


  getch();
  endwin();

  free(cabecalho);
  free(menu);
  free(dados);
  free(novidades);
}
