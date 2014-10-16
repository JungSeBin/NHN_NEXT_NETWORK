#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/////
//#include <unistd.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
/////

#pragma comment ( lib, "Ws2_32.lib" )
#include <WinSock2.h>



#define PORT 3000
#define MAX_DATA 100

int main()
{
	int ret = -1;
	int serverSock;
	int acceptedSock;
	struct sockaddr_in Addr;
	char readBuf[MAX_DATA];
	int AddrSize = sizeof(Addr);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		goto error;
	}

	if ((serverSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		goto leave;
	}

	Addr.sin_family = AF_INET;
	Addr.sin_addr.s_addr = INADDR_ANY;
	Addr.sin_port = htons(PORT);
	if ((ret = bind(serverSock, (struct sockaddr *)&Addr, sizeof(Addr)))) {
		perror("bind");
		goto error;
	}

	if ((ret = listen(serverSock, 1))) {
		perror("listen");
		goto error;
	}
	acceptedSock = accept(serverSock, (struct sockaddr *)&Addr, &AddrSize);
	if (acceptedSock == -1) {
		perror("accept");
		ret = -1;
		goto error;
	}
	if ((ret = recv(acceptedSock, readBuf, MAX_DATA, 0)) <= 0) {
		perror("recv");
		ret = -1;
	}
	else
		printf("Read %d Bytes: '%s'\n", ret, readBuf);

	if ((ret = send(acceptedSock, readBuf, strlen(readBuf), 0)) <= 0) {
		perror("recv");
		ret = -1;
	}
	else
		printf("Wrote %d Bytes: '%s'\n", ret, readBuf);

	closesocket(acceptedSock);
error:
	closesocket(serverSock);
leave:
	getchar();
	return ret;
}