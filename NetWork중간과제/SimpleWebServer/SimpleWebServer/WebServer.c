#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

#define BUF_SIZE 2048
#define BUF_SMALL 100

#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")

typedef enum Error
{
	ERROR_NONE,
	ERROR_400_BAD_REQUEST,
	ERROR_404_NOT_FOUND,
	ERROR_LAST,
}ErrorType;

unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* fileName);
void SendErrorMsg(SOCKET sock, ErrorType errorType);
void ErrorHandling(char* message);

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAddr, clntAddr;

	HANDLE hThread;
	DWORD dwThreadID;
	int clntAddrSize;

	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() error!!");
	}

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	{
		ErrorHandling("bing() error");
	}
	if (listen(hServSock, 5) == SOCKET_ERROR)
	{
		ErrorHandling("listen() error");
	}

	while (1)
	{
		clntAddrSize = sizeof(clntAddr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &clntAddrSize);
		printf("Connection Request: %s: %d\n",
			inet_ntoa(clntAddr.sin_addr), ntohs(clntAddr.sin_port));
		hThread = (HANDLE)_beginthreadex(
			NULL, 0, RequestHandler, (void*)hClntSock, 0, (unsigned*)&dwThreadID);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			ErrorHandling("_beginthreadex() error!!");
		}
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned WINAPI RequestHandler(void* arg)
{
	SOCKET hClntSock = (SOCKET)arg;
	char buf[BUF_SIZE] = { 0, };
	char method[BUF_SMALL] = { 0, };
	char ct[BUF_SMALL] = { 0, };
	char fileName[BUF_SMALL] = { 0, };

	if (recv(hClntSock, buf, BUF_SIZE, 0) == SOCKET_ERROR)
	{
		ErrorHandling("recv() error!!");
	}

	if (strstr(buf, "HTTP/") == NULL)
	{
		SendErrorMsg(hClntSock, ERROR_400_BAD_REQUEST);
		closesocket(hClntSock);
		return 1;
	}

	strcpy(method, strtok(buf, " "));
	if (strcmp(method, "GET"))
	{
		SendErrorMsg(hClntSock, ERROR_400_BAD_REQUEST);
	}

	strcpy(fileName, strtok(NULL, " "));
	memmove(fileName + 1, fileName, strlen(fileName));
	fileName[0] = '.';
	for (unsigned int i = 0; i < strlen(fileName) + 1; ++i)
	{
		if (fileName[i] == '/')
		{
			fileName[i] = '\\';
		}
	}

	char* type = ContentType(fileName);
	if (type == NULL)
	{
		SendErrorMsg(hClntSock,ERROR_404_NOT_FOUND);
		closesocket(hClntSock);
		return 0;
	}
	strcpy(ct, ContentType(fileName));
	SendData(hClntSock, ct, fileName);
	return 0;
}

void SendData(SOCKET sock, char* ct, char* fileName)
{
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char servName[] = "Server:simple web server\r\n";
	char cntLen[BUF_SMALL] = { 0, };
	char cntType[BUF_SMALL] = { 0, };
	char buf[BUF_SIZE] = { 0, };
	FILE* sendFile;

	sprintf(cntType, "Content-type:%s\r\n\r\n", ct);
	if ((sendFile = fopen(fileName, "rb")) == NULL)
	{
		SendErrorMsg(sock, ERROR_404_NOT_FOUND);
		return;
	}

	int fileSize;
	fseek(sendFile, 0, SEEK_END);
	fileSize = ftell(sendFile);
	fseek(sendFile, 0, SEEK_SET);

	sprintf(cntLen, "Content-length: %d\r\n", fileSize);
	if (send(sock, protocol, strlen(protocol), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send(): protocol error!!");
	}
	if (send(sock, servName, strlen(protocol), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send(): servName error!!");
	}
	if (send(sock, cntType, strlen(protocol), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send(): cntType error!!");
	}

	while (!feof(sendFile))
	{
		int read = fread(buf, 1, BUF_SIZE, sendFile);
		send(sock, buf, read, 0);
	}
	closesocket(sock);
	fclose(sendFile);
}

void SendErrorMsg(SOCKET sock, ErrorType errorType)
{
	char protocol[BUF_SMALL] = { 0, };
	char cntLen[BUF_SMALL] = { 0, };
	char content[BUF_SIZE] = { 0, };
	char errorMessage[BUF_SMALL] = { 0, };
	switch (errorType)
	{
	case ERROR_400_BAD_REQUEST:
		strcpy(errorMessage, "400 Bad Request\r\n");
		break;
	case ERROR_404_NOT_FOUND:
		strcpy(errorMessage, "404 Not Found\r\n");
		break;
	default:
		strcpy(errorMessage, "400 Bad Request\r\n");
		break;
	}

	char servName[] = "Server : simple web server\r\n";
	char cntType[] = "Content-type:text/html\r\n\r\n";
	sprintf(protocol, "HTTP/1.0 %s", errorMessage);
	sprintf(content, "<html><head><title>NETWORK</title></head>"
		"<body><font size=+2><br>%s"
		"</font></body></html>", errorMessage);
	sprintf(cntLen, "Content-length : %d\r\n", strlen(content));
	if (send(sock, protocol, strlen(protocol), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send():protocol error!");
	}
	if (send(sock, servName, strlen(servName), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send():cntType error!");
	}
	if (send(sock, cntLen, strlen(cntLen), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send():cntLen error!");
	}
	if (send(sock, cntType, strlen(cntType), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send():cntType error!");
	}
	if (send(sock, content, strlen(content), 0) == SOCKET_ERROR)
	{
		ErrorHandling("send():content error!");
	}
	closesocket(sock);
}

char* ContentType(char* file)
{
	char extension[BUF_SMALL] = { 0, };
	char fileName[BUF_SMALL] = { 0, };
	char* ret;
	strcpy(fileName, file);
	strtok(fileName, ".");
	ret = strtok(NULL, ".");
	if (ret == NULL)
		return ret;
	strcpy(extension, ret);
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
	{
		return "text/html";
	}
	else
	{
		return "text/plain";
	}
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}