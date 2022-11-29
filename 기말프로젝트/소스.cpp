#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" )
#endif

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
#define BLOCKNUM 24
using namespace std;

LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"windows program";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
DWORD WINAPI CommunicationThread(LPVOID);

HANDLE hRecvEvent, hRenderEvent, hFileEvent, hKeyInputEvent;
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
static BLOCK Block_local[28];

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
CRITICAL_SECTION cs;
PlayerInfo players[MAX_PLAYER];
BYTE msg = 0;
BYTE player_code;
static COORD charPos = { 0,0 };
static int Monster_X[3] = { 0 };
static int MonsterTurn[3] = { 0 };

void ReadFile(SOCKET sock)
{
	int ret;

	long long f_size;
	ret = recv(sock, (char*)&f_size, sizeof(f_size), 0);
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
		while (true) {
			ret = recv(sock, buf, BUFSIZE, 0);
			sum += ret;

			fwrite(buf, sizeof(char), ret, ff);
			memset(buf, 0, BUFSIZE);

			if (f_size <= sum) {
				//SetEvent(hFileEvent);
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
	/*for (int i = 0; i < BLOCKNUM; ++i) {
		cout << Block_local[i].x  << " " << Block_local[i].y << " " << Block_local[i].width << endl;
	}*/
}

DWORD WINAPI RecvThread(LPVOID arg)
{
	int retval = 0;
	SOCKET sock = (SOCKET)arg;
	retval = recv(sock, (char*)&player_code, sizeof(player_code), MSG_WAITALL);
	if (retval == SOCKET_ERROR) return  1;

	while (1) {
		
	}

	return 0;
}
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

	HANDLE hThread = CreateThread(NULL, 0, RecvThread, (LPVOID) sock, 0, NULL);
	
	//맵 위치 파일수신
	//ReadFile(sock);
	/*
	retval = send(sock, (char*)&Block_local[2].x, sizeof(Block_local[2].x), 0);	// 몬스터 초기 좌표 전송, Block_local[2].x 부분 변경 필요
	if (retval == SOCKET_ERROR) {
		return 1;
	}
	printf("%d\n", Block_local[2].x);
	retval = send(sock, (char*)&Block_local[2].width, sizeof(Block_local[2].width), 0);
	if (retval == SOCKET_ERROR) {
		return 1;
	}
	printf("%d\n", Block_local[2].width);
	*/
	// 캐릭터 코드 및 초기값 받기
	/*retval = recv(sock, (char*)&player_code, sizeof(player_code), MSG_WAITALL);
	if (retval == SOCKET_ERROR) return  1;
	int value = 0;*/

	while (1) {
		//WaitForSingleObject(hKeyInputEvent, INFINITE);

		retval = send(sock, (char*)&msg, sizeof(msg), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}

		/*retval = recv(sock, (char*)&Monster_X[0], sizeof(Monster_X[0]), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}

		retval = recv(sock, (char*)&MonsterTurn[0], sizeof(MonsterTurn[0]), 0);
		if (retval == SOCKET_ERROR) {
			break;
		}*/
		retval = recv(sock, (char*)&players, sizeof(players), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			break;
		}

	}

	//Exit
	CloseHandle(hThread);
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

	hFileEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	hKeyInputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE hThread = CreateThread(NULL, 0, CommunicationThread, NULL, 0, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	DeleteCriticalSection(&cs);
	CloseHandle(hThread);
	CloseHandle(hFileEvent);
	CloseHandle(hKeyInputEvent);
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

	//BLOCK Block_local[24] = { 0 };

	static HBITMAP hBitmap, hBitmap2;
	static CImage BackGround, imgGround;

	static ImgSprite imgSprite[3];
	static CImage Start, Dialog[6], Guide, Block, Heart, Key, Portal, Clear[2], Guide2, GameOver, Monster_L[4], Monster_R[4], Map, Damage[4], Eblock[2];

	static RECT rect;

	/*
	static int w_stand[10], h_stand[10];
	static int w_run[4], h_run[4];
	static int w_jump[4], h_jump[4];
	*/
	static int w1_stand[4], h1_stand[4];
	static int w1_run[4], h1_run[4];
	static int w1_jump[4], h1_jump[4];
	static int w2_stand[4], h2_stand[4];
	static int w2_run[4], h2_run[4];
	static int w2_jump[4], h2_jump[4];
	static int w3_stand[4], h3_stand[4];
	static int w3_run[4], h3_run[4];
	static int w3_jump[4], h3_jump[4];
	static int w_damage[4], h_damage[4];
	static int Image_Number = 0;
	static int w_guide, h_guide;
	static int MouseX, MouseY;
	static int w_block, h_block;
	static int w_eblock, h_eblock;
	static int OnBlock = 0;
	static int w_heart, h_heart;
	static int w_Key, h_Key;
	static int w_Portal, h_Portal;
	static int w_monster[4], h_monster[4];
	static int ClearCount = 0;
	static int DamageNum = 0;
	static int KillNum = 0;

	static int bw, bh, gw, gh;

	static int count = 0;



	static int last;

	static int CharNum = 1;
	static int click = 0;

	static int Monster1Turn;	//	아래쪽 몬스터, 해당 변수가 짝수인지 홀수인지에 따라 방향 바뀜
	static int Monster2Turn;	//	위쪽 몬스터

	static int hit;	//	몬스터 충돌 횟수
	static int protect;	//	처음 충돌 후 일정 시간 무적, 데미지 받지 않음. 이거 판단하는 변수

	static int Key_X = 1200;	// Key 위치 수정 필요
	static int Key_Y = -1020;
	static int Key_Image = 1;
	static int Heart_Click = 0;
	static int MapNum = 0;
	static int Switch = 0;

	static int Portal_X = 800;		// Portal 위치 수정 필요
	static int Portal_Y = -120;


	//Block_local[0].x = 150;
	//Block_local[0].y = 600;
	//Block_local[0].width = 50;
	//Block_local[1].x = 250;
	//Block_local[1].y = 550;
	//Block_local[1].width = 200;
	//Block_local[2].x = 500;
	//Block_local[2].y = 470;
	//Block_local[2].width = 70;
	//Block_local[3].x = 700;
	//Block_local[3].y = 510;
	//Block_local[3].width = 300;
	//Block_local[4].x = 1080;
	//Block_local[4].y = 430;
	//Block_local[4].width = 300;
	//Block_local[5].x = 1000;
	//Block_local[5].y = 330;
	//Block_local[5].width = 100;
	//Block_local[6].x = 1150;
	//Block_local[6].y = 260;
	//Block_local[6].width = 100;
	//Block_local[7].x = 1370;	// 여기 몬스터포인트로 하면 될 듯!
	//Block_local[7].y = 170;
	//Block_local[7].width = 900;
	//Block_local[8].x = 1800;
	//Block_local[8].y = 70;
	//Block_local[8].width = 100;
	//Block_local[9].x = 1600;
	//Block_local[9].y = 0;
	//Block_local[9].width = 100;
	//Block_local[10].x = 1400;
	//Block_local[10].y = -70;
	//Block_local[10].width = 100;
	//Block_local[11].x = 500;		// 여기도 큼지막한 블럭이긴 함 아 문을 둘까
	//Block_local[11].y = -140;
	//Block_local[11].width = 800;
	//Block_local[12].x = 0;
	//Block_local[12].y = -220;
	//Block_local[12].width = 400;
	//Block_local[13].x = 500;
	//Block_local[13].y = -270;
	//Block_local[13].width = 100;
	//Block_local[14].x = 670;
	//Block_local[14].y = -340;
	//Block_local[14].width = 100;
	//Block_local[15].x = 850;			// 몬스터 포인트?
	//Block_local[15].y = -400;
	//Block_local[15].width = 600;
	//Block_local[16].x = 1300;
	//Block_local[16].y = -460;
	//Block_local[16].width = 100;
	//Block_local[17].x = 1500;
	//Block_local[17].y = -530;
	//Block_local[17].width = 50;
	//Block_local[18].x = 1650;
	//Block_local[18].y = -600;
	//Block_local[18].width = 100;
	//Block_local[19].x = 1800;
	//Block_local[19].y = -700;
	//Block_local[19].width = 400;
	//Block_local[20].x = 1700;
	//Block_local[20].y = -770;
	//Block_local[20].width = 50;
	//Block_local[21].x = 1500;
	//Block_local[21].y = -850;
	//Block_local[21].width = 100;
	//Block_local[22].x = 1200;
	//Block_local[22].y = -950;
	//Block_local[22].width = 150;

	/*
	// 이동 블록 테스트용 -> static int로 선언해야 값 변경 반영됨
	static int Block_localX = 50;
	Block_local[23].y = 500;
	Block_local[23].width = 100;
	static int MoveBlock;	// 방향 변수 -> 짝수일 경우 왼쪽으로 이동, 아닐 경우 오른쪽
	// 여기 위까지 방해되면 주석 처리 해주세요
	*/
	/*
	static int Monster1_X;	//	아래쪽 몬스터 x좌표
	static int Monster2_X;	//	위쪽 몬스터 x좌표
	*/

	static int KillMonster1;
	static int KillMonster2;

	static int player_code;

	static COORD charPos = { 0,0 };
	static COORD pos = { 0,0 };
	static BYTE left = 0;
	static BYTE right = 0;
	static bool jump = 0;
	static int jumpCount = 0;
	static BYTE hp;
	static int window_left;
	static int window_bottom;
	
	switch (iMsg) {
	case WM_CREATE:
		PlaySound(L"start.wav", NULL, SND_ASYNC);

		BackGround.Load(L"BackGround.png");
		imgGround.Load(L"Ground.png");

		Start.Load(L"GameStart.png");

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
		Map.Load(L"Map.png");

		Damage[0].Load(L"Damage1.png");
		Damage[1].Load(L"Damage2.png");
		Damage[2].Load(L"Damage3.png");
		Damage[3].Load(L"Damage4.png");

		Block.Load(L"Block.png");
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

			imgSprite[0].width_stand[i]	= imgSprite[0].stand[i].GetWidth();
			imgSprite[0].height_stand[i] = imgSprite[0].stand[i].GetHeight();

			imgSprite[0].width_run[i] = imgSprite[0].run_L[i].GetWidth();
			imgSprite[0].height_run[i] = imgSprite[0].run_L[i].GetHeight();

			imgSprite[0].width_jump[i] = imgSprite[0].jump[i].GetWidth();
			imgSprite[0].height_jump[i] = imgSprite[0].jump[i].GetHeight();

			imgSprite[1].width_stand[i] =  imgSprite[1].stand[i].GetWidth();
			imgSprite[1].height_stand[i] = imgSprite[1].stand[i].GetHeight();
						 
			imgSprite[1].width_run[i] =	imgSprite[1].run_L[i].GetWidth();
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

		/*
		Monster1_X = Block_local[2].x - charPos.X + Block_local[2].width - 72;
		Monster2_X = Block_local[15].x - charPos.X;
		*/

		SetTimer(hWnd, 0, 50, NULL);

		break;

	case WM_PAINT:
		/*window_left = clamp(0, players[player_code].pos.X - 1280 / 2, bw - 1280 - 1);
		window_bottom = clamp(0, players[player_code].pos.Y - 800 / 2, bh - 800 - 1);*/

		pos = players[player_code].pos;
		charPos = players[player_code].charPos;
		left = players[player_code].left;
		right = players[player_code].right;
		jump = players[player_code].jump;
		jumpCount = players[player_code].jumpCount;
		hp = players[player_code].hp;

		hdc = BeginPaint(hWnd, &ps);

		if (count != 4) {
			count++;
		} 
		if (count == 4) {
			count = 0;
		}

		hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
		mem1dc = CreateCompatibleDC(hdc);
		SelectObject(mem1dc, hBitmap);

		//BackGround.Draw(mem1dc, 0, 0, window_left, window_bottom, 1280, 800, 0, 0);
		BackGround.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0 + charPos.X, bh - 1600 + charPos.Y, 2560, 1600);
		imgGround.Draw(mem1dc, 0 - charPos.X, 130 - charPos.Y, rect.right, rect.bottom, 0, 0, gw, gh);
		imgGround.Draw(mem1dc, rect.right - charPos.X, 130 - charPos.Y, rect.right, rect.bottom, 0, 0, gw, gh);

		Guide.Draw(mem1dc, 750 - charPos.X, 590 - charPos.Y, w_guide * 2 / 3, h_guide * 2 / 3, 0, 0, w_guide, h_guide);
		Portal.Draw(mem1dc, Portal_X - charPos.X, Portal_Y - charPos.Y - h_Portal * 1 / 4, w_Portal * 1 / 4, h_Portal * 1 / 4, 0, 0, w_Portal, h_Portal);

		/*
		for (int i = 0; i < BLOCKNUM; i++) {
			Block.Draw(mem1dc, Block_local[i].x - charPos.X, Block_local[i].y - charPos.Y, Block_local[i].width, 60, 0, 0, w_block, h_block);	// 벽돌-
		}
		*/
		//블록 28개까지
		//Block.Draw(mem1dc, Block_localX - charPos.X, Block_local[23].y - charPos.Y, Block_local[23].width, 60, 0, 0, w_block, h_block);
		hBitmap2 = CreateCompatibleBitmap(mem1dc, rect.right, rect.bottom);
		mem2dc = CreateCompatibleDC(mem1dc);
		SelectObject(mem2dc, hBitmap2);

		Eblock[0].Draw(mem2dc, 600 - charPos.X, 660 - charPos.Y, w_eblock / 3, h_eblock / 3, 0, 0, w_eblock, h_eblock);

		SelectObject(mem1dc, hBitmap);

		if (GetPixel(mem2dc, players[player_code].pos.X + 40, players[player_code].pos.Y + 40) != RGB(254, 0, 0)) {
			Switch = 0;
			Eblock[0].Draw(mem1dc, 600 - charPos.X, 660 - charPos.Y, w_eblock / 3, h_eblock / 3, 0, 0, w_eblock, h_eblock);
		}

		else if (GetPixel(mem2dc, players[player_code].pos.X + 40, players[player_code].pos.Y) == RGB(254, 0, 0)) {
			Switch = 1;
			Eblock[1].Draw(mem1dc, 600 - charPos.X, 660 - charPos.Y, w_eblock / 3, h_eblock / 3, 0, 0, w_eblock, h_eblock);
		}
		/*
		// 블록 이동 코드 (현재 사용 x)
		if (Switch == 1) {
			if (MoveBlock % 2 == 0) {
				if (Block_localX - charPos.X >= 0) {
					Block_localX -= 5;
				}

				else {
					MoveBlock++;
				}
			}

			else {
				if (Block_localX + charPos.X <= rect.right) {
					Block_localX += 5;
				}

				else {
					MoveBlock++;
				}
			}
		}
		*/
		//	몬스터가 블록 왼쪽, 오른쪽 끝에 도달하면 방향 바꾸는 코드
		/*
		if (KillMonster1 == 0) {
			if (MonsterTurn[0] % 2 == 0) {
				Monster_L[count].Draw(mem1dc, Monster_X[0], Block_local[2].y - charPos.Y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);
			}

			if (MonsterTurn[0] % 2 != 0) {
				Monster_R[count].Draw(mem1dc, Monster_X[0], Block_local[2].y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);
			}
		}
		*/
		/*if (KillMonster1 == 0) {
			if (Monster1_X < Block_local[2].x - charPos.X || Monster1_X + w_monster[count] / 2 > Block_local[2].x - charPos.X + Block_local[2].width) {
				if (Monster1_X < Block_local[2].x - charPos.X) {
					Monster1_X = Block_local[2].x - charPos.X;
				}

				if (Monster1_X + w_monster[count] / 2 > Block_local[2].x - charPos.X + Block_local[2].width) {
					Monster1_X = Block_local[2].x - charPos.X + Block_local[2].width - w_monster[count] / 2;
				}

				Monster1Turn++;
			}

			//	몬스터가 오른쪽으로 이동할 때, 캐릭터가 오른쪽에서 부딪히면 하트 깎임... 등지고 있을 때도 깎이게 할까 고민 중

			if (Monster1Turn % 2 == 0) {
				Monster_L[count].Draw(mem1dc, Monster1_X, Block_local[2].y - charPos.Y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (players[player_code].pos.X + w1_stand[count] / 2 > Monster1_X && players[player_code].pos.X < Monster1_X && players[player_code].pos.Y + h1_stand[count] / 2 > Block_local[2].y - charPos.Y - h_monster[count] / 2 && players[player_code].pos.Y + h1_stand[count] / 2 < Block_local[2].y - charPos.Y + 60 && jump == 0) {
					hit++;
					DamageNum = 1;
					SetTimer(hWnd, 1, 700, NULL);

					if (hit != 0) {
						if (hit == 1) {
							heart--;
						}

						protect++;
					}

					//	protect가 20이 되면 무적 해제, 부딪힐 경우 피 깎임

					if (protect == 20) {
						protect = 0;
						hit = 0;
					}
				}

				Monster1_X -= 5;
			}

			if (Monster1Turn % 2 != 0) {
				Monster_R[count].Draw(mem1dc, Monster1_X, Block_local[2].y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (players[player_code].pos.X < Monster1_X + h_monster[count] / 2 && players[player_code].pos.X + w1_stand[count] / 2 > Monster1_X && players[player_code].pos.Y + h1_stand[count] / 2 > Block_local[2].y - charPos.Y - h_monster[count] / 2 && players[player_code].pos.Y + h1_stand[count] / 2 < Block_local[2].y - charPos.Y + 60 && jump == 0) {
					hit++;
					DamageNum = 1;
					SetTimer(hWnd, 1, 700, NULL);

					if (hit != 0) {
						if (hit == 1) {
							heart--;
						}

						protect++;
					}

					if (protect == 20) {
						protect = 0;
						hit = 0;
					}
				}

				Monster1_X += 5;
			}
		}
		
		//	위랑 똑같은 내용인데 위쪽 몬스터

		if (KillMonster2 == 0) {
			if (Monster2_X < Block_local[15].x - charPos.X || Monster2_X + w_monster[count] / 2 > Block_local[15].x - charPos.X + Block_local[15].width) {
				if (Monster2_X < Block_local[15].x - charPos.X) {
					Monster2_X = Block_local[15].x - charPos.X;
				}

				if (Monster2_X + w_monster[count] / 2 > Block_local[15].x - Block_local[15].width) {
					Monster2_X = Block_local[15].x - Block_local[15].width - w_monster[count] / 2;
				}

				Monster2Turn++;
			}

			if (Monster2Turn % 2 != 0) {
				Monster_L[count].Draw(mem1dc, Monster2_X, Block_local[15].y - charPos.Y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (players[player_code].pos.X + w1_stand[count] / 2 > Monster2_X && players[player_code].pos.X < Monster2_X && players[player_code].pos.Y + w1_stand[count] / 2 > Block_local[15].y - charPos.Y - h_monster[count] / 2 && players[player_code].pos.Y + h1_stand[count] / 2 < Block_local[15].y - charPos.Y + 60 && jump == 0) {
					hit++;
					DamageNum = 1;
					SetTimer(hWnd, 1, 700, NULL);

					if (hit != 0) {
						if (hit == 1) {
							heart--;
						}

						protect++;
					}

					if (protect == 20) {
						protect = 0;
						hit = 0;
					}
				}

				Monster2_X -= 5;
			}

			if (Monster2Turn % 2 == 0) {
				Monster_R[count].Draw(mem1dc, Monster2_X, Block_local[15].y - charPos.Y - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (players[player_code].pos.X < Monster2_X + h_monster[count] / 2 && players[player_code].pos.X + w1_stand[count] / 2 > Monster2_X && players[player_code].pos.Y + h1_stand[count] / 2 > Block_local[15].y - charPos.Y - h_monster[count] / 2 && players[player_code].pos.Y + h1_stand[count] / 2 < Block_local[15].y - charPos.Y + 60 && jump == 0) {
					hit++;
					DamageNum = 1;
					SetTimer(hWnd, 1, 700, NULL);

					if (hit != 0) {
						if (hit == 1) {
							heart--;
						}

						protect++;
					}

					if (protect == 20) {
						protect = 0;
						hit = 0;
					}
				}

				Monster2_X += 5;
			}
		}*/
/*
		if (Key_Image == 1)
			Key.Draw(mem1dc, Key_X - charPos.X, Key_Y - charPos.Y, w_Key * 1 / 2, h_Key * 1 / 2, 0, 0, w_Key, h_Key);

		if (Key_Image == 0) {
			Key.Draw(mem1dc, 1170, 0, w_Key * 1 / 2, h_Key * 1 / 2, 0, 0, w_Key, h_Key);
		}

		if (players[player_code].pos.X >= Key_X - charPos.X && players[player_code].pos.X <= Key_X - charPos.X + w_Key * 1 / 2 && players[player_code].pos.Y >= Key_Y - charPos.Y && players[player_code].pos.Y <= Key_Y + h_Key * 1 / 2) {
			Key_Image = 0;
		}
*/
		// 내캐릭터 그리기
		if (left == 0 && right == 0 && jump == 0)
			imgSprite[player_code].stand[count].Draw(mem1dc, pos.X, pos.Y, imgSprite[player_code].width_stand[count] / 2, imgSprite[player_code].height_stand[count] / 2,
				0, 0, imgSprite[player_code].width_stand[count], imgSprite[player_code].height_stand[count]);
		else if (jump == 1)
			imgSprite[player_code].jump[count].Draw(mem1dc, pos.X, pos.Y, imgSprite[player_code].width_jump[count] / 2, imgSprite[player_code].height_jump[count] / 2,
				0, 0, imgSprite[player_code].width_jump[count], imgSprite[player_code].height_jump[count]);
		else if (left == 1)
			imgSprite[player_code].run_L[count].Draw(mem1dc, pos.X, pos.Y, imgSprite[player_code].width_run[count] / 2, imgSprite[player_code].height_run[count] / 2,
				0, 0, imgSprite[player_code].width_run[count], imgSprite[player_code].height_run[count]);
		else if (right == 1)
			imgSprite[player_code].run_R[count].Draw(mem1dc, pos.X, pos.Y, imgSprite[player_code].width_run[count] / 2, imgSprite[player_code].height_run[count] / 2,
				0, 0, imgSprite[player_code].width_run[count], imgSprite[player_code].height_run[count]);
		

		//다른 캐릭터들 그리기
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (i != player_code) {
				if (players[i].left == 0 && players[i].right == 0 && players[i].jump == 0)
					imgSprite[i].stand[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_stand[count] / 2, imgSprite[i].height_stand[count] / 2,
						0, 0, imgSprite[i].width_stand[count], imgSprite[i].height_stand[count]);
				else if (players[i].jump == 1) 
					imgSprite[i].jump[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_jump[count] / 2, imgSprite[i].height_jump[count] / 2,
						0, 0, imgSprite[i].width_jump[count], imgSprite[i].height_jump[count]);
				else if(players[i].left == 1)
					imgSprite[i].run_L[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_run[count] / 2, imgSprite[i].height_run[count] / 2,
						0, 0, imgSprite[i].width_run[count], imgSprite[i].height_run[count]);
				else if(players[i].right == 1)
					imgSprite[i].run_R[count].Draw(mem1dc, players[i].pos.X - charPos.X, players[i].pos.Y - charPos.Y, imgSprite[i].width_run[count] / 2, imgSprite[i].height_run[count] / 2,
						0, 0, imgSprite[i].width_run[count], imgSprite[i].height_run[count]);
			}
		}
		
		/*
		if (Image_Number == 0) {
			Start.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
		}

		if (Image_Number != 0 && Image_Number < 4) {
			Dialog[Image_Number].Draw(mem1dc, 0, 100, rect.right, rect.bottom, 0, 0, 1280, 800);
		}

		if (Image_Number == 5) {
			Dialog[Image_Number].Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
		}

		if (DamageNum == 1) {
			if (Monster1Turn % 2 == 0 && Monster2Turn % 2 != 0) {
				Damage[count].Draw(mem1dc, players[player_code].pos.X + 20, players[player_code].pos.Y, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);
			}

			if (Monster1Turn % 2 != 0 && Monster2Turn % 2 == 0) {
				Damage[count].Draw(mem1dc, players[player_code].pos.X - 20, players[player_code].pos.Y, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);
			}

			if (count == 3) {
				DamageNum = 0;
			}
		}

		if (KillNum == 1) {
			Damage[count].Draw(mem1dc, players[player_code].pos.X, players[player_code].pos.Y + 70, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);

			if (count == 3) {
				KillNum = 0;
			}
		}
		*/
		/*
		if (jump == 1) {
			jumpcount++;

			if (jumpcount < 10) {
				y -= 5;
				if (CharY <= 800)
					CharY -= 8;
				else if (CharY > 800)
					CharY -= 14;
			}

			else if (jumpcount < 20) {
				y += 5;

				if (CharY <= 800)
					CharY += 8;
				else if (CharY > 800)
					CharY += 10;

			}

			else {
				if (GetPixel(mem1dc, x + 40, y + 70) == RGB(37, 176, 77)) {
					OnBlock = 1;
					jumpcount = 0;
					jump = 0;
				}
				else {
					jumpcount = 10;
				}
			}
		}


		if (GetPixel(mem1dc, x + 40, y + 70) == RGB(37, 176, 77) || y == 620) {
			OnBlock = 1;
			jump = 0;
			jumpcount = 0;
		}

		if (GetPixel(mem1dc, x + 40, y + 70) == RGB(243, 216, 19)) {
			if (y < Block_local[7].y - CharY && y > Block_local[15].y - CharY) {
				KillMonster1 = 1;
				KillNum = 1;
			}
			else {
				KillMonster2 = 1;
				KillNum = 1;
			}
		}
		
		// 중력 처리 하려고 했던 부분
		if (jump == 0 && GetPixel(mem1dc, x, y + 70) != RGB(37, 176, 77) && y != 620) {
			OnBlock = 0;
		}

		if (OnBlock == 0 && jump == 0) {
			jump = 1;
			jumpcount = 10;
		}
		*/

		//if (click != 0) {
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

			/*else if (hp == 0) {
				GameOver.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			}*/
		//}
		
		if (MapNum == 1) {
			Map.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
		}

		if (Image_Number == 7) {
			if (ClearCount == 0) {
				ClearCount = 1;
			}

			else ClearCount = 0;

			Clear[ClearCount].Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
		}

		if (Image_Number == 8) {
			Guide2.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
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
		
		if (wParam == 'A' || wParam == 'a') {
			msg = CS_KEYUP_A;
		}


		if (wParam == 'D' || wParam == 'd') {
			msg = CS_KEYUP_D;
		}

		if (wParam == VK_SPACE) {
			msg = CS_KEYUP_SPACE;
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_SPACE) {
			msg = CS_KEYDOWN_SPACE;
		}

		if (wParam == 'A' || wParam == 'a') {
			msg = CS_KEYDOWN_A;
		}


		if (wParam == 'D' || wParam == 'd') {
			msg = CS_KEYDOWN_D;
		}
		if (wParam == 'W' || wParam == 'w') {
			msg = CS_KEYDOWN_W;
		}

		/*
		if (wParam == 'W' || wParam == 'w') {
			if (x >= Portal_X - charPos.X && x <= Portal_X - charPos.X + w_Portal * 1 / 4) {
				if (Key_Image == 0) {
					Image_Number = 7;
				}

				else if (Image_Number == 8) {
					Image_Number = 6;
				}

				else Image_Number = 8;
			}
		}
		*/
		break;

	case WM_LBUTTONDOWN:
		MouseX = LOWORD(lParam);
		MouseY = HIWORD(lParam);

		click++;

		if (click == 1) {
			//PlaySound(L"OST.wav", NULL, SND_ASYNC | SND_LOOP);
		}

		if (Image_Number >= 0 && Image_Number < 4)
			Image_Number++;

		if (MouseX >= 750 - players[player_code].charPos.X && MouseX <= 750 - players[player_code].charPos.X + w_guide * 2 / 3 && MouseY >= 590 - players[player_code].charPos.Y && MouseY <= 590 - players[player_code].charPos.Y + h_guide * 2 / 3) {
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