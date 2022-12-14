#include "global.h"
#include "Protocol.h"
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;

#define BLOCKNUM 35

int ClientNum = 0;
bool AllClientReady = false;
FILE* fp;

HANDLE gameStartEvent;
CRITICAL_SECTION cs;

int ready = 0;
bool gameover = false;
static int Monster_X[3] = { 0 };
static int MonsterTurn[3] = { 0, 0, 1 };
static int MonsterBlock[3] = { 2,6,9 };
static int MonsterKill[3] = { 0 };
static int Switch[3] = { 0 };
static int MoveBlockTurn[2] = { 0, 1 };
static int MoveBlock_X[2] = { 0 };
static int MoveBlockDis_L[2] = { 1400,1400 };
static int MoveBlockDis_R[2] = { 2200,2500 };
static int MoveBlockPos[2] = { 17,25 };
int push[3] = { -1,-1,-1 };
int key = 1;
int NowBlock[3] = { 0 };
int NowEblock[3] = { 0 };
int NowChar[3] = { -1,-1,-1 };
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
	BYTE hp;
	BYTE characterCode;
	bool jump;
	int jumpCount;
	bool MCollision;
	bool CCollision;
	bool ECollision;
};

PlayerInfo players[MAX_PLAYER];

typedef struct SendStruct {
	PlayerInfo players[MAX_PLAYER];
	int Monster_X[3];
	int MonsterTurn[3];
	int MonsterKill[3];
	int KillChar;
	int HitChar;
	int DamageNum;
	int Switch[3];
	int MoveBlockTurn[2];
	int MoveBlock_X[2];
	int key;
	bool potal;
	BYTE sceneNumber;
} Send;

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

void InitPlayer(BYTE player_code)
{
	if (player_code == 0)
		players[player_code].pos = { 30, 620 };
	else if (player_code == 1)
		players[player_code].pos = { 230, 620 };
	else if (player_code == 2)
		players[player_code].pos = { 430, 620 };

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
			MonsterTurn[i] = 0;
		}
		else {
			Monster_X[i] = Block_local[MonsterBlock[i]].x + Block_local[MonsterBlock[i]].width;
			MonsterTurn[i] = 1;
		}

		MonsterKill[i] = 0;
	}
}

void InitMoveBlock()
{
	for (int i = 0; i < 2; ++i) {
		MoveBlock_X[i] = Block_local[MoveBlockPos[i]].x;
		MoveBlockTurn[i] = i;
	}
}

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
		ClientNum--;
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
				ClientNum--;
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
		if (players[i].jump || players[i].MCollision || players[i].CCollision || players[i].ECollision)
			continue;
		if (players[i].pos.Y >= 620) {
			players[i].pos.Y = 620;
			players[i].MCollision = true;
		}

		else if (!players[i].MCollision && !players[i].CCollision && !players[i].ECollision) {
			players[i].jump = true;
			players[i].jumpCount = 60;
		}
	}
}

void MapCollision()
{
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 0; j < BLOCKNUM - 2; ++j) {
			if (j < 14) {}
			else if (j < 20) {
				if (i != 0) continue;
			}
			else if (j < 25) {
				if (i != 2) continue;
			}
			else {
				if (i != 1) continue;
			}

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

		for (int j = 0; j < 2; ++j) {
			if (players[i].pos.X + 40 >= MoveBlock_X[j] && players[i].pos.X + 40 <= MoveBlock_X[j] + Block_local[MoveBlockPos[j]].width &&
				players[i].pos.Y + 70 >= Block_local[MoveBlockPos[j]].y && players[i].pos.Y + 70 <= Block_local[MoveBlockPos[j]].y + Block_local[MoveBlockPos[j]].width) {
				players[i].jump = 0;
				players[i].jumpCount = 0;
				players[i].ECollision = true;
				NowEblock[i] = j;
			}

			else if (players[i].pos.X + 40 < MoveBlock_X[NowEblock[i]] || players[i].pos.X + 40 > MoveBlock_X[NowEblock[i]] + Block_local[MoveBlockPos[NowEblock[i]]].width) {
				players[i].ECollision = false;
			}
		}

		if (key == 0 && players[i].pos.X + 40 >= Block_local[33].x && players[i].pos.X + 40 <= Block_local[33].x + 150 &&
			players[i].pos.Y + 70 >= Block_local[33].y - 150 && players[i].pos.Y + 70 <= Block_local[33].y + 100) {
			potal = true;
			gameover = true;
		}

		if (players[i].pos.X + 40 >= Block_local[34].x && players[i].pos.X + 40 <= Block_local[34].x + 50 &&
			players[i].pos.Y + 70 >= Block_local[34].y && players[i].pos.Y <= Block_local[34].y + 50) {
			key = 0;
		}

		for (int j = 30; j < 33; ++j) {
			if (Switch[j - 30] == 0) {
				if (players[i].pos.X + 40 >= Block_local[j].x && players[i].pos.X + 40 <= Block_local[j].x + 35 &&
					players[i].pos.Y + 70 >= Block_local[j].y && players[i].pos.Y + 70 <= Block_local[j].y + 35) {
					Switch[j - 30] = 1;
					push[j - 30] = i;
				}
			}

			else if (players[push[j - 30]].pos.X + 40 < Block_local[j].x || players[push[j - 30]].pos.X + 40 > Block_local[j].x + 35) {
				Switch[j - 30] = 0;
				push[j - 30] = -1;
			}
		}
	}
}

void CharacterCollision()
{ 
	for (int i = 0; i < MAX_PLAYER; ++i) {
		for (int j = 0; j < MAX_PLAYER; ++j) {
			if (i == j) continue;
			if (players[i].pos.X + 40 >= players[j].pos.X && players[i].pos.X + 40 <= players[j].pos.X + 60 &&
				players[i].pos.Y + 70 <= players[j].pos.Y + 30 && players[i].pos.Y + 70 >= players[j].pos.Y - 2) {
				if (!(NowChar[i] == j && players[i].jump)) {
					players[i].jump = 0;
					players[i].jumpCount = 0;
				}
				players[i].CCollision = true;
				NowChar[i] = j;
			}
			else if (players[i].pos.X + 50 < players[NowChar[i]].pos.X || players[i].pos.X > players[NowChar[i]].pos.X + 50) {
				players[i].CCollision = false;
				NowChar[i] = -1;
			}
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
							DamageNum = 0;
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

void EventBlockPos(int num, int dis_l, int dis_r)
{
	if (MoveBlockTurn[num] % 2 == 0) {
		if (MoveBlock_X[num] >= dis_l) {
			MoveBlock_X[num] -= 1;
		}

		else {
			MoveBlockTurn[num]++;
		}
	}

	else {
		if (MoveBlock_X[num] <= dis_r) {
			MoveBlock_X[num] += 1;
		}

		else {
			MoveBlockTurn[num]++;
		}
	}
}
void CharacterPos()
{
	for (int i = 0; i < MAX_PLAYER; ++i)
	{
		EnterCriticalSection(&cs);
		if (true == players[i].jump) {
			players[i].jumpCount++;
			if (players[i].jumpCount < 40)
				players[i].pos.Y -= 5;
			else if (players[i].jumpCount < 79)
				players[i].pos.Y += 5;
			else if (players[i].jumpCount >= 80) {
				players[i].jump = false;
				players[i].jumpCount = 0;
			}
		}
		if (true == players[i].keyPress_A) {
			if (players[i].pos.X > 10) {
				players[i].pos.X -= 4;
			}
			players[i].left = 1;
		}
		else {
			players[i].left = 0;
		}
		if (true == players[i].keyPress_D) {
			if (players[i].pos.X < 2450) {
				players[i].pos.X += 4;
			}
			players[i].right = 1;
		}
		else {
			players[i].right = 0;
		}
		LeaveCriticalSection(&cs);
	}
}
void Restart()
{
	gameover = false;

	SetEvent(gameStartEvent);

	InitPlayer(0);
	InitPlayer(1);
	InitPlayer(2);

	InitMonster();

	KillChar - 1;
	HitChar = -1;
	DamageNum = 0;

	for (int i = 0; i < 3; ++i) {
		Switch[i] = 0;
	}

	InitMoveBlock();

	key = 1;
	potal = false;
}

DWORD WINAPI SendRecvThread(LPVOID arg)
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

	InitPlayer(player_code);

	retval = send(client_sock, (char*)&player_code, sizeof(player_code), 0);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		ClientNum--;
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
	for (int i = 0; i < 3; ++i) {
		send_struct.Switch[i] = Switch[i];
	}
	for (int i = 0; i < 2; ++i) {
		send_struct.MoveBlockTurn[i] = MoveBlockTurn[i];
		send_struct.MoveBlock_X[i] = MoveBlock_X[i];
	}
	send_struct.sceneNumber = 0;
	memcpy(send_struct.players, players, sizeof(players));

	retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		ClientNum--;
		LeaveCriticalSection(&cs);
		return 1;
	}

	//맵 위치 파일전송
	SendFile(client_sock);
	InitMonster();
	InitMoveBlock();

	EnterCriticalSection(&cs);
	ready++;
	if (ready == 3) {
		AllClientReady = true;
	}
	LeaveCriticalSection(&cs);


	// 3명 접속까지 대기
	WaitForSingleObject(gameStartEvent, INFINITE);

	send_struct.sceneNumber = 1;

	retval = send(client_sock, (char*)&send_struct.sceneNumber, sizeof(send_struct.sceneNumber), 0);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		ClientNum--;
		LeaveCriticalSection(&cs);
		return 1;
	}

	while (1) {
		// 키입력 Recv
		retval = recv(client_sock, (char*)&msg, sizeof(msg), MSG_WAITALL);
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
		for (int i = 0; i < 3; ++i) {
			send_struct.Switch[i] = Switch[i];
		}
		for (int i = 0; i < 2; ++i) {
			send_struct.MoveBlockTurn[i] = MoveBlockTurn[i];
			send_struct.MoveBlock_X[i] = MoveBlock_X[i];
		}
		send_struct.key = key;
		send_struct.potal = potal;
		memcpy(send_struct.players, players, sizeof(players));

		retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}

		if (gameover == true) {
			if (true == potal) send_struct.sceneNumber = 3;
			else send_struct.sceneNumber = 2;
			retval = send(client_sock, (char*)&send_struct, sizeof(send_struct), 0);

			Sleep(3000);

			send_struct.sceneNumber = 1;

			Restart();
		}
	}
	putchar('\n');

	ResetEvent(gameStartEvent);
	EnterCriticalSection(&cs);
	ClientNum--;
	LeaveCriticalSection(&cs);
	return 0;
}

DWORD WINAPI CollisionThread(LPVOID arg)
{
	while (1) {
		EnterCriticalSection(&cs);
		if (AllClientReady) {
			LeaveCriticalSection(&cs);
			break;
		}
		LeaveCriticalSection(&cs);
	}

	Sleep(1000);
	// 게임 시작
	SetEvent(gameStartEvent);

	while (1) { // CheckGameEnd()
		EnterCriticalSection(&cs);
		if (ClientNum != 3) {
			LeaveCriticalSection(&cs);
			break;
		}
		LeaveCriticalSection(&cs);

		CharacterPos();

		if (Switch[0] != 0 || Switch[1] != 0) {
			EventBlockPos(1, MoveBlockDis_L[1], MoveBlockDis_R[1]);
		}

		if (Switch[2] != 0) {
			EventBlockPos(0, MoveBlockDis_L[0], MoveBlockDis_R[0]);
		}

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
	EnterCriticalSection(&cs);
	ready = 0;
	LeaveCriticalSection(&cs);

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
	if (retval == SOCKET_ERROR) return 1;

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) return 1;

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
		EnterCriticalSection(&cs);
		if (ClientNum < 3) {
			hThread = CreateThread(NULL, 0, SendRecvThread, (LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }

			ClientNum++;

		}
		else if (ClientNum > 3) {
			closesocket(client_sock);
		}

		if (ClientNum == 3) {
			hThread = CreateThread(NULL, 0, CollisionThread, NULL, 0, NULL);
			if (hThread == NULL) { CloseHandle(hThread); }
		}
		LeaveCriticalSection(&cs);
		printf("Access : %d client\n", ClientNum);
	}

	closesocket(listen_sock);
	closesocket(client_sock);

	CloseHandle(gameStartEvent);

	DeleteCriticalSection(&cs);
	WSACleanup();

	return 0;
}