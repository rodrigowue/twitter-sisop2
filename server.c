#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
static char follow_command[] = "FOLLOW";
static char stats_command[] = "STATS";
static char send_command[] = "SEND";
int num_followers = 0;
/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	int num_followers;
	// struct client_t *followers[MAX_CLIENTS];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void str_overwrite_stdout() {
    printf("\r%s", "> ");
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

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}


void add_follower(char *to_be_followed, client_t *follower)
{
	pthread_mutex_lock(&clients_mutex);
	client_t * temp = follower;
	int bufferLength = 255;
	char buffer[bufferLength];
	if(strstr(temp->name,to_be_followed)){
		printf("Can't follow himself\n");
		char message[32];
		snprintf(message, sizeof(message), "You cant follow yourself :[\n");
		if(write(temp->sockfd, message, strlen(message)) < 0){
			perror("ERROR: write to descriptor failed");}
	}
	else{
	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(strstr(clients[i]->name,to_be_followed)){
				FILE *fp;
				char filename1[256];
				char follower_name[256];
				int already_follow = 0;
				snprintf(filename1, sizeof(filename1), "data/%s_followers", clients[i]->name);

				if( access( filename1, F_OK ) == 0 ) {
					fp = fopen(filename1, "r");
					//verify if he already followers
					while(fgets(buffer, bufferLength, fp)) {
						strtok(buffer,"\n");
						if(strstr(temp->name, buffer)){
							char message[32];
							snprintf(message, sizeof(message), "you already follow %s!\n",clients[i]->name);
							if(write(temp->sockfd, message, strlen(message)) < 0){
								perror("ERROR: write to descriptor failed");
								break;}
							already_follow = 1;
							break;
						}
					}
					fclose(fp);
				}

				if(already_follow != 1){
					fp = fopen(filename1, "a+");
					snprintf(follower_name, sizeof(follower_name), "%s\n", temp->name);
					fprintf(fp, follower_name);
					clients[i]->num_followers++;
					num_followers = clients[i]->num_followers;
					fclose(fp);

					char message[32];
					snprintf(message, sizeof(message), "you followed %s!\n",clients[i]->name);
					if(write(temp->sockfd, message, strlen(message)) < 0){
						perror("ERROR: write to descriptor failed");
						break;}
				  snprintf(message, sizeof(message), "%s followed you!\n",temp->name);
						if(write(clients[i]->sockfd, message, strlen(message)) < 0){
							perror("ERROR: write to descriptor failed");
							break;}
					printf("%s followed %s\n",temp->name, clients[i]->name);

				}

				}
			}
		}
	}
  pthread_mutex_unlock(&clients_mutex);
}

void send_private_message_file(char *s, char *name){
	pthread_mutex_lock(&clients_mutex);
	char filename2[256];
	snprintf(filename2, sizeof(filename2), "data/%s_followers", name);
	int bufferLength = 255;
	char buffer[bufferLength];
	FILE *fp;
	if( access( filename2, F_OK ) == 0 ) {
		fp = fopen(filename2, "r");

		while(fgets(buffer, bufferLength, fp)) {
			strtok(buffer,"\n");
			for(int i=0; i<MAX_CLIENTS; ++i){
				if(clients[i]){
					if(strstr(clients[i]->name, buffer)){
						if(write(clients[i]->sockfd, s, strlen(s)) < 0){
							perror("ERROR: write to descriptor failed");
							break;
						}
					}
				}
			}
		}
		fclose(fp);
		} else {
  printf("no followers file were found\n");
	}
	pthread_mutex_unlock(&clients_mutex);
}



/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
		int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s has joined Twitter\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);
	}

	bzero(buff_out, BUFFER_SZ);

	while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0){
			if(strlen(buff_out) > 0){
				if(strstr(buff_out,follow_command)){
					char * token = "";
					int found = 0;
					char *newline = strchr( buff_out, '\n' );
					if ( newline )
  						*newline = 0;
					token = strtok(buff_out, " ");
					while ((token = strtok(NULL, " ")) != NULL){
						if (token[0]=='@'){
							found = 1;
							add_follower(token,cli);
						}
					}
					if (found == 0){
						printf("No @ was found\n");
					}
				}
				else if(strstr(buff_out,stats_command)){
					printf("NUMBER OF FOLLOWERS: %d\n",cli->num_followers);
				}
				else if(strstr(buff_out,send_command)){
				char *b = buff_out + strlen(send_command)+3;
				send_private_message_file(b, name);
				str_trim_lf(b, strlen(b));
				str_trim_lf(buff_out, strlen(buff_out));
				printf("%s -> %s\n", b, name);
				}
			}
		} else if (receive == 0 || strcmp(buff_out, "exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

  /* Delete client from queue and yield thread */
	close(cli->sockfd);
  queue_remove(cli->uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	if(argc != 3){
		printf("Usage: %s <ip><port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = argv[1];
	int port = atoi(argv[2]);
	int option = 1;
	int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  /* Socket settings */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  /* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
	}

	/* Bind */
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR: Socket binding failed");
    return EXIT_FAILURE;
  }

  /* Listen */
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
	}

	printf("\e[1;97;44m____________________________________\e[0m\n");
	printf("\e[1;97;44m Twitter Server                     \e[0m\n");
	printf("\e[1;97;44m____________________________________\e[0m\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;
		cli->num_followers = num_followers;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
