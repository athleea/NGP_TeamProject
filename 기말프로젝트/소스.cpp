#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <atlImage.h>
#include <Windows.h>
#pragma comment(lib, "winmm")
#include <mmsystem.h>
#include <fstream>
#include <vector>
#include <iostream>
#include "global.h"
#include "Protocol.h"

#define SERVERIP "127.0.0.1"
#define BLOCKNUM 35
using namespace std;

LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"NGP_Project";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
DWORD WINAPI CommunicationThread(LPVOID);

char buf[BUFSIZE];

struct BLOCK {
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
static BLOCK Block_local[BLOCKNUM];

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

BYTE scene_number;
CRITICAL_SECTION cs;
PlayerInfo players[MAX_PLAYER];
BYTE msg = -1;
BYTE player_code;

static int Monster_X[3] = { 0 };
static int MonsterTurn[3] = { 0, 0, 1 };
static int MonsterBlock[3] = { 2,6,9 };
static int MonsterKill[3] = { 0 };
static int Switch[3] = { 0 };
static int MoveBlockTurn[3] = { 0 };
static int MoveBlock_X[3] = { 0 };
static int MoveBlockPos[2] = { 17,25 };
int KillChar = 0;
int HitChar = 0;
int DamageNum = 0;

static int Key_Image = 1;
static int Key_X;
static int Key_Y;
static int Portal_X;
static int Portal_Y;
int potal;

bool gameover = false;

typedef struct RecvStruct {
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
} Recv;

void ReadFile(SOCKET sock)
{
	int ret;

	long long f_size;
	ret = recv(sock, (char*)&f_size, sizeof(f_size), MSG_WAITALL);
	if (ret == SOCKET_ERROR) {
		err_display("recv()");
	}
	FILE* ff;
	ff = fopen("mappos.txt", "wb");
	if (NULL == ff) {
		exit(1);
	}
	else {
		memset(buf, 0, BUFSIZE);

		int sum{};
		int buf_size;
		while (true) {
			buf_size = BUFSIZE;

			if (f_size - sum < buf_size) {
				buf_size = f_size - sum;
			}
			ret = recv(sock, buf, buf_size, MSG_WAITALL);

			sum += ret;

			fwrite(buf, sizeof(char), ret, ff);
			memset(buf, 0, BUFSIZE);

			if (f_size <= sum) {
				break;
			}
		}
	}
	fclose(ff);
	putchar('\n');
	printf("File - %s - download complete\n", "mappos.txt");

	ifstream in{ "mappos.txt", ios::binary };

	for (int i = 0; i < BLOCKNUM; ++i) {
		Block_local[i].read(in);
	}

	Portal_X = Block_local[33].x;
	Portal_Y = Block_local[33].y;
	Key_X = Block_local[34].x;
	Key_Y = Block_local[34].y;
}

Recv recv_struct;
HANDLE hRecvInitData;

DWORD WINAPI CommunicationThread(LPVOID arg)
{
	WSAData wsa;
	int retval;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == SOCKET_ERROR) return 0;

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) return 1;

	retval = recv(sock, (char*)&player_code, sizeof(player_code), MSG_WAITALL);
	if (retval == SOCKET_ERROR) return  1;

	retval = recv(sock, (char*)&recv_struct, sizeof(recv_struct), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		return 1;
	}


	for (int i = 0; i < 3; ++i) {
		MonsterTurn[i] = recv_struct.MonsterTurn[i];
		Monster_X[i] = recv_struct.Monster_X[i];
		MonsterKill[i] = recv_struct.MonsterKill[i];
	}

	KillChar = recv_struct.KillChar;
	HitChar = recv_struct.HitChar;
	DamageNum = recv_struct.DamageNum;

	for (int i = 0; i < 3; ++i) {
		Switch[i] = recv_struct.Switch[i];
	}

	for (int i = 0; i < 2; ++i) {
		MoveBlockTurn[i] = recv_struct.MoveBlockTurn[i];
		MoveBlock_X[i] = recv_struct.MoveBlock_X[i];
	}

	scene_number = recv_struct.sceneNumber;
	memcpy(players, recv_struct.players, sizeof(players));

	SetEvent(hRecvInitData);

	ReadFile(sock);

	EnterCriticalSection(&cs);
	retval = recv(sock, (char*)&scene_number, sizeof(scene_number), MSG_WAITALL);
	LeaveCriticalSection(&cs);
	if (retval == SOCKET_ERROR) return  1;

	while (1) {
		EnterCriticalSection(&cs);
		retval = send(sock, (char*)&msg, sizeof(msg), 0);
		LeaveCriticalSection(&cs);
		if (retval == SOCKET_ERROR) {
			break;
		}

		retval = recv(sock, (char*)&recv_struct, sizeof(recv_struct), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			break;
		}

		for (int i = 0; i < 3; ++i) {
			MonsterTurn[i] = recv_struct.MonsterTurn[i];
			Monster_X[i] = recv_struct.Monster_X[i];
			MonsterKill[i] = recv_struct.MonsterKill[i];
		}
		KillChar = recv_struct.KillChar;
		HitChar = recv_struct.HitChar;
		DamageNum = recv_struct.DamageNum;
		for (int i = 0; i < 3; ++i) {
			Switch[i] = recv_struct.Switch[i];
		}
		for (int i = 0; i < 2; ++i) {
			MoveBlockTurn[i] = recv_struct.MoveBlockTurn[i];
			MoveBlock_X[i] = recv_struct.MoveBlock_X[i];
		}
		Key_Image = recv_struct.key;
		potal = recv_struct.potal;
		scene_number = recv_struct.sceneNumber;
		memcpy(players, recv_struct.players, sizeof(players));
	}

	//Exit
	printf("close\n");
	closesocket(sock);
	WSACleanup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&WndClass);

	hWnd = CreateWindow(
		lpszClass,
		lpszWindowName,
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		1280,
		800,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	InitializeCriticalSection(&cs);
	hRecvInitData = CreateEvent(NULL, TRUE, FALSE, NULL);

	HANDLE hThread = CreateThread(NULL, 0, CommunicationThread, NULL, 0, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	DeleteCriticalSection(&cs);

	CloseHandle(hThread);
	CloseHandle(hRecvInitData);
}

struct ImgSprite {
	CImage stand[4];
	CImage run_L[4];
	CImage run_R[4];
	CImage jump[4];

	int width_stand[4];
	int width_run[4];
	int width_jump[4];

	int height_stand[4];
	int height_run[4];
	int height_jump[4];
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, mem1dc, mem2dc;
	PAINTSTRUCT ps;

	COLORREF color;

	static HBITMAP hBitmap, hBitmap2;
	static CImage BackGround, imgGround;

	static ImgSprite imgSprite[3];
	static CImage Start, Lobby, Dialog[6], Guide, Heart, Key, Portal, Clear[2], Guide2, GameOver, Monster_L[4], Monster_R[4], Map, Damage[4], Eblock[2];
	static CImage Block, Blockr, Blockg, Blocko;

	static RECT rect;

	static int w_damage[4], h_damage[4];
	static int Image_Number = 0;
	static int w_guide, h_guide;
	static int MouseX, MouseY;
	static int w_block, h_block;
	static int w_eblock, h_eblock;
	static int w_heart, h_heart;
	static int w_Key, h_Key;
	static int w_Portal, h_Portal;
	static int w_monster[4], h_monster[4];
	static int ClearCount = 0;
	static int KillNum[3] = { 1,1,1 };

	static int bw, bh, gw, gh;

	static int count = 0;
	static int damagecount = 0;

	static int last;

	static int CharNum = 1;
	static int click = 0;

	static int Monster1Turn;
	static int Monster2Turn;

	static int hit;
	static int protect;

	static int Heart_Click = 0;
	static int MapNum = -1;

	static int Block_localX = 50;
	Block_local[23].y = 500;
	Block_local[23].width = 100;
	static int MoveBlock;
	
	static int KillMonster1;
	static int KillMonster2;

	static COORD charPos = { 0,0 };
	static COORD pos = { 0,0 };
	static BYTE left = 0;
	static BYTE right = 0;
	static bool jump = 0;
	static int jumpCount = 0;
	static BYTE hp;


	static int damagetemp = 0;
	static int counttemp = 0;
	static int monstertemp = 0;
	static bool once = false;

	switch (iMsg) {
	case WM_CREATE:
		PlaySound(L"start.wav", NULL, SND_ASYNC);

		BackGround.Load(L"BackGround.png");
		imgGround.Load(L"Ground.png");

		Start.Load(L"GameStart.png");
		Lobby.Load(L"lobby.png");

		imgSprite[0].stand[0].Load(L"cookie1-stand1.png");
		imgSprite[0].stand[1].Load(L"cookie1-stand1.png");
		imgSprite[0].stand[2].Load(L"cookie1-stand2.png");
		imgSprite[0].stand[3].Load(L"cookie1-stand2.png");

		imgSprite[0].run_L[0].Load(L"cookie1-runL1.png");
		imgSprite[0].run_L[1].Load(L"cookie1-runL2.png");
		imgSprite[0].run_L[2].Load(L"cookie1-runL3.png");
		imgSprite[0].run_L[3].Load(L"cookie1-runL4.png");

		imgSprite[0].run_R[0].Load(L"cookie1-runR1.png");
		imgSprite[0].run_R[1].Load(L"cookie1-runR2.png");
		imgSprite[0].run_R[2].Load(L"cookie1-runR3.png");
		imgSprite[0].run_R[3].Load(L"cookie1-runR4.png");

		imgSprite[0].jump[0].Load(L"cookie1-jump1.png");
		imgSprite[0].jump[1].Load(L"cookie1-jump2.png");
		imgSprite[0].jump[2].Load(L"cookie1-jump3.png");
		imgSprite[0].jump[3].Load(L"cookie1-jump4.png");

		imgSprite[1].stand[0].Load(L"cookie2-stand1.png");
		imgSprite[1].stand[1].Load(L"cookie2-stand1.png");
		imgSprite[1].stand[2].Load(L"cookie2-stand2.png");
		imgSprite[1].stand[3].Load(L"cookie2-stand2.png");

		imgSprite[1].run_L[0].Load(L"cookie2-runL1.png");
		imgSprite[1].run_L[1].Load(L"cookie2-runL2.png");
		imgSprite[1].run_L[2].Load(L"cookie2-runL3.png");
		imgSprite[1].run_L[3].Load(L"cookie2-runL4.png");

		imgSprite[1].run_R[0].Load(L"cookie2-runR1.png");
		imgSprite[1].run_R[1].Load(L"cookie2-runR2.png");
		imgSprite[1].run_R[2].Load(L"cookie2-runR3.png");
		imgSprite[1].run_R[3].Load(L"cookie2-runR4.png");

		imgSprite[1].jump[0].Load(L"cookie2-jump1.png");
		imgSprite[1].jump[1].Load(L"cookie2-jump2.png");
		imgSprite[1].jump[2].Load(L"cookie2-jump3.png");
		imgSprite[1].jump[3].Load(L"cookie2-jump4.png");

		imgSprite[2].stand[0].Load(L"cookie3-stand1.png");
		imgSprite[2].stand[1].Load(L"cookie3-stand1.png");
		imgSprite[2].stand[2].Load(L"cookie3-stand2.png");
		imgSprite[2].stand[3].Load(L"cookie3-stand2.png");

		imgSprite[2].run_L[0].Load(L"cookie3-runL1.png");
		imgSprite[2].run_L[1].Load(L"cookie3-runL2.png");
		imgSprite[2].run_L[2].Load(L"cookie3-runL3.png");
		imgSprite[2].run_L[3].Load(L"cookie3-runL4.png");

		imgSprite[2].run_R[0].Load(L"cookie3-runR1.png");
		imgSprite[2].run_R[1].Load(L"cookie3-runR2.png");
		imgSprite[2].run_R[2].Load(L"cookie3-runR3.png");
		imgSprite[2].run_R[3].Load(L"cookie3-runR4.png");

		imgSprite[2].jump[0].Load(L"cookie3-jump1.png");
		imgSprite[2].jump[1].Load(L"cookie3-jump2.png");
		imgSprite[2].jump[2].Load(L"cookie3-jump3.png");
		imgSprite[2].jump[3].Load(L"cookie3-jump4.png");


		Monster_L[0].Load(L"monster-L1.png");
		Monster_L[1].Load(L"monster-L2.png");
		Monster_L[2].Load(L"monster-L3.png");
		Monster_L[3].Load(L"monster-L4.png");

		Monster_R[0].Load(L"monster-R1.png");
		Monster_R[1].Load(L"monster-R2.png");
		Monster_R[2].Load(L"monster-R3.png");
		Monster_R[3].Load(L"monster-R4.png");

		Guide.Load(L"팻말.png");
		Guide2.Load(L"Guide2.png");
		Heart.Load(L"heart.png");
		Key.Load(L"key.png");
		Portal.Load(L"Portal.png");

		Dialog[1].Load(L"Dialog1.png");
		Dialog[2].Load(L"Dialog2.png");
		Dialog[3].Load(L"Dialog3.png");
		Dialog[5].Load(L"Dialog5.png");

		Clear[0].Load(L"Clear1.png");
		Clear[1].Load(L"Clear2.png");
		GameOver.Load(L"GameOver.png");
		Map.Load(L"NewMap.jpg");

		Damage[0].Load(L"Damage1.png");
		Damage[1].Load(L"Damage2.png");
		Damage[2].Load(L"Damage3.png");
		Damage[3].Load(L"Damage4.png");

		Block.Load(L"Block.png");
		Blockr.Load(L"Blockr.jpg");
		Blockg.Load(L"Blockg.jpg");
		Blocko.Load(L"Blocko.jpg");
		Eblock[0].Load(L"EventBlock.jpg");
		Eblock[1].Load(L"EventBlock_down.png");

		GetClientRect(hWnd, &rect);

		bw = BackGround.GetWidth();
		bh = BackGround.GetHeight();
		gw = imgGround.GetWidth();
		gh = imgGround.GetHeight();

		w_guide = Guide.GetWidth();
		h_guide = Guide.GetHeight();

		w_block = Block.GetWidth();
		h_block = Block.GetHeight();

		w_eblock = Eblock[0].GetWidth();
		h_eblock = Eblock[0].GetHeight();

		w_heart = Heart.GetWidth();
		h_heart = Heart.GetHeight();

		w_Key = Key.GetWidth();
		h_Key = Key.GetHeight();

		w_Portal = Portal.GetWidth();
		h_Portal = Portal.GetHeight();

		for (int i = 0; i < 4; ++i) {
			w_damage[i] = Damage[i].GetWidth();
			h_damage[i] = Damage[i].GetHeight();

			imgSprite[0].width_stand[i] = imgSprite[0].stand[i].GetWidth();
			imgSprite[0].height_stand[i] = imgSprite[0].stand[i].GetHeight();

			imgSprite[0].width_run[i] = imgSprite[0].run_L[i].GetWidth();
			imgSprite[0].height_run[i] = imgSprite[0].run_L[i].GetHeight();

			imgSprite[0].width_jump[i] = imgSprite[0].jump[i].GetWidth();
			imgSprite[0].height_jump[i] = imgSprite[0].jump[i].GetHeight();

			imgSprite[1].width_stand[i] = imgSprite[1].stand[i].GetWidth();
			imgSprite[1].height_stand[i] = imgSprite[1].stand[i].GetHeight();

			imgSprite[1].width_run[i] = imgSprite[1].run_L[i].GetWidth();
			imgSprite[1].height_run[i] = imgSprite[1].run_L[i].GetHeight();

			imgSprite[1].width_jump[i] = imgSprite[1].jump[i].GetWidth();
			imgSprite[1].height_jump[i] = imgSprite[1].jump[i].GetHeight();

			imgSprite[2].width_stand[i] = imgSprite[2].stand[i].GetWidth();
			imgSprite[2].height_stand[i] = imgSprite[2].stand[i].GetHeight();

			imgSprite[2].width_run[i] = imgSprite[2].run_L[i].GetWidth();
			imgSprite[2].height_run[i] = imgSprite[2].run_L[i].GetHeight();

			imgSprite[2].width_jump[i] = imgSprite[2].jump[i].GetWidth();
			imgSprite[2].height_jump[i] = imgSprite[2].jump[i].GetHeight();

			w_monster[i] = Monster_L[i].GetWidth();
			h_monster[i] = Monster_R[i].GetHeight();
		}

		SetTimer(hWnd, 0, 50, NULL);

		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
		mem1dc = CreateCompatibleDC(hdc);

		hBitmap2 = CreateCompatibleBitmap(mem1dc, rect.right, rect.bottom);
		mem2dc = CreateCompatibleDC(mem1dc);

		SelectObject(mem1dc, hBitmap);

		count = ++count % 4;

		switch (scene_number)
		{
		case 0:	// 접속대기
			if (click == 0)
				Start.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			else {
				Lobby.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
				WaitForSingleObject(hRecvInitData, INFINITE);
				imgSprite[player_code].stand[count].Draw(mem1dc, 550, 400);
			}
			break;
		case 1:	// 게임시작
			pos = players[player_code].pos;
			left = players[player_code].left;
			right = players[player_code].right;
			jump = players[player_code].jump;
			jumpCount = players[player_code].jumpCount;
			hp = players[player_code].hp;

			charPos.X = clamp(0, pos.X - 640, 1280);
			charPos.Y = clamp(-1200, pos.Y - 500, 620);

			BackGround.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0 + charPos.X, bh - 1800 + charPos.Y, 2560, 1600);
			imgGround.Draw(mem1dc, 0 - charPos.X, 130 - charPos.Y, rect.right, rect.bottom, 0, 0, gw, gh);
			imgGround.Draw(mem1dc, rect.right - charPos.X, 130 - charPos.Y, rect.right, rect.bottom, 0, 0, gw, gh);

			Guide.Draw(mem1dc, 750 - charPos.X, 590 - charPos.Y, w_guide * 2 / 3, h_guide * 2 / 3, 0, 0, w_guide, h_guide);
			Portal.Draw(mem1dc, Portal_X - charPos.X, Portal_Y - charPos.Y - h_Portal * 1 / 4, w_Portal * 1 / 4, h_Portal * 1 / 4, 0, 0, w_Portal, h_Portal);

			for (int i = 0; i < 2; ++i) {
				if (i == 0) {
					Blockr.Draw(mem1dc, MoveBlock_X[i] - charPos.X, Block_local[MoveBlockPos[i]].y - charPos.Y, Block_local[MoveBlockPos[i]].width, 60, 0, 0, w_block, h_block);
				}

				else {
					Blocko.Draw(mem1dc, MoveBlock_X[i] - charPos.X, Block_local[MoveBlockPos[i]].y - charPos.Y, Block_local[MoveBlockPos[i]].width, 60, 0, 0, w_block, h_block);
				}
			}

			for (int i = 0; i < BLOCKNUM - 2; i++) {
				if (i < 14)
					Block.Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, Block_local[i].width, 60, 0, 0, w_block, h_block);
				else if ((i >= 14 && i < 17) || (i > 17 && i < 20))
					Blockr.Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, Block_local[i].width, 60, 0, 0, w_block, h_block);
				else if (i < 25 && i != 17)
					Blockg.Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, Block_local[i].width, 60, 0, 0, w_block, h_block);
				else if (i > 25 && i < 30)
					Blocko.Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, Block_local[i].width, 60, 0, 0, w_block, h_block);
				else {
					if (i >= 30 && Switch[i - 30] == 0) {
						Eblock[0].Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, w_eblock / 3, h_eblock / 3, 0, 0, w_eblock, h_eblock);
					}

					else if (i >= 30 && Switch[i - 30] == 1) {
						Eblock[1].Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, w_eblock / 3, h_eblock / 3, 0, 0, w_eblock, h_eblock);
					}
				}
			}

			SelectObject(mem2dc, hBitmap2);

			SelectObject(mem1dc, hBitmap);

			for (int i = 0; i < 3; ++i) {
				if (MonsterKill[i] == 0) {
					if (MonsterTurn[i] % 2 == 0) {
						Monster_L[count].Draw(mem1dc, Monster_X[i] - charPos.X, Block_local[MonsterBlock[i]].y - charPos.Y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);
					}

					if (MonsterTurn[i] % 2 != 0) {
						Monster_R[count].Draw(mem1dc, Monster_X[i] - charPos.X, Block_local[MonsterBlock[i]].y - charPos.Y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);
					}
				}
			}

			if (Key_Image == 1)
				Key.Draw(mem1dc, Key_X - charPos.X, Key_Y - charPos.Y, w_Key * 1 / 2, h_Key * 1 / 2, 0, 0, w_Key, h_Key);

			if (Key_Image == 0) {
				Key.Draw(mem1dc, 1170, 0, w_Key * 1 / 2, h_Key * 1 / 2, 0, 0, w_Key, h_Key);
			}

			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (players[i].left == 0 && players[i].right == 0 && players[i].jump == 0)
					imgSprite[i].stand[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_stand[count] / 2, imgSprite[i].height_stand[count] / 2,
						0, 0, imgSprite[i].width_stand[count], imgSprite[i].height_stand[count]);
				else if (players[i].jump == 1)
					imgSprite[i].jump[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_jump[count] / 2, imgSprite[i].height_jump[count] / 2,
						0, 0, imgSprite[i].width_jump[count], imgSprite[i].height_jump[count]);
				else if (players[i].left == 1)
					imgSprite[i].run_L[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_run[count] / 2, imgSprite[i].height_run[count] / 2,
						0, 0, imgSprite[i].width_run[count], imgSprite[i].height_run[count]);
				else if (players[i].right == 1)
					imgSprite[i].run_R[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_run[count] / 2, imgSprite[i].height_run[count] / 2,
						0, 0, imgSprite[i].width_run[count], imgSprite[i].height_run[count]);
			}

			if (Image_Number != 0 && Image_Number < 4) {
				Dialog[Image_Number].Draw(mem1dc, 0, 100, rect.right, rect.bottom, 0, 0, 1280, 800);
			}

			if (Image_Number == 5) {
				Dialog[Image_Number].Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			}

			damagetemp = ++damagetemp % 6;
			if (!damagetemp) {
				damagecount = ++damagecount % 4;
				if (DamageNum == 1 || (MonsterKill[0] == 1 && KillNum[0] == 1) || (MonsterKill[1] == 1 && KillNum[1] == 1) || (MonsterKill[2] == 1 && KillNum[2] == 1))
					counttemp = ++counttemp % 4;
			}


			if (DamageNum == 1) {
				Damage[damagecount].Draw(mem1dc, players[HitChar].pos.X - charPos.X, players[HitChar].pos.Y - charPos.Y, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);
			}
			if (counttemp == 3) DamageNum = 0;

			for (int i = 0; i < 3; ++i) {
				if (MonsterKill[i] == 1) {
					if (KillNum[i] == 1) {
						if (once == false) {
							monstertemp = Monster_X[i];
							once = true;
						}
						Damage[damagecount].Draw(mem1dc, monstertemp, Block_local[MonsterBlock[i]].y - 30 - charPos.Y, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);
						if (counttemp == 3) {
							KillNum[i] = 0;
						}
					}
				}
			}

			if (hp == 3) {
				Heart.Draw(mem1dc, 10, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
				Heart.Draw(mem1dc, 20 + w_heart / 120, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
				Heart.Draw(mem1dc, 30 + w_heart / 60, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
			}

			else if (hp == 2) {
				Heart.Draw(mem1dc, 10, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
				Heart.Draw(mem1dc, 20 + w_heart / 120, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
			}

			else if (hp == 1) {
				Heart.Draw(mem1dc, 10, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
			}

			if (MapNum == 1) {
				Map.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1440, 1008);
			}

			if (Image_Number == 8) {
				Guide2.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			}
			break;
		case 2:	// 게임종료
			GameOver.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			break;
		case 3:
			if (ClearCount == 0) {
				ClearCount = 1;
			}
			else ClearCount = 0;

			Clear[ClearCount].Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			break;
		default:
			break;
		}

		BitBlt(hdc, 0, 0, rect.right, rect.bottom, mem1dc, 0, 0, SRCCOPY);

		DeleteObject(hBitmap);
		DeleteDC(mem1dc);
		DeleteObject(hBitmap2);
		DeleteDC(mem2dc);

		EndPaint(hWnd, &ps);
		break;

	case WM_TIMER:
		InvalidateRect(hWnd, NULL, FALSE);

		break;

	case WM_KEYUP:
		if (scene_number == 1) {
			if (wParam == 'A' || wParam == 'a') {
				EnterCriticalSection(&cs);
				msg = CS_KEYUP_A;
				LeaveCriticalSection(&cs);
			}
			else if (wParam == 'D' || wParam == 'd') {
				EnterCriticalSection(&cs);
				msg = CS_KEYUP_D;
				LeaveCriticalSection(&cs);
			}
			else if (wParam == VK_SPACE) {
				EnterCriticalSection(&cs);
				msg = CS_KEYUP_SPACE;
				LeaveCriticalSection(&cs);
			}
		}

		break;
	case WM_KEYDOWN:
		if (scene_number == 1) {
			if (wParam == VK_SPACE) {
				EnterCriticalSection(&cs);
				msg = CS_KEYDOWN_SPACE;
				LeaveCriticalSection(&cs);
			}
			else if (wParam == 'M' || wParam == 'm') {
				MapNum = -MapNum;
			}
			else if (wParam == 'A' || wParam == 'a') {
				EnterCriticalSection(&cs);
				msg = CS_KEYDOWN_A;
				LeaveCriticalSection(&cs);
			}
			else if (wParam == 'D' || wParam == 'd') {
				EnterCriticalSection(&cs);
				msg = CS_KEYDOWN_D;
				LeaveCriticalSection(&cs);
			}
			else if (wParam == 'W' || wParam == 'w') {
				if (potal) {
					if (Key_Image == 0) {
						Image_Number = 7;
					}
					else if (Image_Number == 8) {
						Image_Number = 6;
					}
					else Image_Number = 8;
				}
			}
		}

		break;

	case WM_LBUTTONDOWN:
		MouseX = LOWORD(lParam);
		MouseY = HIWORD(lParam);

		click++;

		if (click == 2) {
			PlaySound(L"OST.wav", NULL, SND_ASYNC | SND_LOOP);
		}

		if (Image_Number >= 0 && Image_Number < 4)
			Image_Number++;

		if (MouseX >= 750 - charPos.X && MouseX <= 750 - charPos.X + w_guide * 2 / 3 && MouseY >= 590 - charPos.Y && MouseY <= 590 - charPos.Y + h_guide * 2 / 3) {
			Image_Number = 5;
		}

		else if (Image_Number == 5)
			Image_Number = 4;

		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_DESTROY:
		KillTimer(hWnd, 0);
		PostQuitMessage(0);

		break;

	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
};