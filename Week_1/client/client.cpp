#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/////
//#include <unistd.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
/////


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment ( lib, "Ws2_32.lib" )
#include <WinSock2.h>



#define IP "127.0.0.1"
#define PORT 3000
#define WRITE_DATA "Hello World!"
#define MAX_DATA 100

int main()
{
	int ret = -1;
	int clientSock;
	char buf[MAX_DATA] = { NULL, };
	struct sockaddr_in serverAddr;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		goto error;
	}

	if ((clientSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		goto leave;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(IP);
	serverAddr.sin_port = htons(PORT);

	if ((ret = connect(clientSock, (struct sockaddr*)&serverAddr,
		sizeof(serverAddr)))) {
		perror("connect");
		goto error;
	}

	if ((ret = send(clientSock, WRITE_DATA, sizeof(WRITE_DATA), 0)) <= 0) {
		perror("send");
		ret = -1;
	}
	else
		printf("Wrote '%s' (%d Bytes)\n", WRITE_DATA, ret);

	if ((ret = recv(clientSock, buf, MAX_DATA, 0)) <= 0) {
		perror("send");
		ret = -1;
	}
	else
		printf("Read '%s' (%d Bytes)\n", buf, ret);


error:
	closesocket(clientSock);
leave:
	getchar();
	return ret;
}