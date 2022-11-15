#include "global.h"
#include "Protocol.h"

int cnt = 0;
int ClientNum = 0;
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

void SendMapFile()
{
	
}

DWORD WINAPI Recv_Thread(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	char buf[BUFSIZE];
	PACKET keyEvent;

	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;

	EnterCriticalSection(&cs);
	int mycode = ClientNum;
	LeaveCriticalSection(&cs);

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	FILE* ff;
	ff = fopen("mappos.txt", "rb");
	if (!ff) {
		err_display("fopen()");
		exit(1);
	}
	else
		fseek(ff, 0, SEEK_END);

	long long f_size;
	f_size = ftell(ff);

	retval = send(client_sock, (char*)&f_size, sizeof(f_size), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
	}

	fseek(ff, 0, SEEK_SET);
	if (ff == NULL)
		printf("File not exist");
	else {
		memset(buf, 0, BUFSIZE);

		while (fread(buf, sizeof(char), BUFSIZE, ff) > 0) {
			retval = send(client_sock, buf, sizeof(buf), 0);
			if (retval == SOCKET_ERROR) {
				printf("Send Failed\n");
				exit(1);
			}
			memset(buf, 0, BUFSIZE);
		}
		fclose(ff);
		printf("File - %s - Send complete\n", "mappos.txt");
	}
	SendMapFile();

	while (1) {
		WaitForSingleObject(gameStartEvent, INFINITE);

		// 키입력 Recv
		retval = recv(client_sock, (char*)buf, sizeof(PACKET), MSG_WAITALL);
		if (retval == SOCKET_ERROR) return 0;
		else if (retval == 0) {
			ResetEvent(gameStartEvent);
			break;
		}

		// 키 입력 처리



	}

	return 0;
}

DWORD WINAPI Collsion_Send_Thread(LPVOID arg) 
{

	char restart = 1;
	int retval;
	char buf[BUFSIZE];
	SOCKET client_sock = (SOCKET)arg;

	while (1) {
		while (ClientNum != 3) Sleep(3000);

		// 캐릭터 코드 설정

		// Send Init Data


		SetEvent(gameStartEvent);
		while (1) { // CheckGameEnd()

			//collsion


			//send
		}
		ResetEvent(gameStartEvent);

		//재시작 여부 송신
		retval = send(client_sock, (char*)buf, sizeof(BUFSIZE), 0);
		if (retval == SOCKET_ERROR) break;

		//재시작 여부 수신
		retval = recv(client_sock, (char*)restart, sizeof(restart), MSG_WAITALL);
		if (retval == SOCKET_ERROR) break;

		if (restart == 0) break;
	}

	EnterCriticalSection(&cs);
	ClientNum--;
	LeaveCriticalSection(&cs);
}

int RandomCharacter() 
{
	int temp{};
	static bool characternum[3]{ false, false, false }; 
	srand(time(NULL));

	do { 
		temp = rand() % 3; 
	} while (characternum[temp]);
	characternum[temp] = true;

	return temp + 1;
}


int main() 
{
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

		if (ClientNum < 3) {
			hThread = CreateThread(NULL, 0, Recv_Thread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			hThread = CreateThread(NULL, 0, Collsion_Send_Thread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			EnterCriticalSection(&cs);
			ClientNum++;
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