#include "stdafx.h"
#include <windowsx.h>

void DrawBin(HWND hWnd, COLORREF color, int number)
{
	if (number == 0)
	{
		Static_SetText(hWnd, L"");
		return;
	}

	HDC hdc = GetDC(hWnd);

	HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
	SelectObject(hdc, hBrush);

	HPEN hPen = CreatePen(0, 10, color);
	SelectObject(hdc, hPen);

	TCHAR temp[100];
	_stprintf_s(temp, L"%d", number);
	Static_SetText(hWnd, temp);

	RECT r;
	GetClientRect(hWnd, &r);

	Rectangle(hdc, 0, 0, r.right + 1, r.bottom + 1);

	DeleteObject(hPen);
	ReleaseDC(hWnd, hdc);
}