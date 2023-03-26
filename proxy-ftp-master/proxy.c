//! |=================================================|
//! |   ANTUNES Mathieu et FERREIRA Iannis GROUPE C1  |
//! |=================================================|

#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port

//ICI TOUT LE PROTOCOLE
void fils();

int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockCOM;                 // Descripteur de socket de communication
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les
				                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par
				                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN,
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

	len = sizeof(struct sockaddr_storage);
    
    // Attente connexion du client
    // Lorsque demande de connexion, creation d'une socket de communication avec le client 
    while (true) {
        int err;
        pid_t pid;
        int rapport, numSig, status;
        
        descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
        if (descSockCOM == -1){
            perror("Erreur accept\n");
            exit(6);
        }

        // Creation d'un processus fils
        pid = fork();
        switch (pid) {
            case -1:
                perror("Impossible de creer le fils 1");
                exit(1);

            case 0:
                fils(descSockCOM);
                exit(0);
        }
    }
}

void fils(int descSockCOM) {
    int descSockSERV;                // Descripteur de socket du serveur
    int descSockPASVC;               // Descripteur de socket du mode passive CLIENT
    int descSockPASVS;               // Descripteur de socket du mode passive SERVEUR
    struct addrinfo hintsPasv;       // Contrôle la fonction getaddrinfo pour le mode passif
    socklen_t lenR;                  // variable utilisée pour stocker la longueur des structures de socket en READ
    socklen_t lenW;                  // variable utilisée pour stocker la longueur des structures de socket en WRITE
    int ecode;
    char buffer[MAXBUFFERLEN], nomUser[MAXBUFFERLEN/2-1], nomServeur[MAXBUFFERLEN/2-1], port[] = "21"; 

    //Premier envoi du 220 au client pour recuperer user et adresse serveur
    strcpy(buffer, "220 Connexion socket acceptée, identifiez vous avec user@server\r\n");
    write(descSockCOM, buffer, strlen(buffer));
    memset(buffer, 0, MAXBUFFERLEN);

    //Lecture des données de l'utilisateur apres le premier 220
    lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture des données de l'utilisateur");
        exit(-1);
    } else {
        buffer[lenR] = '\0';
        sscanf(buffer, "%[^@]@%s",nomUser, nomServeur);
        printf("\nLe login: %s\nLe serveur: %s\n\n", nomUser, nomServeur);
        strcat(nomUser,"\r\n");
    }

//! |=========================|
//! |   CONNEXION AU SERVEUR  |
//! |=========================|
    ecode = connect2Server(nomServeur, port, &descSockSERV);
    if(ecode < 0) {
        perror("Erreur de connexion");
        exit(-1);
    } else {
        printf("Connecté !\n\n");
    }


//! |=====================|
//! |   AUTHENTIFICATION  |
//! |=====================|
    //Lecture des données renvoyées par le serveur apres connexion (220 USER)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }

    //Envoi du nomUser dans le serveur apres le second 220
    memset(buffer, 0, MAXBUFFERLEN);
    lenW = write(descSockSERV, nomUser, strlen(nomUser));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //Lecture des données renvoyées par le serveur apres connexion (331 PASS)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }
    
    //Ecrire les données du serveur dans l'interface utilisateur (331 PASS)
    lenW = write(descSockCOM, buffer, strlen(buffer));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //Lecture des données de l'interface utilisateur (331 PASS)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }

    //Ecriture du PASS sur le serveur
    lenW = write(descSockSERV, buffer, strlen(buffer));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //Lecture des données renvoyées par le serveur apres connexion (230 login successful)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }

    //Ecrire les données du serveur dans l'interface utilisateur (230 login successful)
    lenW = write(descSockCOM, buffer, strlen(buffer));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //Lire les données de l'interface utilisateur (SYST)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }

    //Ecrire les données sur le serveur (SYST)
    lenW = write(descSockSERV, buffer, strlen(buffer));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //Lecture des données renvoyées par le serveur (215 SYST)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }

    //Ecrire les données du serveur dans l'interface utilisateur (215 SYST)
    lenW = write(descSockCOM, buffer, strlen(buffer));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //PREMIERE INTERROGATION DE COMMANDE AVANT DEPART DE BOUCLE
    //Lire les données de l'interface utilisateur
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }
    printf("Client, lecture: %s\n",buffer);



//! |===============================================================|
//! |   DEPART DE LA BOUCLE D'ECHANGES COMMANDES CLIENT -> SERVEUR  |
//! |===============================================================|
    while(strstr(buffer, "QUIT") == NULL) {    
        //! |================================================|
        //! |   CAS OU LA COMMANDE NECESSITE LE MODE PASSIF  |
        //! |================================================|
        //Un while, car si un if, si on refait une commande declenchant PORT juste apres, alors
        //on execute le code de boucle classique qui bloque le reste, donc un while.
        //commandes ls,get,put (si on en a les droits, sinon message d'erreur 550 operation not permitted)
        while(strstr(buffer, "PORT") != NULL) {
            int u1, u2, u3, u4, u5, u6;
            char adressePasvC[25], portPasvC[10];
            //assigner valeurs aux variables
            sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &u1, &u2, &u3, &u4, &u5, &u6);
            //adresse client
            sprintf(adressePasvC, "%d.%d.%d.%d", u1, u2, u3, u4);
            //port client
            sprintf(portPasvC, "%d", u5 * 256 + u6);
            printf("PASV Client, Adresse: %s Port: %s\n", adressePasvC, portPasvC);
            
            memset( &hintsPasv, 0, sizeof(hintsPasv));
            //Hints
            hintsPasv.ai_socktype = SOCK_STREAM;
            hintsPasv.ai_family = AF_INET;

            ecode = connect2Server(adressePasvC, portPasvC, &descSockPASVC);
            if(ecode < 0) {
                perror("Erreur de connexion");
                exit(-1);
            } else {
                printf("\nConnecté en mode passif client !\n\n");
            }

            //Ecriture du mode passif sur le serveur
            lenW = write(descSockSERV, "PASV\r\n", strlen("PASV\r\n"));
            if (lenW < 0) {
                perror("Erreur d'écriture");
                exit(-1);
            }

            //Lecture des données renvoyées par le serveur (227 PASSIVE)
            memset(buffer, 0, MAXBUFFERLEN);
            lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
            if(lenR < 0) {
                perror("Erreur de lecture");
                exit(-1);
            }

            u1, u2, u3, u4, u5, u6 = 0;
            char adressePasvS[25],portPasvS[10];
            //assigner valeurs aux variables
            sscanf(buffer, "%*[^(](%d,%d,%d,%d,%d,%d", &u1, &u2, &u3, &u4, &u5, &u6);
            //adresse serveur
            sprintf(adressePasvS, "%d.%d.%d.%d", u1, u2, u3, u4);
            //port serveur
            sprintf(portPasvS, "%d", u5 * 256 + u6);
            printf("PASV Serveur, Adresse: %s Port: %s\n", adressePasvS, portPasvS);

            ecode = connect2Server(adressePasvS, portPasvS, &descSockPASVS);
            if(ecode < 0) {
                perror("Erreur de connexion");
                exit(-1);
            } else {
                printf("\nConnecté en mode passif serveur !\n\n");
            }

            //Ecrire dans l'interface utilisateur que la connection en passif est ok :)(200 PORT connection )
            lenW = write(descSockCOM, "200 PORT command successful.\r\n", strlen("200 PORT passive conection OK\r\n"));
            if (lenW < 0) {
                perror("Erreur d'écriture");
                exit(-1);
            }

            //Lire les données de l'interface utilisateur (la commande de base)
            memset(buffer, 0, MAXBUFFERLEN);
            lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
            if(lenR < 0) {
                perror("Erreur de lecture");
                exit(-1);
            }

            //Ecriture de la commande sur le serveur
            lenW = write(descSockSERV, buffer, strlen(buffer));
            if (lenW < 0) {
                perror("Erreur d'écriture");
                exit(-1);
            }

            //Lecture des données renvoyées par le serveur (150)
            memset(buffer, 0, MAXBUFFERLEN);
            lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
            if(lenR < 0) {
                perror("Erreur de lecture");
                exit(-1);
            }

            //Ecrire les données du serveur dans l'interface utilisateur (150)
            lenW = write(descSockCOM, buffer, strlen(buffer));
            if (lenW < 0) {
                perror("Erreur d'écriture");
                exit(-1);
            }

            //! |=========================|
            //! |   TRANSFERT DE DONNEES  |
            //! |=========================|
            
            //Lecture du serveur passif
            memset(buffer, 0, MAXBUFFERLEN);
            lenR = read(descSockPASVS, buffer, MAXBUFFERLEN-1);
            if(lenR < 0) {
                perror("Erreur de lecture PASV Serveur");
                exit(-1);
            }

            //Pour rentrer dans la boucle la premiere fois on change la valeur de ecode mais pas a 0
            ecode = 1;
            while (ecode != 0) {
                ecode = write(descSockPASVC, buffer, strlen(buffer));
                memset(buffer, 0, MAXBUFFERLEN);
                ecode = read(descSockPASVS, buffer, MAXBUFFERLEN - 1);
                if (ecode < 0) {
                    perror("Erreur lecture pasv serveur");
                    exit(-1);
                }
            }

            //Lecture des données renvoyées par le serveur 
            memset(buffer, 0, MAXBUFFERLEN);
            lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
            if(lenR < 0) {
                perror("Erreur de lecture");
                exit(-1);
            }

            //Fermeture des sockets de connexion en mode passif
            close(descSockPASVC);
            close(descSockPASVS);
            
            //Ecrire les données du serveur dans l'interface utilisateur (Données + 226)
            lenW = write(descSockCOM, buffer, strlen(buffer));
            if (lenW < 0) {
                perror("Erreur d'écriture");
                exit(-1);
            }
            
            //PREMIERE INTERROGATION DE COMMANDE AVANT DEPART DE BOUCLE PASV
            //Lire les données de l'interface utilisateur
            memset(buffer, 0, MAXBUFFERLEN);
            lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
            if(lenR < 0) {
                perror("Erreur de lecture");
                exit(-1);
            }
            printf("Client, lecture: %s\n",buffer);
        }

        //! |=====================|
        //! |   BOUCLE CLASSIQUE  |
        //! |=====================|
        //Ecrire les données de l'interface utilisateur dans le serveur (données récupérées soit en dehors de la boucle
        //la première fois, soit a la fin de la boucle pour éviter les décalages.)
        //Client -> Serveur, ecriture
        lenW = write(descSockSERV, buffer, strlen(buffer));
        if (lenW < 0) {
            perror("Erreur d'écriture");
            exit(-1);
        }
        
        //Lecture des données renvoyées par le serveur 
        //Serveur, lecture
        memset(buffer, 0, MAXBUFFERLEN);
        lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
        if(lenR < 0) {
            perror("Erreur de lecture");
            exit(-1);
        }

        //Ecrire les données du serveur dans l'interface utilisateur
        //Serveur -> Client, ecriture
        lenW = write(descSockCOM, buffer, strlen(buffer));
        if (lenW < 0) {
            perror("Erreur d'écriture");
            exit(-1);
        }

        //Lire les données de l'interface utilisateur
        //Client, lecture
        memset(buffer, 0, MAXBUFFERLEN);
        lenR = read(descSockCOM, buffer, MAXBUFFERLEN-1);
        if(lenR < 0) {
            perror("Erreur de lecture");
            exit(-1);
        } 

    }

    //! |====================|
    //! |   QUIT ET GOODBYE  |
    //! |====================|
    //Lecture des données renvoyées par le serveur (221 GOODBYE)
    memset(buffer, 0, MAXBUFFERLEN);
    lenR = read(descSockSERV, buffer, MAXBUFFERLEN-1);
    if(lenR < 0) {
        perror("Erreur de lecture");
        exit(-1);
    }
    //Ecrire les données du serveur dans l'interface utilisateur (221 GOODBYE)
    lenW = write(descSockCOM, buffer, strlen(buffer));
    if (lenW < 0) {
        perror("Erreur d'écriture");
        exit(-1);
    }

    //Fermeture du socket de connexion utilisateur
    close(descSockCOM);
}
