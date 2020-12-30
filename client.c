#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define h_addr h_addr_list[0]
#define BUFFSIZE 600
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define errno -1


int connect_socket( char * adresse, int port);
int str_comp( char * str1, char * str2);
int send_server(int sock, char *buffer);
int recv_server(int sock, char *buffer);


int main(int argc, char ** argv)
{
	// Verification du nombre d'arguments sur la ligne de commande
	if (argc != 4){
		printf("Nombre de paramètres incorrect!\nL'appel doit être de la forme: \t \"Le pseudo\" \t \"L'adresse du serveur\" \t \"Le port\" \n");
		exit (-1);
	}

	char * pseudo = argv[1];
	char * adresse = argv[2];
	int port = atoi(argv[3]);

	printf("Bonjour!\nLe principe du jeu consiste à deviner un mot. ");
	printf("Pour jouer, envoyez vos propositions sous forme de lettres: 'a' , 'b' , 'c', ...\n");
	int sock = connect_socket(adresse, port);
	send_server(sock, pseudo);

	fd_set readfds;
	int j;
	int continu = 1;
	while (continu){
 		FD_ZERO(&readfds); /* Vide l'ensemble readfs */
        FD_SET(STDIN_FILENO, &readfds); /* Ajout de STDIN_FILENO */
        FD_SET(sock, &readfds); /* Ajout du socket */
	char buffer[BUFFSIZE];
        memset(buffer, 0, sizeof(buffer) ); /* Initialisation du buffer qui sera utilise */

		int s = select(sock+1, &readfds, NULL, NULL, NULL);
		if (s == -1)
		{
			perror("select()");
			exit(-1);
		}
		if(FD_ISSET(STDIN_FILENO, &readfds) == 1)
		{
			do
			{
				fgets(buffer, BUFFSIZE, stdin);
				buffer[strlen(buffer)-1] = '\0';
			}while (strlen(buffer) != 1);

			send_server(sock, buffer);

			if( str_comp(buffer, "!"))
			{
				printf("Deconnexion.\n");
				continu = 0;
			}

			for(j = 0; j < BUFFSIZE; j++) buffer[j] = '\0';

		}else if(FD_ISSET(sock, &readfds) == 1)
		{
			char buffer[BUFFSIZE];
			int r = recv_server(sock, buffer);
			if(r == 0)
			{
				printf("Le serveur s'est déconnecté.\n");
				break;
			}
			printf("SERVEUR: %s\n", buffer);
			for(j = 0; j < BUFFSIZE; j++) buffer[j] = '\0';
		}
	}
	close(sock);
	return 0;
}


int connect_socket(char * adresse, int port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0); /* Creation d'un socket TCP */
	if( sock == INVALID_SOCKET )
	{
		perror("socket()");
		exit (-1);
	}

	struct hostent* hostinfo = gethostbyname(adresse); /* Infos du serveur */
	if( hostinfo == NULL )
	{
		perror("gethostname()");
		exit (-1);
	}

	struct sockaddr_in sin; /* Structure qui possede toutes les infos pour le socket */
	sin.sin_addr = *(struct in_addr*) hostinfo->h_addr; /* On spécifie l'addresse */
	sin.sin_port = htons(port); /* Le port */
	sin.sin_family = AF_INET; /* Le protocole (AF_INET pour IP) */

	int c = connect(sock, (struct sockaddr*) &sin, sizeof(struct sockaddr)); /* Demande de connection */
	if( c == -1 )
	{
		perror("connect()");
		exit (-1);
	}
	return sock;
}


int str_comp( char * str1, char * str2){
	int i = 0;
	while( (str1[i] == str2[i]) && (str2[i] != '\0') && (str1[i] != '\0') )
	{
		i++;
	}
	if((str2[i] == '\0') && (str1[i] == '\0')) return 1;
	else return 0;
}

int recv_server(int sock, char *buffer)
{
	int r = recv(sock, buffer, BUFFSIZE, 0);
	if( r == SOCKET_ERROR )
	{
		perror("recv()");
		exit (-1);
	}
	return r;
}


int send_server(int sock, char *buffer)
{
	int s = send(sock, buffer, strlen(buffer), 0);
	if( s == -1 )
	{
		perror("send()");
		exit (errno);
	}
	return s;
}









