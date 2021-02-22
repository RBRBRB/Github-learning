#include <arpa/inet.h>
#include <sys/socket.h>
//#include <linux/netlink.h>
#include <stdio.h>
#include "esp_wifi.h"

#define EXAMPLE_SOCK_SER_IP        CONFIG_SOCKET_DEST_IP
#define SOCK_PORT 8888
#define SOCK_PORT_CK 8889


int ser_sock;
struct sockaddr_in client;
struct sockaddr_in server;



void init_socket_clinet( void )
{
    char *ser_ip = EXAMPLE_SOCK_SER_IP;
    ser_sock = socket(AF_INET , SOCK_DGRAM , 0);
    if (ser_sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //bzero((char *)&client, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = htons(0);


    //server.sin_addr.s_addr = inet_addr("140.96.101.94");
    server.sin_family = AF_INET;
    server.sin_port = htons( SOCK_PORT );
        printf("%s\n",ser_ip);

        if(inet_pton(AF_INET, ser_ip , &server.sin_addr)<=0)
        {
                printf("\n inet_pton error occured\n");
                exit(1);
                return;
        }

        if ( bind(ser_sock, (struct sockaddr*)&client, sizeof(client) ) <0 ) {
                perror("bind failed!");
                exit(1);
        }
 
}


int ser_sock_ck;
struct sockaddr_in client_ck;
struct sockaddr_in server_ck;


void init_socket_clinet_ck( void )
{
    char *ser_ip = EXAMPLE_SOCK_SER_IP;
    ser_sock_ck = socket(AF_INET , SOCK_DGRAM , 0);
    if (ser_sock_ck == -1)
    {
        printf("Could not create socket");
    }
    //puts("Socket created");

    //bzero((char *)&client, sizeof(client));
    client_ck.sin_family = AF_INET;
    client_ck.sin_addr.s_addr = htonl(INADDR_ANY);
    client_ck.sin_port = htons(0);


    //server.sin_addr.s_addr = inet_addr("140.96.101.94");
    server_ck.sin_family = AF_INET;
    server_ck.sin_port = htons( SOCK_PORT_CK );
        printf("%s\n",ser_ip);

        if(inet_pton(AF_INET, ser_ip , &server_ck.sin_addr)<=0)
        {
                printf("\n inet_pton error occured\n");
                exit(1);
                return;
        }

        if ( bind(ser_sock_ck, (struct sockaddr*)&client_ck, sizeof(client_ck) ) <0 ) {
                perror("bind failed!");
                exit(1);
        }
 
}

