// SimpleWeb.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <fcntl.h>
#include <io.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable:4996)

/* Sample TCP server */


int main(int argc, char**argv)
{
	WSADATA wsa;
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 0;
	}

	int listenfd, connfd, n;
	struct sockaddr_in servaddr, cliaddr;
	int clilen;
	char mesg[4096];
	char header[] = "HTTP/1.0 200 OK\r\nDate: Wed, 12 Mar 2014 00:14:10 GMT\r\n\r\n";
	char buf[BUFSIZ];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(32000);
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(listenfd, 1024);

	for (int i = 0; i < 100; i++) {
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		printf("connected (%d)\n", connfd);

		if (connfd > 0) {
			n = recv(connfd, mesg, 4096, 0);
			mesg[n] = 0;
			printf("=====\n%s=====\n", mesg);
			send(connfd, header, strlen(header), 0);
			int fd = open("ex03.html", O_RDONLY);
			while ((n = read(fd, buf, BUFSIZ)) > 0) {
				send(connfd, buf, n, 0);
			}
			closesocket(fd);
			closesocket(connfd);
		}
	}
}

