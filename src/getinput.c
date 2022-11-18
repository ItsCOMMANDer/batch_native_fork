/*
 * dll.c (getinput.dll)
 * By mousiedev & kenan238
 * ---------------------------
 * Original getinput.exe by kenan238
 * Big big rewrite by mousiedev
 *
 * Licensed under conditions stated in the
   "getinput.txt" document you recieved with this software.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellscalingapi.h>

// compiler, i know what im doing, now shut up
#pragma GCC diagnostic ignored "-Wimplicit-int"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

// i am too lazy lmfao
#define CreateThreadS(funptr) CreateThread(0,0,funptr,0,0,0)
#define ENV SetEnvironmentVariable
#define InRange(x, b, t) (b < x && x < t)

signed wheelDelta = 0;

BYTE b[21] = {0}, *c;
PCHAR itoa_(i,x) {
  c=b+20,x=abs(i);
  do *--c = 48 + x % 10; while(x && (x/=10)); if(i<0) *--c = 45;
  return c;
}

BYTE conversion_table[] = {
//       0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
/* 0 */  -1  , -1  , 0x02, 0x03, -1  , -1  , -1  , 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // exclude mouse buttons
/* 1 */  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
/* 2 */  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
/* 3 */  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
/* 4 */  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
/* 5 */  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
/* 6 */  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
/* 7 */  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
/* 8 */  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
/* 9 */  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
/* A */  -1  , -1  , -1  , -1  , -1  , -1  , 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // exclude right/left ctrl,shift,etc.
/* B */  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
/* C */  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
/* D */  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
/* E */  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
/* F */  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};

BYTE m[0x100] = {0};
char buffer[0x300] = {0};
/* TODO optimize */
VOID process_keys() {
  int isAnyKeyDown = 0, actionHappened = 0;
  BOOL state = 0;

  for(int i = 3; i < 0x100; i++) {
    state = GetAsyncKeyState(i);
    // todo optimize
    if (!m[i] && state & 0x8000) { m[i] = TRUE; actionHappened = TRUE; }
    if (m[i] && !state) { m[i] = FALSE; actionHappened = TRUE; }

    isAnyKeyDown |= m[i];
  }

  if(!actionHappened) return;
  if(!isAnyKeyDown) {SetEnvironmentVariable("keyspressed", NULL); return;} // this DOES work
  ZeroMemory(buffer, 0x300);

  for(int i = 0; i < 0x100; i++) {
    if(m[i]) {
      int num = conversion_table[i];
      if(num != (BYTE)(-1)) {
		if(buffer[0] == 0) {
			buffer[0] = '-';
#ifdef IS_YESHI_ANNOYING_AGAIN
			ENV("keypressed", itoa_(num));
#endif
		}
        	lstrcat(buffer, itoa_(num));
        	lstrcat(buffer, "-");
      }
    }
  }

  SetEnvironmentVariable("keyspressed",buffer);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  MSLLHOOKSTRUCT *info = (MSLLHOOKSTRUCT *)lParam;
  wheelDelta = GET_WHEEL_DELTA_WPARAM(info->mouseData);
  if (wheelDelta != 0) {
	wheelDelta /= WHEEL_DELTA;
	ENV("wheeldelta", itoa_(wheelDelta));
  }
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

#define PXTODIP(x,y) ((float)(x) / y)

DWORD CALLBACK Process(void *data) {
  // todo maybe close these
  HANDLE 
    hOut = GetStdHandle(STD_OUTPUT_HANDLE),
    hIn = GetStdHandle(STD_INPUT_HANDLE);
  HWND hCon = GetConsoleWindow();

  BYTE mouseclick;
  
  CONSOLE_FONT_INFO info;
  POINT pt[2], font;

  COORD * fontSz = &info.dwFontSize;

	CONSOLE_FONT_INFOEX infoex;

  HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);

  //float scale = GetDpiForWindow(hCon)/96.0f;

	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  MSG mouseMsg;

  while(TRUE) {
	SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS & ~ENABLE_QUICK_EDIT_MODE);
    GetCurrentConsoleFont(hOut, FALSE, &info);

    GetCursorPos(&pt);
    ScreenToClient(hCon,&pt);

    mouseclick = 
		(GetKeyState(VK_LBUTTON) & 0x80) >> 7 |
		(GetKeyState(VK_RBUTTON) & 0x80) >> 6 |
		(GetKeyState(VK_MBUTTON) & 0x80) >> 5;

	if(mouseclick && GetSystemMetrics(SM_SWAPBUTTON))
		mouseclick |= ~(mouseclick & 0b11); // todo when mouse buttons r inverted, the middle mouse button reports 5 instead of 4

	pt[1].x = fontSz->X; LPtoDP(GetDC(hCon), &pt[1], 1);
	pt[1].y = fontSz->Y; LPtoDP(GetDC(hCon), &pt[1], 1);
	float x = pt[0].x / pt[1].x + 0.5f;
	float y = pt[0].y / pt[1].y + 0.5f;

    ENV("mousexpos", itoa_((int)x));
    ENV("mouseypos", itoa_((int)y));

    if(hCon == GetForegroundWindow()) {
      ENV("click", itoa_(mouseclick));
      process_keys();
    }

    PeekMessage(&mouseMsg, hCon, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE);
    Sleep(1000 / 45);
  }
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH) {
	  DisableThreadLibraryCalls(hInst);
    CloseHandle(CreateThreadS(Process));
  }
  return TRUE;
}
