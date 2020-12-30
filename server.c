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
#define NB_CLIENT 4
#define TAILLE_MOT 10
#define NAMELONG 50

int connect_client(int sock, unsigned int sinsize);
int recv_client(int csock, char *buffer);
int send_client(int csock, char *buffer);
char * Add_name( char * msg, int num_client, char ** pseudo);
int str_comp( char * str1, char * str2);
char * msg_deco( int num_client, char ** pseudo);
void deconnexion(int indice_client, char ** pseudo, int * clients, int nb_clients, int * essai);
void reset_buff( char * buffer, int buff_len);
char * copy_str( char * str );
int test(char * buffer, char * mot_cacher, char * mot_deviner);

int main(int argc, char ** argv)
{
	// Verification du nombre d'arguments sur la ligne de commande
	if(argc != 3)
	{
		
		printf("Nombre de paramètres incorrect!\nL'appel doit être de la forme: \t \"mot à deviner\" \t \"port du serveur\" \n");
		exit (-1);
	}

	char * mot_deviner = argv[1];
	int port = atoi( argv[2] );
	char mot_cacher[TAILLE_MOT];
	int reussi;
	int i,j;

	for (i = 0; i < strlen(mot_deviner); i++)
	{
		mot_cacher[i] = '*';
	}

	int sock = socket(AF_INET, SOCK_STREAM, 0); /* Création d'un socket TCP */
	if( sock == INVALID_SOCKET )
	{
		perror("socket()");
		exit (errno);
	}

	int * clients;
	clients = malloc( NB_CLIENT * sizeof(int));

	int * essai = calloc( NB_CLIENT * sizeof(int), 0);

	char ** pseudo;
	pseudo = malloc( NB_CLIENT * sizeof(char *));

	for(i = 0; i< NB_CLIENT; i++)
	{
		pseudo[i] = malloc( NAMELONG * sizeof(char));
	}

	struct sockaddr_in sin; /* structure qui possede toutes les informations pour le socket */
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;
	unsigned int sinsize = sizeof( sin );

	int b = bind( sock, (struct sockaddr*) &sin, sinsize);	/* Association de socket et des parametres reseaux du serveur */
	if( b == -1 ) /* Erreur lors de l'appel a bind */
	{
		perror("bind()");
		exit (-1);
	}

	fd_set readfds; /* Descripteur qui surveille pour voir si des donnees en lecture sont disponibles */
	int nb_clients = 0;
	int desc_max;

	while(1){
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		desc_max = sock;
		for(i = 0; i < nb_clients; i++){
			FD_SET(clients[i], &readfds); /* Remettre tous les elements dans readfds */
			if(clients[i] > desc_max) desc_max = clients[i];
		}
		select( desc_max + 1, &readfds, NULL, NULL, NULL);

		if(FD_ISSET(sock, &readfds))
		{
			clients[i] = connect_client(sock, sinsize);
			nb_clients++;
			recv_client(clients[nb_clients - 1], pseudo[nb_clients - 1]);
			printf("Le joueur %s s'est connecté.\n", pseudo[nb_clients - 1]);
			send_client(clients[i], mot_cacher);

		}
		else
		{
			for(i = 0; i < nb_clients; i++)
			{
				if(FD_ISSET( clients[i], &readfds))
				{
					char * msg;
					char buffer[BUFFSIZE];
					recv_client(clients[i], buffer);
					buffer[BUFFSIZE] = '\0';
					if (str_comp( buffer, "!"))
					{
						msg = msg_deco( i, pseudo);
						printf("%s\n", msg);
						deconnexion(i, pseudo, clients, nb_clients, essai);
						nb_clients--;
						for(j = 0; j < nb_clients; j++)
						{
 							send_client(clients[j], msg);
						}
					}

					else
					{

						if  ('A' <= buffer[0] && 'Z' >= buffer[0])
						{
							buffer[0] += 'a' - 'A';
						}

						if (('a' <= buffer[0] && 'z' >= buffer[0]) && essai[i] < 6)
						{
							reussi = test(buffer, mot_cacher, mot_deviner);
							essai[i] += reussi;
							for(j = 0; j < nb_clients; j++)
							{
	 							send_client(clients[j], mot_cacher);
	 							if (strcmp(mot_cacher, mot_deviner) == 0)
	 							{
	 								send_client(clients[j], "\nBravo ");
	 								send_client(clients[j], pseudo[i]);
	 								send_client(clients[j], "! Vous êtes le vainqueur.\n");

	 							}
	 							if (essai[i] == 6)
	 							{
	 								send_client(clients[j], "\nOups! ");
	 								send_client(clients[j], pseudo[i]);
	 								send_client(clients[j], " a épuisé ces coups.\n");
	 							}
							}
						}
						else
						{
							if (essai[i] >= 6)
							{
								send_client(clients[i], "Vous avez épuisé vos 6 tentatives\n");
							}
							else
							{
								send_client(clients[i], "Le caractère envoyé n'est pas une lettre.\n");
							}
						}
						reset_buff(buffer, BUFFSIZE);
					}


				}
			}
		}
	}
	close(sock);
	return 0;
}

int connect_client(int sock, unsigned int sinsize)
{
	struct sockaddr_in csin;
	int l = listen(sock, 1);

	if(l ==-1)
	{
		perror("listen()");
		exit (-1);
	}

	int c = accept(sock, (struct sockaddr *)&csin, &sinsize); /* Accepter un client */
	if( c == -1)
	{
		perror("accept()");
		exit (-1);
	}

	return c;
}

int recv_client(int csock, char *buffer)
{
	int r = recv(csock, buffer, BUFFSIZE, 0);
	if( r == SOCKET_ERROR )
	{
		perror("recv()");
		exit (-1);
	}
	return r;
}


int send_client(int csock, char *buffer)
{
	int s = send(csock, buffer, strlen(buffer), 0);
	if( s == -1 )
	{
		perror("send()");
		exit (errno);
	}
	return s;
}


char * Add_name( char * msg, int num_client, char ** pseudo)
{
	char * new_msg;
	int i = 0;
	int j = 0;

	new_msg = (char *)malloc( sizeof(char) * (BUFFSIZE + NAMELONG + 3));

	while( pseudo[num_client][i] != '\0' )
	{
		new_msg[i] = pseudo[num_client][i];
		i++;
	}

	new_msg[i] = ' ';
	new_msg[i + 1] = ':';
	new_msg[i + 2] = ' ';
	j = i + 3;

	for(i = 0; i < BUFFSIZE; i++)
	{
		new_msg[j] = msg[i];
		j++;
	}
	new_msg[j] = '\0';
	return new_msg;
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

char * msg_deco( int num_client, char ** pseudo)
{
	int j,i;
	char * new_msg;
	char debut[23] = "Deconnexion du joueur: ";

	new_msg = (char*)malloc( sizeof(char) * (NAMELONG + 23));

	for(i = 0; i < 23; i++) new_msg[i] = debut[i];

	j = 23;
	for( i = 0; i < NAMELONG; i++)
	{
		new_msg[j] = pseudo[num_client][i];
		j++;
	}
	return new_msg;
}

void deconnexion(int indice_client, char ** pseudo, int * clients, int nb_clients, int * essai)
{
	int i;
	int j = 0;
	reset_buff(pseudo[indice_client], NAMELONG);
	close(clients[indice_client]);

	for(i = indice_client; i < nb_clients - 1; i++)
	{
		clients[i] = clients[i + 1];
		essai[i] = essai[i + 1];

		reset_buff(pseudo[i], NAMELONG);
		while (pseudo[i + 1][j] != '\0' && j < NAMELONG - 1)
		{
			pseudo[i][j] = pseudo[i + 1][j];
			j++;
		}
		pseudo[i][j] = '\0';
	}
}

void reset_buff( char * buffer, int buff_len)
{
	int i;
	for(i = 0; i < buff_len; i++)
	{
		buffer[i] = '\0';
	}
}


char * copy_str( char * str )
{
	int  taille = 0;
	int i;
	char * str_copy;
	while( str[taille] != '\0' ) taille++;

	str_copy = malloc( sizeof( char ) * taille );
	for(i = 0; i < taille; i++)
	{
		str_copy[i] = str[i];
	}
	return str_copy;
}

int test(char * buffer, char * mot_cacher, char * mot_deviner)
{
	int cmp = 1;
	for (int i = 0; i < strlen(mot_deviner); i++)
	{
		if (buffer[0] == mot_deviner[i])
		{
			mot_cacher[i] = buffer[0];
			cmp = 0;
		}
	}
	return cmp;
}





