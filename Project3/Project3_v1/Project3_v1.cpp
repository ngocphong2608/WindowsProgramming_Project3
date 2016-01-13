// DemoDialog2.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Drawing.h"
#include "Thread.h"
#include "DebugConsole.h"
#include "Project3_v1.h"
#include <time.h>
#include <Windowsx.h>

#define TOTAL 10
#define LINE 3
#define SLEEP 1000

// Forward declarations of functions included in this code module:
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);		// dialog proc
void drawBitMap(HBITMAP &hBitmap, HWND hWnd);			// draw a bitmap on full windows
void OnInitDialog(HWND hWnd);
void PaintThread(HWND hWnd);			// thread for drawing purpose
void LineThread(LINETHREADINFO* info);	// thread control each line

// Global variable
HINSTANCE hInst;						// instance
HBITMAP hGlobalBitmap;					// bitmap background
int ListBin[TOTAL + 10];				// list bin on processing
int Bin[3];								// 3 bins for drawing purpose
int Remaining;							// number of bin not process
int Index;								// index of list bin
HANDLE ghMutex, ghSemaphore;			// global sync variables
int LineCount[LINE];					// store cout value of each line
int LineTurn;							// value = {1, 2, 3}, indcate which line on processing

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	hInst = hInstance;

	if (-1 == DialogBox(hInstance, MAKEINTRESOURCE(IDD_MYDLG), NULL, DlgProc))
	{
		MessageBox(NULL, TEXT("Cannot create dialog!"),
			L"", MB_ICONERROR);
	}

	return 0;
}

BOOL CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// local variable 

	switch (message)
	{
	case WM_INITDIALOG:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnInitDialog, hWnd, 0, NULL);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		}
		return FALSE;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hWnd, IDC_STATIC_COUNT1) || (HWND)lParam == GetDlgItem(hWnd, IDC_STATIC_LINE1))
		{
			// we're about to draw the static
			// set the text colour in (HDC)lParam
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			// NOTE: per documentation as pointed out by selbie, GetSolidBrush would leak a GDI handle.
			return (BOOL)GetSysColorBrush(COLOR_MENU);
		}
		else if ((HWND)lParam == GetDlgItem(hWnd, IDC_STATIC_COUNT2) || (HWND)lParam == GetDlgItem(hWnd, IDC_STATIC_LINE2))
		{
			// we're about to draw the static
			// set the text colour in (HDC)lParam
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetTextColor((HDC)wParam, RGB(0, 0, 255));
			// NOTE: per documentation as pointed out by selbie, GetSolidBrush would leak a GDI handle.
			return (BOOL)GetSysColorBrush(COLOR_MENU);
		}
		else if ((HWND)lParam == GetDlgItem(hWnd, IDC_STATIC_COUNT3) || (HWND)lParam == GetDlgItem(hWnd, IDC_STATIC_LINE3))
		{
			// we're about to draw the static
			// set the text colour in (HDC)lParam
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetTextColor((HDC)wParam, (255, 174, 201));
			// NOTE: per documentation as pointed out by selbie, GetSolidBrush would leak a GDI handle.
			return (BOOL)GetSysColorBrush(COLOR_MENU);
		}
		break;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_CLOSE:
			/*DeleteObject(hGlobalBitmap);
			CloseHandle(ghSemaphore);
			CloseHandle(ghMutex);*/
			EndDialog(hWnd, 0);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

void drawBitMap(HBITMAP &hBitmap, HWND hWnd)
{
	if (hBitmap == NULL)
	{
		MessageBox(hWnd, L"Load bitmap fail", L"Error", MB_OK);
		return;
	}

	HDC hdc = GetDC(hWnd);

	HDC hCopy = CreateCompatibleDC(hdc);

	SelectObject(hCopy, hBitmap);

	RECT r;
	GetClientRect(hWnd, &r);

	BitBlt(hdc, r.left, r.top, r.right, r.bottom, hCopy, 0, 0, MERGECOPY);

	DeleteDC(hCopy);
	ReleaseDC(hWnd, hdc);
}

void PaintThread(HWND hWnd)
{
	HWND hST[5];
	hST[0] = GetDlgItem(hWnd, IDC_STATIC0);
	hST[1] = GetDlgItem(hWnd, IDC_STATIC1);
	hST[2] = GetDlgItem(hWnd, IDC_STATIC2);
	hST[3] = GetDlgItem(hWnd, IDC_STATIC3);
	hST[4] = GetDlgItem(hWnd, IDC_STATIC4);

	HWND hL1, hL2, hL3, hC1, hC2, hC3, hBin1, hBin2, hBin3;
	hL1 = GetDlgItem(hWnd, IDC_STATIC_LINE1);
	hL2 = GetDlgItem(hWnd, IDC_STATIC_LINE2);
	hL3 = GetDlgItem(hWnd, IDC_STATIC_LINE3);
	hC1 = GetDlgItem(hWnd, IDC_STATIC_COUNT1);
	hC2 = GetDlgItem(hWnd, IDC_STATIC_COUNT2);
	hC3 = GetDlgItem(hWnd, IDC_STATIC_COUNT3);
	hBin1 = GetDlgItem(hWnd, IDC_STATIC_BIN1);
	hBin2 = GetDlgItem(hWnd, IDC_STATIC_BIN2);
	hBin3 = GetDlgItem(hWnd, IDC_STATIC_BIN3);

	HWND hTotal, hRemaining;
	hTotal = GetDlgItem(hWnd, IDC_STATIC_TOTAL);
	hRemaining = GetDlgItem(hWnd, IDC_STATIC_REMAINING);

	// red RGB(237, 28, 36)
	// blue RGB(63, 72, 204)
	// pink = RGB(255, 174, 201)
	COLORREF red = RGB(237, 28, 36), blue = RGB(63, 72, 204), pink = RGB(255, 174, 201), black = RGB(0, 0, 0);

	while (TRUE)
	{
		TCHAR temp[100];

		// update total, remaining
		_stprintf_s(temp, L"Total = %d", TOTAL);
		Static_SetText(hTotal, temp);
		_stprintf_s(temp, L"Remaining = %d", Remaining);
		Static_SetText(hRemaining, temp);

		// update line count 1, 2, 3
		_stprintf_s(temp, L"Count = %d", LineCount[0]);
		Static_SetText(hC1, temp);
		_stprintf_s(temp, L"Count = %d", LineCount[1]);
		Static_SetText(hC2, temp);
		_stprintf_s(temp, L"Count = %d", LineCount[2]);
		Static_SetText(hC3, temp);

		drawBitMap(hGlobalBitmap, hWnd);
		InvalidateRect(hL1, NULL, TRUE);
		InvalidateRect(hL2, NULL, TRUE);
		InvalidateRect(hL3, NULL, TRUE);
		InvalidateRect(hC1, NULL, TRUE);
		InvalidateRect(hC2, NULL, TRUE);
		InvalidateRect(hC3, NULL, TRUE);
		InvalidateRect(hBin1, NULL, TRUE);
		InvalidateRect(hBin2, NULL, TRUE);
		InvalidateRect(hBin3, NULL, TRUE);
		InvalidateRect(hTotal, NULL, TRUE);
		InvalidateRect(hRemaining, NULL, TRUE);

		// update hST0, hST1, hST2, hST3, hST4
		// hST0 = ListBin[Index], ... , hST4 = ListBin[Index + 4]

		COLORREF Bin4Color = red;
		if (LineTurn == 3)
			Bin4Color = pink;
		if (LineTurn == 2)
			Bin4Color = blue;

		COLORREF color = black;
		for (int i = 0; i < 5; i++)
		{
			InvalidateRect(hST[i], NULL, TRUE);

			if (i == 4)
				color = Bin4Color;

			DrawBin(hST[i], color, ListBin[Index + i]);
		}

		DrawBin(hBin1, red, Bin[0]);
		DrawBin(hBin2, blue, Bin[1]);
		DrawBin(hBin3, pink, Bin[2]);

		Sleep(200);
	}
}

void OnInitDialog(HWND hWnd)
{
	// init value
	Remaining = TOTAL;
	Index = TOTAL + 5;

	// Bin = 0
	memset(Bin, 0, sizeof(int)* LINE);

	// LineCount = 0
	memset(LineCount, 0, sizeof(int)* LINE);

	// random bin values
	srand(time(NULL));
	memset(ListBin, 0, (TOTAL + 10) * sizeof(int));
	for (int i = 5; i < TOTAL + 5; i++)
	{
		ListBin[i] = rand() % 3 + 1;
	}

	// sync variables
	ghMutex = CreateMutex(NULL, FALSE, NULL);
	ghSemaphore = CreateSemaphore(NULL, 3, 3, NULL);

	// background
	hGlobalBitmap = (HBITMAP)LoadImage(hInst, L"demo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	// update paiting thread
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PaintThread, hWnd, NULL, NULL);

	// create 3 thread associate with 3 line
	LINETHREADINFO ltInfo[3];
	HANDLE hThread[3];
	DWORD ThreadID[3];
	for (int i = 0; i < 3; i++)
	{
		ltInfo[i].LineNumber = i + 1;
		hThread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LineThread, &ltInfo[i], NULL, &ThreadID[0]);
	}

	WaitForMultipleObjects(3, hThread, TRUE, INFINITE);

	// 3 thread done, show result
	TCHAR temp[100];
	_stprintf_s(temp, L"Line 1, Count = %d\nLine 2, Count = %d\nLine 3, Count = %d", LineCount[0], LineCount[1], LineCount[2]);
	MessageBox(hWnd, temp, L"Result", MB_OK);

	// exit
	EndDialog(hWnd, 0);
}

void LineThread(LINETHREADINFO* info)
{
	WaitForSingleObject(ghSemaphore, 0L);
	while (TRUE)
	{
		WaitForSingleObject(ghMutex, INFINITE);

		if (Remaining == 0)
		{
			ReleaseMutex(ghMutex);
			break;
		}

		LineTurn = info->LineNumber;

		Sleep(SLEEP / 2);

		if (ListBin[Index + 4] == info->LineNumber)
		{
			Remaining--;
			//Index--;
			LineCount[info->LineNumber - 1]++;
			ListBin[Index + 4] = 0;
			Bin[info->LineNumber - 1] = info->LineNumber;
			Sleep(SLEEP / 2);
			Bin[info->LineNumber - 1] = 0;
		}
		else if (ListBin[Index + 4] == 0)
		{
			Index--;
			Sleep(SLEEP / 2);
		}
		else
			Sleep(SLEEP / 2);

		ReleaseMutex(ghMutex);
	}
	ReleaseSemaphore(ghSemaphore, 1, NULL);
}
