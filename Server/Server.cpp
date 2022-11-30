#include "global.h"
#include "Protocol.h"
#include <fstream>
#include <vector>

using namespace std;

int clientCount = 0;
FILE* fp;

HANDLE gameStartEvent, hFileEvent;
CRITICAL_SECTION cs;
SOCKET sockManager[MAX_PLAYER];

int ready = 0;
bool gameover = false;
static int Monster_X[3] = { 0 };
static int MonsterTurn[3] = { 0 };
static int Switch[3] = { 0 };
static int MoveBlockTurn[3] = { 0 };
static int MoveBlock_X[3] = { 50, 0, 0 };

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

typedef struct SendStruct {
	PlayerInfo players[MAX_PLAYER];
	int Monster_X;
	int MonsterTurn;
	int MoveBlockTurn;
	int MoveBlock_X;
} Send;

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
	if (true == players[player_code].jump) {
		players[player_code].jumpCount++;
		if (players[player_code].jumpCount < 10)
			players[player_code].pos.Y -= 15;
		else if (players[player_code].jumpCount < 19)
			players[player_code].pos.Y += 15;
		else if (players[player_code].jumpCount >= 20) {
			players[player_code].jump = false;
			players[player_code].jumpCount = 0;
		}
	}

	if (true == players[player_code].keyPress_A) {
		if (players[player_code].pos.X > 10) {			
			players[player_code].pos.X -= 5 ;
		}
		if (players[player_code].charPos.X > 0) {
			players[player_code].charPos.X -= 5;
		}
		players[player_code].left = 1;
	}
	else {
		players[player_code].left = 0;
	}


	if (true == players[player_code].keyPress_D) {
		if (players[player_code].pos.X < 1200) { 
			players[player_code].pos.X += 5;
		}
		if (players[player_code].charPos.X < 1200) {
			players[player_code].charPos.X += 5;
		}
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
		EnterCriticalSection(&cs);
		clientCount--;
		LeaveCriticalSection(&cs);
		return;
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
				EnterCriticalSection(&cs);
				clientCount--;
				LeaveCriticalSection(&cs);
				return;
			}
			memset(buf, 0, BUFSIZE);
		}
		retval = send(client_sock, buf, sizeof(buf), 0);
		printf("%d", retval);
		if (retval == SOCKET_ERROR) {
			printf("Send Failed\n");
			EnterCriticalSection(&cs);
			clientCount--;
			LeaveCriticalSection(&cs);
			return;
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
			Monster_X[num] -= 1;
		}

		else {
			Monster_X[num] += 1;
		}
	}
}

void EventBlockPos(int num, BYTE player_code)
{
	if (num == 0) {
		/*if (Switch[num] == 1) {
			if (MoveBlockTurn[num] % 2 == 0) {
				if (MoveBlock_X[num] - players[player_code].charPos.X >= 0) {
					MoveBlock_X[num] -= 1;
				}

				else {
					MoveBlockTurn[num]++;
				}
			}

			else {
				if (MoveBlock_X[num] + players[player_code].charPos.X <= 1000) {
					MoveBlock_X[num] += 1;
				}

				else {
					MoveBlockTurn[num]++;
				}
			}
		}*/
		if (MoveBlockTurn[num] % 2 == 0) {
			if (MoveBlock_X[num] - players[player_code].charPos.X >= 0) {
				MoveBlock_X[num] -= 1;
			}

			else {
				MoveBlockTurn[num]++;
			}
		}

		else {
			if (MoveBlock_X[num] + players[player_code].charPos.X <= 1000) {
				MoveBlock_X[num] += 1;
			}

			else {
				MoveBlockTurn[num]++;
			}
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

	retval = recv(client_sock, (char*)&Block_local[2].x, sizeof(Block_local[2].x), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		clientCount--;
		LeaveCriticalSection(&cs);

		return 1;
	}

	retval = recv(client_sock, (char*)&Block_local[2].width, sizeof(Block_local[2].width), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		clientCount--;
		LeaveCriticalSection(&cs);

		return 1;
	}
	
	// 캐릭터 초기값 설정
	InitPlayer(player_code);

	/*retval = send(client_sock, (char*)&player_code, sizeof(player_code), 0);
	if (retval == SOCKET_ERROR) {
		clientCount--;
		return 1;
	}*/

	printf("%d : send_code\n", player_code);

	EnterCriticalSection(&cs);
	ready++;
	LeaveCriticalSection(&cs);

	
	WaitForSingleObject(gameStartEvent, INFINITE);

	//게임 시작
	printf("%d : gamestart\n", player_code);

	while (1) {  // 1 -> checkGameEnd()
		// 키입력 Recv
		retval = recv(client_sock, (char*)&msg, sizeof(msg), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			break;
		}
		else if (retval == 0) {
			break;
		}

		retval = recv(client_sock, (char*)&Switch[0], sizeof(Switch[0]), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			break;
		}

		EventBlockPos(0, 0);

		MonsterPos(0, 0);

		ProcessPacket(msg, player_code);

		Send send_struct;
		send_struct.MonsterTurn = MonsterTurn[0];
		send_struct.Monster_X = Monster_X[0];
		send_struct.MoveBlockTurn = MoveBlockTurn[0];
		send_struct.MoveBlock_X = MoveBlock_X[0];
		memcpy(send_struct.players, players, sizeof(players));

		retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}
		//printf("Monster_X[0]: %d\n", Monster_X[0]);

		Sleep(10);
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
	int retval;
	SOCKET client_sock = (SOCKET)arg;

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

	InitPlayer(0);
	InitPlayer(1);
	InitPlayer(2);

	while (1) {
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			//err_display("accept()");
			break;
		}
		if (clientCount < 3) {
			hThread = CreateThread(NULL, 0, RecvThread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			EnterCriticalSection(&cs);
			clientCount++;
			LeaveCriticalSection(&cs);
		}
		else if(clientCount > 3) {
			closesocket(client_sock);
		}
		
		if (clientCount == 3) {
			hThread = CreateThread(NULL, 0, CollisionSendThread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { CloseHandle(hThread); }
		}

		printf("Access : %d client\n", clientCount);
	}

	closesocket(listen_sock);
	closesocket(client_sock);

	CloseHandle(gameStartEvent);
	CloseHandle(hFileEvent);

	DeleteCriticalSection(&cs);
	WSACleanup();

	return 0;
}