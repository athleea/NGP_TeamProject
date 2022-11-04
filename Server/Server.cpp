#define _CRT_SECURE_NO_WARNINGS // ���� C �Լ� ��� �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // ���� ���� API ��� �� ��� ����

#include <winsock2.h> // ����2 ���� ���
#include <ws2tcpip.h> // ����2 Ȯ�� ���

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#pragma comment(lib, "ws2_32") // ws2_32.lib ��ũ

#define MAX_PLAYER 3
#define BUFSIZE 512
#define SERVERPORT 9000

int cnt = 0;
int clientNum;
FILE* fp;
bool allClientReady;
bool restart[MAX_PLAYER] = { 1,1,1 };

HANDLE gameStartEvent;
CRITICAL_SECTION cs;

struct PACKET {
	char type;
};

struct PlayerInfo {
	COORD pos;
	int hp;
	char characterCode;
	bool jump;
	short CharacterCollsion;
	short MapCollision;
	int MonsterCollsion;
};
PlayerInfo players[MAX_PLAYER];

DWORD WINAPI Recv_Thread(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	char buf[BUFSIZE];
	PACKET keyEvent;
	EnterCriticalSection(&cs);
	int mycode = clientNum;
	LeaveCriticalSection(&cs);

	while (1) {
		WaitForSingleObject(gameStartEvent, INFINITE);

		// Ű�Է� Recv
		retval = recv(client_sock, (char*)buf, sizeof(PACKET), MSG_WAITALL);
		if (retval == SOCKET_ERROR) return 0;
		else if (retval == 0) {
			ResetEvent(gameStartEvent);
			break;
		}

		// Ű �Է� ó��



	}

	return 0;
}
DWORD WINAPI Collsion_Send_Thread(LPVOID arg) {

	char restart = 1;
	int retval;
	char buf[BUFSIZE];
	SOCKET client_sock = (SOCKET)arg;

	while (1) {
		while (clientNum != 3) Sleep(3000);

		// ĳ���� �ڵ� ����

		// Send Init Data


		SetEvent(gameStartEvent);
		while (1) { // CheckGameEnd()

			//collsion


			//send
		}
		ResetEvent(gameStartEvent);

		//����� ���� �۽�
		retval = send(client_sock, (char*)buf, sizeof(BUFSIZE), 0);
		if (retval == SOCKET_ERROR) break;

		//����� ���� ����
		retval = recv(client_sock, (char*)restart, sizeof(restart), MSG_WAITALL);
		if (retval == SOCKET_ERROR) break;

		if (restart == 0) break;
	}

	EnterCriticalSection(&cs);
	clientNum--;
	LeaveCriticalSection(&cs);
}


int main() {
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET); //err_quit("socket()");

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR);// err_quit("bind()");

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR);// err_quit("listen()");


	HANDLE hThread;

	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	gameStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	InitializeCriticalSection(&cs);
	while (1) {
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			//err_display("accept()");
			break;
		}

		if (clientNum < 3) {
			hThread = CreateThread(NULL, 0, Recv_Thread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			hThread = CreateThread(NULL, 0, Collsion_Send_Thread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			EnterCriticalSection(&cs);
			clientNum++;
			LeaveCriticalSection(&cs);
		}
		else {
			closesocket(client_sock);
		}
	}
	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	WSACleanup();

	return 0;
}