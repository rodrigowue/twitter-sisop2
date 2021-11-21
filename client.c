#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

// Global variables
volatile sig_atomic_t flag = 0;
volatile sig_atomic_t flag_server = 0;

int sockfd = 0;
char name[32];

void str_overwrite_stdout() {
  printf("%s", "> ");
  fflush(stdout);
}

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) {
    if(sig==3){
      flag_server=1;
    }
    else{
    flag = 1;
    }
}

void send_msg_handler() {
  char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

  while(1) {
  	str_overwrite_stdout();
    fgets(message, LENGTH, stdin);
    str_trim_lf(message, LENGTH);

    if (strcmp(message, "exit") == 0) {
			break;
    } else {
      sprintf(buffer, "%s: %s\n", name, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

		bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
  catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
	char message[LENGTH] = {};
  while (1) {
		int receive = recv(sockfd, message, LENGTH, 0);
    if (receive > 0) {
      printf("\n----------------------------------\n");
      printf("%s \n", message);
      printf("----------------------------------\n");
      str_overwrite_stdout();
    } else if (receive == 0) {
			break;
    } else {
			// -1
		}
		memset(message, 0, sizeof(message));
  }
  catch_ctrl_c_and_exit(3);
}

int main(int argc, char **argv){
	if(argc != 4){
		printf("Usage: %s <username> <server_addr> <port>\n", argv[0]);
		return EXIT_FAILURE;
	}


  printf("%s\n",argv[1]);
  char *name = argv[1];

  if (name[0]!='@'){
    printf("Your username should start with @\n");
    return EXIT_FAILURE;
  }

	if (strlen(name) > 32 || strlen(name) < 2){
		printf("Name must be less than 30 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

  char *ip = argv[2];
	int port = atoi(argv[3]);

  while(1){
  flag_server=0;
	signal(SIGINT, catch_ctrl_c_and_exit);
	struct sockaddr_in server_addr;
  float result;
  float best_result = 9999.9999;
  int best_host = 99;
  int online = 0;
  printf("\e[0m\n____________________________________\n");
  printf(" Evaluating Up-Servers                     \n");
  printf("____________________________________\n");
  //==============================================================================
  //QoS Measurements for server 127.0.0.1
  //==============================================================================
  //measure delay of servers using ping ------------------------------------------
  char *cmds0 = "ping -c 1 127.0.0.1 | grep -o 'time=[0-9*.]*' | sed 's/time=/\/g'";
  FILE *cmd0 = popen ( cmds0, "r" );
  char *s = malloc ( sizeof ( char ) * 100 );
  fgets (s,sizeof(char)*100,cmd0);
  result = atof(s);
  printf("127.0.0.1: %f ms",result);//show outcome

  // Connect to Server
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(port);
  int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
      printf(" \e[1;97;41mOFFLINE\e[0m\n");
  }
  else{
    printf(" \e[1;97;44mONLINE\e[0m\n");
    best_result = result;
    best_host = 1;
  }
  close(sockfd);
  pclose(cmd0);


  //==============================================================================
  //QoS Measurements for server 127.0.0.2
  //==============================================================================
  char *cmds1 = "ping -c 1 127.0.0.2 | grep -o 'time=[0-9*.]*' | sed 's/time=/\/g'";
  FILE *cmd1 = popen ( cmds1, "r" );
  fgets (s,sizeof(char)*100,cmd1);
  result = atof(s);
  printf("127.0.0.2: %f ms",result);//show outcome
  // Connect to Server
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.2");
  server_addr.sin_port = htons(port);
  err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
    printf(" \e[1;97;41mOFFLINE\e[0m\n");
  }
  else{
    printf(" \e[1;97;44mONLINE\e[0m\n");
    if(result<best_result){
      best_host = 2;
      best_result = result;
    }
  }
  close(sockfd);
  pclose(cmd1);


  //==============================================================================
  //QoS Measurements for server 127.0.0.3
  //==============================================================================
  char *cmds2 = "ping -c 1 127.0.0.3 | grep -o 'time=[0-9*.]*' | sed 's/time=/\/g'";
  FILE *cmd2 = popen ( cmds2, "r" );
  fgets (s,sizeof(char)*100,cmd2);
  result = atof(s);
  printf("127.0.0.3: %f ms",result);//show outcome
  // Connect to Server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.3");
  server_addr.sin_port = htons(port);
  err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
		printf(" \e[1;97;41mOFFLINE\e[0m\n");
	}
  else{
    printf(" \e[1;97;44mONLINE\e[0m\n");
    if(result<best_result){
      best_host = 3;
      best_result = result;
    }
  }
  close(sockfd);
  pclose(cmd2);
  free(s);
  //==============================================================================
  //Decide which server to connect
  //==============================================================================
  if(best_host==99){
    printf("\e[1;97;44mThere are no servers online!\e[0m\n");
    return EXIT_FAILURE;
  }
  printf("____________________________________\n");
  printf("The best up-server is 127.0.0.%d\n", best_host);
  sprintf(ip,"127.0.0.%d",best_host);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);
  err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	// Send name
	send(sockfd, name, 32, 0);
  printf("\e[1;97;44m____________________________________\n");
  printf("\e[1;97;44m Twitter Client                     \n");
  printf("\e[1;97;44m____________________________________\n");

	pthread_t send_msg_thread;
  if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
    return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag|flag_server){
			break;
    }
	}
  if (flag) {
    printf("\nBye\n");
    break;
  }
	close(sockfd);
}
	return EXIT_SUCCESS;
}
