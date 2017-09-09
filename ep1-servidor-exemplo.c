/* Por Prof. Daniel Batista <batista@ime.usp.br>
 * Em 9/8/2017
 * 
 * Um código simples (não é o código ideal, mas é o suficiente para o
 * EP) de um servidor de eco a ser usado como base para o EP1. Ele
 * recebe uma linha de um cliente e devolve a mesma linha. Teste ele
 * assim depois de compilar:
 * 
 * ./servidor 8000
 * 
 * Com este comando o servidor ficará escutando por conexões na porta
 * 8000 TCP (Se você quiser fazer o servidor escutar em uma porta
 * menor que 1024 você precisa ser root).
 *
 * Depois conecte no servidor via telnet. Rode em outro terminal:
 * 
 * telnet 127.0.0.1 8000
 * 
 * Escreva sequências de caracteres seguidas de ENTER. Você verá que
 * o telnet exibe a mesma linha em seguida. Esta repetição da linha é
 * enviada pelo servidor. O servidor também exibe no terminal onde ele
 * estiver rodando as linhas enviadas pelos clientes.
 * 
 * Obs.: Você pode conectar no servidor remotamente também. Basta saber o
 * endereço IP remoto da máquina onde o servidor está rodando e não
 * pode haver nenhum firewall no meio do caminho bloqueando conexões na
 * porta escolhida.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define LISTENQ 1
#define MAXDATASIZE 100
#define MAXLINE 4096

#define MAXWORDS 10
#define MAXLINES 10

#define TRUE 1
#define FALSE 0

#define LINEBREAK 10
#define WORDBREAK 20

typedef int boolean;


boolean login(char *username, char *password) {
    /* Usuários */
    char user1[] = "carlos@127.0.0.1";
    char pass1[] = "pass";
    char user2[] = "joao";
    char pass2[] = "1q";

    if (!strcmp(username, user1) && !strcmp(password, pass1))
      return TRUE;

    if (!strcmp(username, user2) && !strcmp(password, pass2))
      return TRUE;

    return FALSE;
}

int read_message(char *input, char *arguments[], int type)
{
  char *text, *token;
  int i, j, k, size;
  char tk;

  if (type == LINEBREAK) {
    tk = '\n';
  } else {
    tk = ' ';
  }

  text = (char *) malloc(sizeof(char) * MAXLINE + 1);
  text = input;
  size = strlen(text);

  token = (char *) malloc(sizeof(char) * MAXLINES + 1);
  j = 0;
  k = 0;
  for (i = 0; i < size; i++) {
    if (text[i] != tk) {
      if (text[i] == '"') continue;
      token[j] = text[i];
      j++;
    } else {
      if (j == 0) continue;
      token[j] = 0;
      strcpy(arguments[k], token);
      k++;
      j = 0;
      token = (char *) malloc(sizeof(char) * MAXLINES + 1);
    }
  }
  if (j) {
      token[j-1] = 0;
      strcpy(arguments[k], token);
      k++;
  }
  return k;
}

int main (int argc, char **argv) {
   /* Os sockets. Um que será o socket que vai escutar pelas conexões
    * e o outro que vai ser o socket específico de cada conexão */
    int listenfd, connfd;
   /* Informações sobre o socket (endereço e porta) ficam nesta struct */
    struct sockaddr_in servaddr;
   /* Retorno da função fork para saber quem é o processo filho e quem
    * é o processo pai */
   pid_t childpid;
   /* Armazena linhas recebidas do cliente */
    char    recvline[MAXLINE + 1];

   /* Armazwna linhas enviadas ao cliente */
    char senvline[MAXLINE + 1];
   /* Armazena o tamanho da string lida do cliente */
   ssize_t  n;
   
    if (argc != 2) {
      fprintf(stderr,"Uso: %s <Porta>\n",argv[0]);
      fprintf(stderr,"Vai rodar um servidor de echo na porta <Porta> TCP\n");
        exit(1);
    }

   /* Criação de um socket. Eh como se fosse um descritor de arquivo. Eh
    * possivel fazer operacoes como read, write e close. Neste
    * caso o socket criado eh um socket IPv4 (por causa do AF_INET),
    * que vai usar TCP (por causa do SOCK_STREAM), já que o IMAP
    * funciona sobre TCP, e será usado para uma aplicação convencional sobre
    * a Internet (por causa do número 0) */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket :(\n");
        exit(2);
    }

   /* Agora é necessário informar os endereços associados a este
    * socket. É necessário informar o endereço / interface e a porta,
    * pois mais adiante o socket ficará esperando conexões nesta porta
    * e neste(s) endereços. Para isso é necessário preencher a struct
    * servaddr. É necessário colocar lá o tipo de socket (No nosso
    * caso AF_INET porque é IPv4), em qual endereço / interface serão
    * esperadas conexões (Neste caso em qualquer uma -- INADDR_ANY) e
    * qual a porta. Neste caso será a porta que foi passada como
    * argumento no shell (atoi(argv[1]))
    */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(atoi(argv[1]));
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind :(\n");
        exit(3);
    }

   /* Como este código é o código de um servidor, o socket será um
    * socket passivo. Para isto é necessário chamar a função listen
    * que define que este é um socket de servidor que ficará esperando
    * por conexões nos endereços definidos na função bind. */
    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen :(\n");
        exit(4);
    }

   printf("[Servidor no ar. Aguardando conexoes na porta %s]\n",argv[1]);
   printf("[Para finalizar, pressione CTRL+c ou rode um kill ou killall]\n");
   
   /* O servidor no final das contas é um loop infinito de espera por
    * conexões e processamento de cada uma individualmente */
    for (;;) {
      /* O socket inicial que foi criado é o socket que vai aguardar
       * pela conexão na porta especificada. Mas pode ser que existam
       * diversos clientes conectando no servidor. Por isso deve-se
       * utilizar a função accept. Esta função vai retirar uma conexão
       * da fila de conexões que foram aceitas no socket listenfd e
       * vai criar um socket específico para esta conexão. O descritor
       * deste novo socket é o retorno da função accept. */
        if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
            perror("accept :(\n");
            exit(5);
        }
      
      /* Agora o servidor precisa tratar este cliente de forma
       * separada. Para isto é criado um processo filho usando a
       * função fork. O processo vai ser uma cópia deste. Depois da
       * função fork, os dois processos (pai e filho) estarão no mesmo
       * ponto do código, mas cada um terá um PID diferente. Assim é
       * possível diferenciar o que cada processo terá que fazer. O
       * filho tem que processar a requisição do cliente. O pai tem
       * que voltar no loop para continuar aceitando novas conexões */
      /* Se o retorno da função fork for zero, é porque está no
       * processo filho. */
      if ( (childpid = fork()) == 0) {
         /**** PROCESSO FILHO ****/
         printf("[Uma conexao aberta]\n");
         /* Já que está no processo filho, não precisa mais do socket
          * listenfd. Só o processo pai precisa deste socket. */
         close(listenfd);
         
         /* Agora pode ler do socket e escrever no socket. Isto tem
          * que ser feito em sincronia com o cliente. Não faz sentido
          * ler sem ter o que ler. Ou seja, neste caso está sendo
          * considerado que o cliente vai enviar algo para o servidor.
          * O servidor vai processar o que tiver sido enviado e vai
          * enviar uma resposta para o cliente (Que precisará estar
          * esperando por esta resposta) 
          */
         strcpy(senvline, "* OK [CAPABILITY IMAP4]\n");
         write(connfd, senvline, strlen(senvline));
         strcpy(senvline, "");

         /* ========================================================= */
         /* ========================================================= */
         /*                         EP1 INÍCIO                        */
         /* ========================================================= */
         /* ========================================================= */
         /* TODO: É esta parte do código que terá que ser modificada
          * para que este servidor consiga interpretar comandos IMAP  */
         while ((n=read(connfd, recvline, MAXLINE)) > 0) {
            int i, k, count;
            char **lines;
            char **words;
            char messages[MAXLINE+1];
            boolean response;


            lines = (char **) malloc (sizeof(char*) * MAXLINES);
            words = (char **) malloc (sizeof(char*) * MAXWORDS); 
            recvline[n]=0;

            for(k = 0; k < MAXLINES; k++) lines[k] = (char *) malloc (sizeof(char) * MAXLINE);

            count = read_message(recvline, lines, LINEBREAK);

            for (i = 0; i < count; i++) {

              for(k = 0; k < MAXWORDS; k++) words[k] = (char *) malloc (sizeof(char) * MAXLINE);
              read_message(lines[i], words, WORDBREAK);
              strcpy(senvline, words[0]);

              if (!strcmp(words[1], "CAPABILITY")) {
                    strcpy(messages, "* CAPABILITY IMAP4rev1 AUTH=PLAIN\n");
                    write(connfd, messages, strlen(messages));
                    strcat(senvline, " OK - capability completed\n");
                    write(connfd, senvline, strlen(senvline));
              } else if (!strcmp(words[1], "STARTTLS")) {
                    strcat(senvline, " OK Begin TLS negotiation now\n");
                    write(connfd, senvline, strlen(senvline));
                    strcpy(messages, "<TLS negotiation, further commands are under [TLS] layer>\n");
                    write(connfd, messages, strlen(messages));
              } else if (!strcmp(words[1], "login")) {
                  response = login(words[2], words[3]);
                  if (response == TRUE) {
                    strcat(senvline, " OK - login completed, now in authenticated state\n");
                    write(connfd, senvline, strlen(senvline));
                  } else{
                    strcat(senvline, " NO - login failure: user name or password rejected\n");
                    write(connfd, senvline, strlen(senvline));
                    break;
                  }
              } else if (!strcmp(words[1], "LIST")) {
                /* TO DO */
              } else if (!strcmp(words[1], "LOGOUT")) {
                    strcpy(messages, "* BYE IMAP4rev1 Server logging out\n");
                    write(connfd, messages, strlen(messages));
                    strcat(senvline, " OK - logout completed\n");
                    write(connfd, senvline, strlen(senvline));
                    break;
              } else {
                  strcpy(senvline, " BAD - command unknown or arguments invalid\n");
                  write(connfd, senvline, strlen(senvline));
                  break;
              }
              strcpy(senvline, "");
            }
            strcpy(recvline, "");
         }
         /* ========================================================= */
         /* ========================================================= */
         /*                         EP1 FIM                           */
         /* ========================================================= */
         /* ========================================================= */

         /* Após ter feito toda a troca de informação com o cliente,
          * pode finalizar o processo filho */
         printf("[Uma conexao fechada]\n");
         exit(0);
      }
      /**** PROCESSO PAI ****/
      /* Se for o pai, a única coisa a ser feita é fechar o socket
       * connfd (ele é o socket do cliente específico que será tratado
       * pelo processo filho) */
        close(connfd);
    }
    exit(0);
}