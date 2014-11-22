#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <WinSock2.h>
#include <windows.h>

#define BUF_SIZE 100
#define MAX_CLIENT 100
#define RECV 3
#define SEND 5

#pragma comment(lib,"ws2_32.lib")

typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAddr;
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct //buffer info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rsMode; //Recv or Send Mode
}PER_IO_DATA, *LPPER_IO_DATA;

DWORD WINAPI ChatThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char* message);

SOCKET* hClientList;
unsigned int clientNum = 0;

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAddr;
	DWORD recvBytes, flags = 0;

	hClientList = (SOCKET*)malloc(sizeof(SOCKET)*MAX_CLIENT);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		ErrorHandling("WSAStartup() Error!!");
	}

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);

	for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
	{
		_beginthreadex(NULL, 0, ChatThreadMain, (LPVOID)hComPort, 0, NULL);
	}

	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(atoi(argv[1]));

	bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	listen(hServSock, 5);

	while (1)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAddr;
		int addrLen = sizeof(clntAddr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &addrLen);
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAddr), &clntAddr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD32)handleInfo, 0);

		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rsMode = RECV;
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf),
			1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
		hClientList[clientNum++] = hClntSock;
	}
	return 0;
}

DWORD WINAPI ChatThreadMain(LPVOID pComPort)
{
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;
	int sendNum = 0;

	while (1)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans,
			(DWORD*)&handleInfo, (OVERLAPPED**)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (ioInfo->rsMode == RECV)
		{
			puts("Message received!!");
			if (bytesTrans == 0) //EOF를 받았을 시
			{
				closesocket(sock);
				free(handleInfo);
				free(ioInfo);
				continue;
			}

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rsMode = SEND;
			// 연결되어있는 모든 client에 BroadCast
			for (unsigned int i = 0; i < clientNum;++i)
			{
				WSASend(hClientList[i], &(ioInfo->wsaBuf),
					1, NULL, 0, &(ioInfo->overlapped), NULL);
				sendNum++;
			}

			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rsMode = RECV;
			WSARecv(sock, &(ioInfo->wsaBuf),
				1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			puts("Message sent!!");
			if (sendNum == clientNum) // 마지막 client까지 메세지를 보냈을 때 free
			{
				free(ioInfo);
			}
		}
	}
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}