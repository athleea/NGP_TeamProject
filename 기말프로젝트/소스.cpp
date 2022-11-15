#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <atlImage.h>
#pragma comment(lib, "winmm")
#include <mmsystem.h>

using namespace std;

LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"windows program";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

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

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
}

struct BLOCK {
	int x;
	int y;
	int width;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, mem1dc;
	PAINTSTRUCT ps;

	COLORREF color;

	BLOCK Block_local[25] = { 0 };

	static HBITMAP hBitmap;
	static CImage BackGround, imgGround;
	static CImage imgSprite1[4], imgSprite1_runR[4], imgSprite1_runL[4], imgSprite1_jump[4];
	static CImage imgSprite2[4], imgSprite2_runR[4], imgSprite2_runL[4], imgSprite2_jump[4];
	static CImage imgSprite3[4], imgSprite3_runR[4], imgSprite3_runL[4], imgSprite3_jump[4];
	static CImage Start, Dialog[6], Guide, Block, Heart, Key, Portal, Clear[2], Guide2, GameOver, Monster_L[4], Monster_R[4], Map, Damage[4];

	static RECT rect;

	static int w_stand[10], h_stand[10];
	static int w_run[4], h_run[4];
	static int w_jump[4], h_jump[4];
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
	static int OnBlock = 0;
	static int w_heart, h_heart;
	static int w_Key, h_Key;
	static int w_Portal, h_Portal;
	static int w_monster[4], h_monster[4];
	static int ClearCount = 0;
	static int DamageNum = 0;
	static int KillNum = 0;

	static int bw, bh, gw, gh;

	static int x = 640;
	static int y = 620;

	static int CharX, CharY;

	static int count = 0;

	static int jump = 0;
	static int jumpcount = 0;

	static int left, right, last;

	static int CharNum = 1;
	static int click = 0;
	static int heart = 3;

	static int blockNum;

	static int Monster1Turn;	//	아래쪽 몬스터, 해당 변수가 짝수인지 홀수인지에 따라 방향 바뀜
	static int Monster2Turn;	//	위쪽 몬스터

	static int hit;	//	몬스터 충돌 횟수
	static int protect;	//	처음 충돌 후 일정 시간 무적, 데미지 받지 않음. 이거 판단하는 변수

	static int Key_X = 1200;	// Key 위치 수정 필요
	static int Key_Y = -1020;
	static int Key_Image = 1;
	static int Heart_Click = 0;
	static int MapNum = 0;

	static int Portal_X = 800;		// Portal 위치 수정 필요
	static int Portal_Y = -120;

	Block_local[0].x = 150;
	Block_local[0].y = 600;
	Block_local[0].width = 50;
	Block_local[1].x = 250;
	Block_local[1].y = 550;
	Block_local[1].width = 200;
	Block_local[2].x = 500;
	Block_local[2].y = 470;
	Block_local[2].width = 70;
	Block_local[3].x = 700;
	Block_local[3].y = 510;
	Block_local[3].width = 300;
	Block_local[4].x = 1080;
	Block_local[4].y = 430;
	Block_local[4].width = 300;
	Block_local[5].x = 1000;
	Block_local[5].y = 330;
	Block_local[5].width = 100;
	Block_local[6].x = 1150;
	Block_local[6].y = 260;
	Block_local[6].width = 100;
	Block_local[7].x = 1370;	// 여기 몬스터포인트로 하면 될 듯!
	Block_local[7].y = 170;
	Block_local[7].width = 900;
	Block_local[8].x = 1800;
	Block_local[8].y = 70;
	Block_local[8].width = 100;
	Block_local[9].x = 1600;
	Block_local[9].y = 0;
	Block_local[9].width = 100;
	Block_local[10].x = 1400;
	Block_local[10].y = -70;
	Block_local[10].width = 100;
	Block_local[11].x = 500;		// 여기도 큼지막한 블럭이긴 함 아 문을 둘까
	Block_local[11].y = -140;
	Block_local[11].width = 800;
	Block_local[12].x = 0;
	Block_local[12].y = -220;
	Block_local[12].width = 400;
	Block_local[13].x = 500;
	Block_local[13].y = -270;
	Block_local[13].width = 100;
	Block_local[14].x = 670;
	Block_local[14].y = -340;
	Block_local[14].width = 100;
	Block_local[15].x = 850;			// 몬스터 포인트?
	Block_local[15].y = -400;
	Block_local[15].width = 600;
	Block_local[16].x = 1300;
	Block_local[16].y = -460;
	Block_local[16].width = 100;
	Block_local[17].x = 1500;
	Block_local[17].y = -530;
	Block_local[17].width = 50;
	Block_local[18].x = 1650;
	Block_local[18].y = -600;
	Block_local[18].width = 100;
	Block_local[19].x = 1800;
	Block_local[19].y = -700;
	Block_local[19].width = 400;
	Block_local[20].x = 1700;
	Block_local[20].y = -770;
	Block_local[20].width = 50;
	Block_local[21].x = 1500;
	Block_local[21].y = -850;
	Block_local[21].width = 100;
	Block_local[22].x = 1200;
	Block_local[22].y = -950;
	Block_local[22].width = 150;

	// 이동 블록 테스트용 -> static int로 선언해야 값 변경 반영됨
	static int Block_localX = 50;
	Block_local[23].y = 500;
	Block_local[23].width = 100;
	static int MoveBlock;	// 방향 변수 -> 짝수일 경우 왼쪽으로 이동, 아닐 경우 오른쪽
	// 여기 위까지 방해되면 주석 처리 해주세요

	blockNum = 23;

	static int Monster1_X;	//	아래쪽 몬스터 x좌표
	static int Monster2_X;	//	위쪽 몬스터 x좌표

	static int KillMonster1;
	static int KillMonster2;

	static bool aDown = false;
	static bool dDown = false;
	static bool spaceDown = false;

	switch (iMsg) {
	case WM_CREATE:
		PlaySound(L"start.wav", NULL, SND_ASYNC);

		BackGround.Load(L"BackGround.png");
		imgGround.Load(L"Ground.png");

		Start.Load(L"GameStart.png");

		imgSprite1[0].Load(L"cookie1-stand1.png");
		imgSprite1[1].Load(L"cookie1-stand1.png");
		imgSprite1[2].Load(L"cookie1-stand2.png");
		imgSprite1[3].Load(L"cookie1-stand2.png");

		imgSprite1_runL[0].Load(L"cookie1-runL1.png");
		imgSprite1_runL[1].Load(L"cookie1-runL2.png");
		imgSprite1_runL[2].Load(L"cookie1-runL3.png");
		imgSprite1_runL[3].Load(L"cookie1-runL4.png");

		imgSprite1_runR[0].Load(L"cookie1-runR1.png");
		imgSprite1_runR[1].Load(L"cookie1-runR2.png");
		imgSprite1_runR[2].Load(L"cookie1-runR3.png");
		imgSprite1_runR[3].Load(L"cookie1-runR4.png");

		imgSprite1_jump[0].Load(L"cookie1-jump1.png");
		imgSprite1_jump[1].Load(L"cookie1-jump2.png");
		imgSprite1_jump[2].Load(L"cookie1-jump3.png");
		imgSprite1_jump[3].Load(L"cookie1-jump4.png");

		imgSprite2[0].Load(L"cookie2-stand1.png");
		imgSprite2[1].Load(L"cookie2-stand1.png");
		imgSprite2[2].Load(L"cookie2-stand2.png");
		imgSprite2[3].Load(L"cookie2-stand2.png");

		imgSprite2_runL[0].Load(L"cookie2-runL1.png");
		imgSprite2_runL[1].Load(L"cookie2-runL2.png");
		imgSprite2_runL[2].Load(L"cookie2-runL3.png");
		imgSprite2_runL[3].Load(L"cookie2-runL4.png");

		imgSprite2_runR[0].Load(L"cookie2-runR1.png");
		imgSprite2_runR[1].Load(L"cookie2-runR2.png");
		imgSprite2_runR[2].Load(L"cookie2-runR3.png");
		imgSprite2_runR[3].Load(L"cookie2-runR4.png");

		imgSprite2_jump[0].Load(L"cookie2-jump1.png");
		imgSprite2_jump[1].Load(L"cookie2-jump2.png");
		imgSprite2_jump[2].Load(L"cookie2-jump3.png");
		imgSprite2_jump[3].Load(L"cookie2-jump4.png");

		imgSprite3[0].Load(L"cookie3-stand1.png");
		imgSprite3[1].Load(L"cookie3-stand1.png");
		imgSprite3[2].Load(L"cookie3-stand2.png");
		imgSprite3[3].Load(L"cookie3-stand2.png");

		imgSprite3_runL[0].Load(L"cookie3-runL1.png");
		imgSprite3_runL[1].Load(L"cookie3-runL2.png");
		imgSprite3_runL[2].Load(L"cookie3-runL3.png");
		imgSprite3_runL[3].Load(L"cookie3-runL4.png");

		imgSprite3_runR[0].Load(L"cookie3-runR1.png");
		imgSprite3_runR[1].Load(L"cookie3-runR2.png");
		imgSprite3_runR[2].Load(L"cookie3-runR3.png");
		imgSprite3_runR[3].Load(L"cookie3-runR4.png");

		imgSprite3_jump[0].Load(L"cookie3-jump1.png");
		imgSprite3_jump[1].Load(L"cookie3-jump2.png");
		imgSprite3_jump[2].Load(L"cookie3-jump3.png");
		imgSprite3_jump[3].Load(L"cookie3-jump4.png");

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

		GetClientRect(hWnd, &rect);

		bw = BackGround.GetWidth();
		bh = BackGround.GetHeight();
		gw = imgGround.GetWidth();
		gh = imgGround.GetHeight();

		w_guide = Guide.GetWidth();
		h_guide = Guide.GetHeight();

		w_block = Block.GetWidth();
		h_block = Block.GetHeight();

		w_heart = Heart.GetWidth();
		h_heart = Heart.GetHeight();

		w_Key = Key.GetWidth();
		h_Key = Key.GetHeight();

		w_Portal = Portal.GetWidth();
		h_Portal = Portal.GetHeight();

		for (int i = 0; i < 4; ++i) {
			w_damage[i] = Damage[i].GetWidth();
			h_damage[i] = Damage[i].GetHeight();

			w1_stand[i] = imgSprite1[i].GetWidth();
			h1_stand[i] = imgSprite1[i].GetHeight();

			w1_run[i] = imgSprite1_runL[i].GetWidth();
			h1_run[i] = imgSprite1_runL[i].GetHeight();

			w1_jump[i] = imgSprite1_jump[i].GetWidth();
			h1_jump[i] = imgSprite1_jump[i].GetHeight();

			w2_stand[i] = imgSprite2[i].GetWidth();
			h2_stand[i] = imgSprite2[i].GetHeight();

			w2_run[i] = imgSprite2_runL[i].GetWidth();
			h2_run[i] = imgSprite2_runL[i].GetHeight();

			w2_jump[i] = imgSprite2_jump[i].GetWidth();
			h2_jump[i] = imgSprite2_jump[i].GetHeight();

			w3_stand[i] = imgSprite3[i].GetWidth();
			h3_stand[i] = imgSprite3[i].GetHeight();

			w3_run[i] = imgSprite3_runL[i].GetWidth();
			h3_run[i] = imgSprite3_runL[i].GetHeight();

			w3_jump[i] = imgSprite3_jump[i].GetWidth();
			h3_jump[i] = imgSprite3_jump[i].GetHeight();

			w_monster[i] = Monster_L[i].GetWidth();
			h_monster[i] = Monster_R[i].GetHeight();
		}

		Monster1_X = Block_local[7].x - CharX + Block_local[7].width - 72;
		Monster2_X = Block_local[15].x - CharX;

		SetTimer(hWnd, 0, 50, NULL);

		break;

	case WM_PAINT:
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

		BackGround.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0 + CharX, bh - 1600 + CharY, 2560, 1600);
		imgGround.Draw(mem1dc, 0 - CharX, 130 - CharY, rect.right, rect.bottom, 0, 0, gw, gh);
		imgGround.Draw(mem1dc, rect.right - CharX, 130 - CharY, rect.right, rect.bottom, 0, 0, gw, gh);

		Guide.Draw(mem1dc, 750 - CharX, 590 - CharY, w_guide * 2 / 3, h_guide * 2 / 3, 0, 0, w_guide, h_guide);

		Portal.Draw(mem1dc, Portal_X - CharX, Portal_Y - CharY - h_Portal * 1 / 4, w_Portal * 1 / 4, h_Portal * 1 / 4, 0, 0, w_Portal, h_Portal);

		for (int i = 0; i < blockNum; i++) {
			Block.Draw(mem1dc, Block_local[i].x - CharX, Block_local[i].y - CharY, Block_local[i].width, 60, 0, 0, w_block, h_block);	// 벽돌-
		}

		Block.Draw(mem1dc, Block_localX - CharX, Block_local[23].y - CharY, Block_local[23].width, 60, 0, 0, w_block, h_block);

		// 블록 이동 코드 (현재 사용 x)
		if (MoveBlock % 2 == 0) {
			if (Block_localX - CharX >= 0) {
				Block_localX -= 5;
			}

			else {
				MoveBlock++;
			}
		}
		
		else {
			if (Block_localX + CharX <= rect.right) {
				Block_localX += 5;
			}

			else {
				MoveBlock++;
			}
		}

		//	몬스터가 블록 왼쪽, 오른쪽 끝에 도달하면 방향 바꾸는 코드

		if (KillMonster1 == 0) {
			if (Monster1_X < Block_local[7].x - CharX || Monster1_X + w_monster[count] / 2 > Block_local[7].x - CharX + Block_local[7].width) {
				if (Monster1_X < Block_local[7].x - CharX) {
					Monster1_X = Block_local[7].x - CharX;
				}

				if (Monster1_X + w_monster[count] / 2 > Block_local[7].x - CharX + Block_local[7].width) {
					Monster1_X = Block_local[7].x - CharX + Block_local[7].width - w_monster[count] / 2;
				}

				Monster1Turn++;
			}

			//	몬스터가 오른쪽으로 이동할 때, 캐릭터가 오른쪽에서 부딪히면 하트 깎임... 등지고 있을 때도 깎이게 할까 고민 중

			if (Monster1Turn % 2 == 0) {
				Monster_L[count].Draw(mem1dc, Monster1_X, Block_local[7].y - CharY - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (x + w1_stand[count] / 2 > Monster1_X && x < Monster1_X && y + h1_stand[count] / 2 > Block_local[7].y - CharY - h_monster[count] / 2 && y + h1_stand[count] / 2 < Block_local[7].y - CharY + 60 && jump == 0) {
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
				Monster_R[count].Draw(mem1dc, Monster1_X, Block_local[7].y - CharY - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (x < Monster1_X + h_monster[count] / 2 && x + w1_stand[count] / 2 > Monster1_X && y + h1_stand[count] / 2 > Block_local[7].y - CharY - h_monster[count] / 2 && y + h1_stand[count] / 2 < Block_local[7].y - CharY + 60 && jump == 0) {
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
			if (Monster2_X < Block_local[15].x - CharX || Monster2_X + w_monster[count] / 2 > Block_local[15].x - CharX + Block_local[15].width) {
				if (Monster2_X < Block_local[15].x - CharX) {
					Monster2_X = Block_local[15].x - CharX;
				}

				if (Monster2_X + w_monster[count] / 2 > Block_local[15].x - CharX + Block_local[15].width) {
					Monster2_X = Block_local[15].x - CharX + Block_local[15].width - w_monster[count] / 2;
				}

				Monster2Turn++;
			}

			if (Monster2Turn % 2 != 0) {
				Monster_L[count].Draw(mem1dc, Monster2_X, Block_local[15].y - CharY - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (x + w1_stand[count] / 2 > Monster2_X && x < Monster2_X && y + w1_stand[count] / 2 > Block_local[15].y - CharY - h_monster[count] / 2 && y + h1_stand[count] / 2 < Block_local[15].y - CharY + 60 && jump == 0) {
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
				Monster_R[count].Draw(mem1dc, Monster2_X, Block_local[15].y - CharY - h_monster[count] / 2, w_monster[count] / 2, h_monster[count] / 2, 0, 0, w_monster[count], h_monster[count]);

				if (x < Monster2_X + h_monster[count] / 2 && x + w1_stand[count] / 2 > Monster2_X && y + h1_stand[count] / 2 > Block_local[15].y - CharY - h_monster[count] / 2 && y + h1_stand[count] / 2 < Block_local[15].y - CharY + 60 && jump == 0) {
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
		}

		if (Key_Image == 1)
			Key.Draw(mem1dc, Key_X - CharX, Key_Y - CharY, w_Key * 1 / 2, h_Key * 1 / 2, 0, 0, w_Key, h_Key);

		if (Key_Image == 0) {
			Key.Draw(mem1dc, 1170, 0, w_Key * 1 / 2, h_Key * 1 / 2, 0, 0, w_Key, h_Key);
		}

		if (x >= Key_X - CharX && x <= Key_X - CharX + w_Key * 1 / 2 && y >= Key_Y - CharY && y <= Key_Y - CharY + h_Key * 1 / 2) {
			Key_Image = 0;
		}

		if (CharNum == 1) {
			if (left == 0 && right == 0 && jump == 0) {
				imgSprite1[count].Draw(mem1dc, x, y, w1_stand[count] / 2, h1_stand[count] / 2, 0, 0, w1_stand[count], h1_stand[count]);
			}

			if (left == 1 && jump == 0) {
				imgSprite1_runL[count].Draw(mem1dc, x, y, w1_run[count] / 2, h1_run[count] / 2, 0, 0, w1_run[count], h1_run[count]);
			}

			if (right == 1 && jump == 0) {
				imgSprite1_runR[count].Draw(mem1dc, x, y, w1_run[count] / 2, h1_run[count] / 2, 0, 0, w1_run[count], h1_run[count]);
			}

			if (jump == 1) {
				imgSprite1_jump[count].Draw(mem1dc, x, y, w1_jump[count] / 2, h1_jump[count] / 2, 0, 0, w1_jump[count], h1_jump[count]);
			}
		}

		if (CharNum == 2) {
			if (left == 0 && right == 0 && jump == 0) {
				imgSprite2[count].Draw(mem1dc, x, y, w2_stand[count] / 2, h2_stand[count] / 2, 0, 0, w2_stand[count], h2_stand[count]);
			}

			if (left == 1 && jump == 0) {
				imgSprite2_runL[count].Draw(mem1dc, x, y, w2_run[count] / 2, h2_run[count] / 2, 0, 0, w2_run[count], h2_run[count]);
			}

			if (right == 1 && jump == 0) {
				imgSprite2_runR[count].Draw(mem1dc, x, y, w2_run[count] / 2, h2_run[count] / 2, 0, 0, w2_run[count], h2_run[count]);
			}

			if (jump == 1) {
				imgSprite2_jump[count].Draw(mem1dc, x, y, w2_jump[count] / 2, h2_jump[count] / 2, 0, 0, w2_jump[count], h2_jump[count]);
			}
		}

		if (CharNum == 3) {
			if (left == 0 && right == 0 && jump == 0) {
				imgSprite3[count].Draw(mem1dc, x, y, w3_stand[count] / 2, h3_stand[count] / 2, 0, 0, w3_stand[count], h3_stand[count]);
			}

			if (left == 1 && jump == 0) {
				imgSprite3_runL[count].Draw(mem1dc, x, y, w3_run[count] / 2, h3_run[count] / 2, 0, 0, w3_run[count], h3_run[count]);
			}

			if (right == 1 && jump == 0) {
				imgSprite3_runR[count].Draw(mem1dc, x, y, w3_run[count] / 2, h3_run[count] / 2, 0, 0, w3_run[count], h3_run[count]);
			}

			if (jump == 1) {
				imgSprite3_jump[count].Draw(mem1dc, x, y, w3_jump[count] / 2, h3_jump[count] / 2, 0, 0, w3_jump[count], h3_jump[count]);
			}
		}

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
				Damage[count].Draw(mem1dc, x + 20, y, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);
			}

			if (Monster1Turn % 2 != 0 && Monster2Turn % 2 == 0) {
				Damage[count].Draw(mem1dc, x - 20, y, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);
			}

			if (count == 3) {
				DamageNum = 0;
			}
		}

		if (KillNum == 1) {
			Damage[count].Draw(mem1dc, x, y + 70, w_damage[count], h_damage[count], 0, 0, w_damage[count], h_damage[count]);

			if (count == 3) {
				KillNum = 0;
			}
		}

		if (true == aDown) {
			if (x >= rect.left) {
				x -= 5;
			}

			if (0 + CharX > 0) {
				CharX -= 5;
			}

			left = 1;
			right = 0;
			last = 1;
		}

		if (true == dDown) {
			x += 5;
			if (x + w3_stand[count] + 5 >= rect.right) {
				x -= 5;
			}

			if (CharX - w_stand[count] < bw - 2560 - 130) {
				CharX += 5;
			}

			left = 0;
			right = 1;
			last = 2;
		}

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

		if (jump == 0 && GetPixel(mem1dc, x, y + 70) != RGB(37, 176, 77) && y != 620) {
			OnBlock = 0;
		}

		if (OnBlock == 0 && jump == 0) {
			jump = 1;
			jumpcount = 10;
		}

		if (click != 0) {
			if (heart == 3) {
				Heart.Draw(mem1dc, 10, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
				Heart.Draw(mem1dc, 20 + w_heart / 120, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
				Heart.Draw(mem1dc, 30 + w_heart / 60, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
			}

			if (heart == 2) {
				Heart.Draw(mem1dc, 10, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
				Heart.Draw(mem1dc, 20 + w_heart / 120, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
			}

			if (heart == 1) {
				Heart.Draw(mem1dc, 10, 10, w_heart / 120, h_heart / 120, 0, 0, w_heart, h_heart);
			}

			if (heart == 0) {
				GameOver.Draw(mem1dc, 0, 0, rect.right, rect.bottom, 0, 0, 1280, 800);
			}
		}

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
		EndPaint(hWnd, &ps);

		printf("(%d, %d), block : %d\n", x, y, OnBlock);

		break;

	case WM_LBUTTONDOWN:
		MouseX = LOWORD(lParam);
		MouseY = HIWORD(lParam);

		click++;

		if (Heart_Click == 0 && heart == 0 && click != 0) {
			Heart_Click++;
		}

		if (Heart_Click == 1) {
			CharNum = 1;
			click = 0;
			heart = 3;
			x = 640;
			y = 620;
			count = 0;

			jump = 0;
			jumpcount = 0;
			CharX = 0, CharY = 0;
		}

		if (click == 1) {
			PlaySound(L"OST.wav", NULL, SND_ASYNC | SND_LOOP);
		}

		if (Image_Number >= 0 && Image_Number < 4)
			Image_Number++;

		if (MouseX >= 750 - CharX && MouseX <= 750 - CharX + w_guide * 2 / 3 && MouseY >= 590 - CharY && MouseY <= 590 - CharY + h_guide * 2 / 3) {
			Image_Number = 5;
		}

		else if (Image_Number == 5)
			Image_Number = 4;

		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_TIMER:
		InvalidateRect(hWnd, NULL, FALSE);

		break;

	case WM_KEYDOWN:
		if (wParam == VK_SPACE) {
			jump = 1;
		}

		if (wParam == 'M') {
			if (MapNum == 1)
				MapNum = 0;
			else MapNum = 1;
		}

		if (wParam == 'A' || wParam == 'a')
			aDown = true;

		if (wParam == 'D' || wParam == 'd')
			dDown = true;

		break;

	case WM_KEYUP:
		if (wParam == 'A' || wParam == 'a')
			aDown = false;
		if (wParam == 'D' || wParam == 'd')
			dDown = false;

		left = 0;
		right = 0;

		break;

	case WM_CHAR:
		if (wParam == 'W' || wParam == 'w') {
			if (x >= Portal_X - CharX && x <= Portal_X - CharX + w_Portal * 1 / 4) {
				if (Key_Image == 0) {
					Image_Number = 7;
				}

				else if (Image_Number == 8) {
					Image_Number = 6;
				}

				else Image_Number = 8;
			}
		}

		if (wParam == '1') {
			CharNum = 1;

			InvalidateRect(hWnd, NULL, FALSE);
		}

		if (wParam == '2') {
			CharNum = 2;

			InvalidateRect(hWnd, NULL, FALSE);
		}

		if (wParam == '3') {
			CharNum = 3;

			InvalidateRect(hWnd, NULL, FALSE);
		}

		break;

	case WM_DESTROY:
		KillTimer(hWnd, 0);

		PostQuitMessage(0);

		break;

	}
	
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
};