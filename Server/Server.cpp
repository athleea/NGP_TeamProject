#include "global.h"
#include "Protocol.h"
#include <fstream>
#include <vector>
#include <iostream>
using namespace std;

#define BLOCKNUM 31

int clientCount = 0;
FILE* fp;

HANDLE gameStartEvent;
CRITICAL_SECTION cs;
SOCKET sockManager[MAX_PLAYER];

int ready = 0;
bool gameover = false;
static int Monster_X[3] = { 0 };
static int MonsterTurn[3] = { 0, 0, 1 };
static int MonsterBlock[3] = { 2,6,9 };
static int MonsterKill[3] = { 0 };
static int Switch[3] = { 0 };
static int MoveBlockTurn[3] = { 0 };
static int MoveBlock_X[3] = { 50, 0, 0 };
int key = 1;
int NowBlock[3] = { 0 };
int KillChar = -1;
int HitChar = -1;
int DamageNum = 0;
int hit = 0;
int protect = 0;
bool damagetemp;
bool potal;

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

BLOCK Block_local[BLOCKNUM];

struct PlayerInfo {
	bool keyPress_D, keyPress_A;
	BYTE left, right;
	COORD pos;
	COORD charPos;
	BYTE hp;
	BYTE characterCode;
	bool jump;
	BYTE jumpCount;
	bool MCollision;
	bool CCollision;
};

PlayerInfo players[MAX_PLAYER];

//struct PlayerCollision {
//	BYTE OnBlock;
//};
//
//PlayerCollision pCollision[MAX_PLAYER];

typedef struct SendStruct {
	PlayerInfo players[MAX_PLAYER];
	int Monster_X[3];
	int MonsterTurn[3];
	int MonsterKill[3];
	int KillChar;
	int HitChar;
	int DamageNum;
	int MoveBlockTurn;
	int MoveBlock_X;
	int key;
	bool potal;
	BYTE sceneNumber;
} Send;

void CheckGameEnd();
void CheckGameEnd()
{
	for (int i = 0; i < MAX_PLAYER; ++i)
		if (players[i].hp <= 0) {
			EnterCriticalSection(&cs);
			gameover = true;
			LeaveCriticalSection(&cs);
			return;
		}
}
void InitPlayer(BYTE player_code);
void InitPlayer(BYTE player_code)
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
	players[player_code].MCollision = true;
	players[player_code].CCollision = false;
}

void InitMonster()
{
	for (int i = 0; i < 3; ++i) {
		if (i != 2) {
			Monster_X[i] = Block_local[MonsterBlock[i]].x;
		}
		else {
			Monster_X[i] = Block_local[MonsterBlock[i]].x + Block_local[MonsterBlock[i]].width;
		}
	}
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

int clamp(int min, int value, int max)
{
	if (value <= min) return min;
	else if (value >= max) return max;
	else return value;
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

	static bool jumptemp = false;
	if (true == players[player_code].jump) {
		if (jumptemp) jumptemp = false;
		else if (!jumptemp) jumptemp = true;

		if (jumptemp)
			players[player_code].jumpCount++;

		if (players[player_code].jumpCount < 10)
			players[player_code].pos.Y -= 10;
		else if (players[player_code].jumpCount < 19)
			players[player_code].pos.Y += 10;
		else if (players[player_code].jumpCount >= 20) {
			players[player_code].jump = false;
			players[player_code].jumpCount = 0;
		}
	}

	if (true == players[player_code].keyPress_A) {
		if (players[player_code].pos.X > 10) {
			players[player_code].pos.X -= 5;
		}
		players[player_code].left = 1;
	}
	else {
		players[player_code].left = 0;
	}

	if (true == players[player_code].keyPress_D) {
		if (players[player_code].pos.X < 2400) {
			players[player_code].pos.X += 5;
		}
		players[player_code].right = 1;
	}
	else {
		players[player_code].right = 0;
	}

	LeaveCriticalSection(&cs);
}

void SendFile(SOCKET client_sock)
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

		while (1) {
			int read_length = fread(buf, sizeof(char), BUFSIZE, ff);
			if (read_length <= 0)
				break;

			retval = send(client_sock, buf, read_length, 0);
			if (retval == SOCKET_ERROR) {
				printf("Send Failed\n");
				EnterCriticalSection(&cs);
				clientCount--;
				LeaveCriticalSection(&cs);
				return;
			}
			memset(buf, 0, BUFSIZE);

		}
		fclose(ff);
		printf("File - %s - Send complete\n", "mappos.txt");
	}

	ifstream in{ "mappos.txt", ios::binary };

	for (int i = 0; i < BLOCKNUM; ++i) {
		Block_local[i].read(in);
	}
}

void Gravity()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players[i].jump || players[i].MCollision || players[i].CCollision)
			continue;
		if (players[i].pos.Y >= 620) {
			players[i].pos.Y = 620;
			players[i].MCollision = true;
		}
		else if (!players[i].MCollision && !players[i].CCollision) {
			players[i].pos.Y += 5;
		}
	}
}

void MapCollision()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 0; j < BLOCKNUM - 2; ++j) {
			if (players[i].pos.X + 40 >= Block_local[j].x && players[i].pos.X + 40 <= Block_local[j].x + Block_local[j].width &&
				players[i].pos.Y + 70 >= Block_local[j].y && players[i].pos.Y + 70 <= Block_local[j].y + 30) {
				players[i].jump = 0;
				players[i].jumpCount = 0;
				players[i].MCollision = true;
				NowBlock[i] = j;
			}
			if (players[i].pos.X + 40 < Block_local[NowBlock[i]].x || players[i].pos.X + 40 > Block_local[NowBlock[i]].x + Block_local[NowBlock[i]].width) {
				players[i].MCollision = false;
			}
		}
		if (key == 0 && players[i].pos.X + 40 >= Block_local[29].x && players[i].pos.X + 40 <= Block_local[29].x + 150 &&
			players[i].pos.Y + 70 >= Block_local[29].y - 150 && players[i].pos.Y + 70 <= Block_local[29].y + 100) {
			potal = true;
		}
		if (players[i].pos.X + 40 >= Block_local[30].x && players[i].pos.X + 40 <= Block_local[30].x + 50 &&
			players[i].pos.Y + 70 >= Block_local[30].y && players[i].pos.Y <= Block_local[30].y + 50) {
			key = 0;
		}
	}
}

void CharacterCollision()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 0; j < MAX_PLAYER; ++j) {
			if (i == j) continue;
			if (players[i].pos.X + 40 >= players[j].pos.X && players[i].pos.X + 40 <= players[j].pos.X + 80 &&
				players[i].pos.Y + 70 <= players[j].pos.Y + 10 && players[i].pos.Y + 70 >= players[j].pos.Y - 2) {
				players[i].jump = 0;
				players[i].CCollision = true;
			}
			else
				players[i].CCollision = false;
		}
	}
}

void MonsterCollision()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (MonsterKill[j] == 0) {
				if (players[i].jump == 1) {
					if (players[i].pos.X + 40 >= Monster_X[j] && players[i].pos.X + 40 <= Monster_X[j] + 40 &&
						players[i].pos.Y + 70 >= Block_local[MonsterBlock[j]].y - 30 && players[i].pos.Y + 70 <= Block_local[MonsterBlock[j]].y) {
						MonsterKill[j] = 1;
						KillChar = i;
					}
				}
				else {
					if (protect == 0 && players[i].pos.X + 40 >= Monster_X[j] && players[i].pos.X + 40 <= Monster_X[j] + 40 &&
						players[i].pos.Y + 70 > Block_local[MonsterBlock[j]].y - 30 && players[i].pos.Y + 35 < Block_local[MonsterBlock[j]].y) {
						HitChar = i;
						damagetemp = true;
						players[i].hp--;
					}
					if (damagetemp) {
						DamageNum = 1;
						protect++;

						if (protect == 300) {
							protect = 0;
							damagetemp = false;
							HitChar = -1;
						}
					}
				}
			}
		}
	}
}

void MonsterPos(int num, int pos)
{
	if (Monster_X[num] < Block_local[pos].x || Monster_X[num] + 72 > Block_local[pos].x + Block_local[pos].width) {
		if (Monster_X[num] < Block_local[pos].x) {
			Monster_X[num] = Block_local[pos].x;
		}

		if (Monster_X[num] + 72 > Block_local[pos].x + Block_local[pos].width) {
			Monster_X[num] = Block_local[pos].x + Block_local[pos].width - 72;
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

void EventBlockPos(int num)
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
			if (MoveBlock_X[num] >= 0) {
				MoveBlock_X[num] -= 1;
			}

			else {
				MoveBlockTurn[num]++;
			}
		}

		else {
			if (MoveBlock_X[num] <= 1000) {
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
	SendFile(client_sock);

	// 캐릭터 초기값 설정
	InitPlayer(player_code);
	InitMonster();

	retval = send(client_sock, (char*)&player_code, sizeof(player_code), 0);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		clientCount--;
		LeaveCriticalSection(&cs);
		return 1;
	}

	Send send_struct;
	for (int i = 0; i < 3; ++i) {
		send_struct.MonsterTurn[i] = MonsterTurn[i];
		send_struct.Monster_X[i] = Monster_X[i];
		send_struct.MonsterKill[i] = MonsterKill[i];
	}
	send_struct.KillChar = KillChar;
	send_struct.HitChar = HitChar;
	send_struct.DamageNum = DamageNum;
	send_struct.MoveBlockTurn = MoveBlockTurn[0];
	send_struct.MoveBlock_X = MoveBlock_X[0];
	send_struct.sceneNumber = 0;
	memcpy(send_struct.players, players, sizeof(players));

	retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		clientCount--;
		LeaveCriticalSection(&cs);
		return 1;
	}

	EnterCriticalSection(&cs);
	ready++;
	LeaveCriticalSection(&cs);


	// 3명 접속까지 대기
	WaitForSingleObject(gameStartEvent, INFINITE);

	send_struct.sceneNumber = 1;

	retval = send(client_sock, (char*)&send_struct.sceneNumber, sizeof(send_struct.sceneNumber), 0);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		clientCount--;
		LeaveCriticalSection(&cs);
		return 1;
	}

	while (1) {
		// 키입력 Recv
		retval = recv(client_sock, (char*)&msg, sizeof(msg), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			break;
		}

		retval = recv(client_sock, (char*)&Switch[0], sizeof(Switch[0]), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			break;
		}

		ProcessPacket(msg, player_code);

		for (int i = 0; i < 3; ++i) {
			send_struct.MonsterTurn[i] = MonsterTurn[i];
			send_struct.Monster_X[i] = Monster_X[i];
			send_struct.MonsterKill[i] = MonsterKill[i];
		}
		send_struct.KillChar = KillChar;
		send_struct.HitChar = HitChar;
		send_struct.DamageNum = DamageNum;
		send_struct.MoveBlockTurn = MoveBlockTurn[0];
		send_struct.MoveBlock_X = MoveBlock_X[0];
		send_struct.key = key;
		send_struct.potal = potal;
		memcpy(send_struct.players, players, sizeof(players));

		retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);

		if (retval == SOCKET_ERROR) {
			break;
		}

		if (gameover == true) {
			send_struct.sceneNumber = 2;
			retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);
			printf("gameover\n");
			break;
		}

		//printf("Monster_X[0]: %d\n", Monster_X[0]);

		Sleep(20);
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

	while (1) { // CheckGameEnd()
		EnterCriticalSection(&cs);
		if (clientCount != 3) {
			LeaveCriticalSection(&cs);
			break;
		}
		LeaveCriticalSection(&cs);

		EventBlockPos(0);
		for (int i = 0; i < 3; ++i) {
			MonsterPos(i, MonsterBlock[i]);
		}
		MapCollision();
		CharacterCollision();
		Gravity();
		MonsterCollision();

		CheckGameEnd();

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
		else if (clientCount > 3) {
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

	DeleteCriticalSection(&cs);
	WSACleanup();

	return 0;
}