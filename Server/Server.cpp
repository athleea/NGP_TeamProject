#include "global.h"
#include "Protocol.h"
#include <fstream>
#include <vector>
#include <iostream>
using namespace std;

int clientCount = 0;
FILE* fp;

HANDLE gameStartEvent, hFileEvent;
CRITICAL_SECTION cs;
SOCKET sockManager[MAX_PLAYER];

int ready = 0;
bool gameover = false;
int Monster_X[3] = { 0 };
int MonsterTurn[3] = { 0 };

class BLOCK {
public:
	int x;
	int y;
	int width;

	void read(ifstream& in) {
		int a, b, w;
		in >> a;
		in >> b;
		in >> w;

		x = a;
		y = b;
		width = w;
	}
};

BLOCK Block_local[28];

struct PlayerInfo {
	bool keyPress_D, keyPress_A;
	BYTE left, right;
	COORD pos;
	COORD charPos;
	BYTE hp;
	BYTE characterCode;
	bool jump;
	BYTE jumpCount;
};

PlayerInfo players[MAX_PLAYER];

void InitPlayer(int player_code);
void InitPlayer(int player_code)
{
	if (player_code == 0)
		players[player_code].pos = { 30, 620 };
	else if (player_code == 1)
		players[player_code].pos = { 230, 620 };
	else if (player_code == 2)
		players[player_code].pos = { 430, 620 };

	players[player_code].charPos = { 0, 0 };
	players[player_code].characterCode = player_code;
	players[player_code].left = 0;
	players[player_code].right = 0;
	players[player_code].hp = 3;
	players[player_code].keyPress_A = false;
	players[player_code].keyPress_D = false;
	players[player_code].jump = false;
	players[player_code].jumpCount = 0;
}

BYTE RandomCharacter();
BYTE RandomCharacter()
{
	BYTE temp{};
	static bool characternum[3]{ false, false, false };
	srand(time(NULL));

	do {
		temp = rand() % 3;
	} while (characternum[temp]);
	characternum[temp] = true;

	return temp;
}

void ProcessPacket(BYTE msg, BYTE player_code)
{
	EnterCriticalSection(&cs);
	switch (msg) {
	case CS_KEYDOWN_W:
		break;
	case CS_KEYDOWN_A:
		players[player_code].keyPress_A = true;
		break;
	case CS_KEYDOWN_D:
		players[player_code].keyPress_D = true;
		break;
	case CS_KEYDOWN_SPACE:
		players[player_code].jump = true;
		break;
	case CS_KEYUP_A:
		players[player_code].keyPress_A = false;
		break;
	case CS_KEYUP_D:
		players[player_code].keyPress_D = false;
		break;
	}

	if (true == players[player_code].keyPress_A) {
		if (players[player_code].pos.X > 10) {			// [임시] 맵 충돌체크
			players[player_code].pos.X -= 3;		
		}
		if (players[player_code].charPos.X > 0) {
			players[player_code].charPos.X -= 5;
		}
		players[player_code].left = 1;
		players[player_code].right = 0;
	}
	else {
		players[player_code].left = 0;
	}


	if (true == players[player_code].keyPress_D) {
		if (players[player_code].pos.X < 1200) { // [임시] 맵 충돌체크
			players[player_code].pos.X += 3;		
		}
		if (players[player_code].charPos.X < 1200) {
			players[player_code].charPos.X += 5;
		}
		players[player_code].left = 0;
		players[player_code].right = 1;
	}
	else {
		players[player_code].right = 0;
	}

	LeaveCriticalSection(&cs);
}

void ReadFile(SOCKET client_sock)
{
	int retval;
	char buf[BUFSIZE];

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

	// 데이터 보내기(고정 길이)

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
		retval = send(client_sock, buf, sizeof(buf), 0);
		printf("%d", retval);
		if (retval == SOCKET_ERROR) {
			printf("Send Failed\n");
			exit(1);
		}
		fclose(ff);
		printf("File - %s - Send complete\n", "mappos.txt");
	}

	//맵 파일 읽어서 변수에 위치 저장
	vector<BLOCK> v{28};

	ifstream in{ "mappos.txt", ios::binary };
	for (BLOCK& b : v) {
		b.read(in);
	}
	/*for (BLOCK& b : v) {
		cout << b.x  << " " << b.y << " " << b.width << endl;
	}*/
}

void MonsterPos(int num, BYTE player_code)
{
	if (num == 0) {
		if (Monster_X[num] < Block_local[2].x - players[player_code].charPos.X || Monster_X[num] + 72 > Block_local[2].x - players[player_code].charPos.X + Block_local[2].width) {
			if (Monster_X[num] < Block_local[2].x - players[player_code].charPos.X) {
				Monster_X[num] = Block_local[2].x - players[player_code].charPos.X;
			}

			if (Monster_X[num] + 72 > Block_local[2].x - players[player_code].charPos.X + Block_local[2].width) {
				Monster_X[num] = Block_local[2].x - players[player_code].charPos.X + Block_local[2].width - 72;
			}

			MonsterTurn[num]++;
		}

		if (MonsterTurn[num] % 2 == 0) {
			Monster_X[num] -= 5;
		}

		else {
			Monster_X[num] += 5;
		}
	}
}

DWORD WINAPI RecvThread(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	BYTE msg = 0;

	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;

	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	BYTE player_code = RandomCharacter();

	sockManager[player_code] = client_sock;

	//맵 위치 파일전송
	ReadFile(client_sock);
	//WaitForSingleObject(hFileEvent, INFINITE);

	retval = recv(client_sock, (char*)&Block_local[2].x, sizeof(Block_local[2].x), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		return 1;
	}

	retval = recv(client_sock, (char*)&Block_local[2].width, sizeof(Block_local[2].width), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		return 1;
	}

	// 캐릭터 초기값 설정
	InitPlayer(player_code);

	retval = send(client_sock, (char*)&player_code, sizeof(player_code), 0);
	if (retval == SOCKET_ERROR) {
		clientCount--;
		return 1;
	}

	printf("%d : send_code\n", player_code);

	Monster_X[0] = Block_local[2].x - players[0].charPos.X + Block_local[2].width - 72;

	EnterCriticalSection(&cs);
	ready++;
	LeaveCriticalSection(&cs);

	
	/*WaitForSingleObject(gameStartEvent, INFINITE);*/

	//게임 시작
	printf("%d : gamestart\n", player_code);

	while (1) {  // 1 -> checkGameEnd()
		/*MonsterPos(0, player_code);

		retval = send(client_sock, (char*)&Monster_X[0], sizeof(Monster_X[0]), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}

		printf("Monster_X[0]: %d\n", Monster_X[0]);

		retval = send(client_sock, (char*)&MonsterTurn[0], sizeof(MonsterTurn[0]), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}*/

		// 키입력 Recv
		retval = recv(client_sock, (char*)&msg, sizeof(msg), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}
		else if (retval == 0) {
			break;
		}
		
		ProcessPacket(msg, player_code);
	}

	ResetEvent(gameStartEvent);
	EnterCriticalSection(&cs);
	clientCount--;
	LeaveCriticalSection(&cs);
	return 0;
}

DWORD WINAPI CollisionSendThread(LPVOID arg)
{
	printf("enter collsion\n");
	char restart = 1;
	int retval;

	while (1) {
		EnterCriticalSection(&cs);
		if (ready > 2) {
			LeaveCriticalSection(&cs);
			break;
		}
		LeaveCriticalSection(&cs);
	}

	// 게임 시작
	SetEvent(gameStartEvent);
	printf("GameStart \n");
	bool flag = false;

	while (false == flag) { // CheckGameEnd()
		EnterCriticalSection(&cs);
		if (clientCount != 3) {
			LeaveCriticalSection(&cs);
			break;
		}
		LeaveCriticalSection(&cs);

		for (int i = 0; i < MAX_PLAYER; i++)
		{
			EnterCriticalSection(&cs);
			retval = send(sockManager[i], (char*)&players, sizeof(players), 0);
			if (retval == SOCKET_ERROR) {
				flag = true;
				LeaveCriticalSection(&cs);
				break;
			}
			LeaveCriticalSection(&cs);
		}

		Sleep(10);

	}

	ResetEvent(gameStartEvent);

	return 0;
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
	hFileEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	InitializeCriticalSection(&cs);
	while (1) {
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			//err_display("accept()");
			break;
		}
		EnterCriticalSection(&cs);
		if (clientCount < 3) {
			hThread = CreateThread(NULL, 0, RecvThread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			clientCount++;
		}
		else if(clientCount > 3) {
			closesocket(client_sock);
		}
		
		if (clientCount == 3) {
			hThread = CreateThread(NULL, 0, CollisionSendThread, NULL, 0, NULL);
			if (hThread == NULL) { CloseHandle(hThread); }
		}
		LeaveCriticalSection(&cs);

		printf("Access : %d client\n", clientCount);
	}

	closesocket(listen_sock);

	CloseHandle(gameStartEvent);
	CloseHandle(hFileEvent);

	DeleteCriticalSection(&cs);
	WSACleanup();

	return 0;
}