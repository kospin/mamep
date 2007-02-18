/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  PaletteEdit.c

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "MAME32.h"
#include "driver.h"
#include "bitmask.h"
#include "options.h"
#include "resource.h"
#include "translate.h"

/***************************************************************
 * Imported function prototypes
 ***************************************************************/

/**************************************************************
 * Local function prototypes
 **************************************************************/

static void InitializePaletteUI(HWND hwnd);
static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
static void PaletteSave(void);
static void PaletteView(HWND hwnd);
static void PaletteChange(HWND hwnd);
static void PaletteSet(HWND hwnd);

/**************************************************************
 * Local private variables
 **************************************************************/

static unsigned char palette_tmp[MAX_COLORTABLE][3];
static unsigned char palette_rgb[3];
static int   palette_num = 0;

/***************************************************************
 * Public functions
 ***************************************************************/

INT_PTR CALLBACK PaletteDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG :
		TranslateDialog(hDlg, lParam, TRUE);

		InitializePaletteUI(hDlg);
		return TRUE;

	case WM_HSCROLL :
		/* slider changed */
		HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OptOnHScroll);
		break;

	case WM_COMMAND :
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDC_PALETTE_COMBO :
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
				PaletteChange(hDlg);
			break;
		case IDOK :
			PaletteSave();
		case IDCANCEL :
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}

	PaletteView(hDlg);

	return 0;
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/* Initialize the palette options */
static void InitializePaletteUI(HWND hwnd)
{
	static const char *palette_names[MAX_COLORTABLE] =
	{
		"Font (blank)",
		"Font (normal)",
		"Font (special)",
		"Window background",
		"Window frame",
		"Window frame (light)",
		"Window frame (dark)",
		"OSD bar",
		"OSD bar (light)",
		"OSD bar (dark)",
		"OSD bar (default)",
		"Button (A or 1)",
		"Button (B or 2)",
		"Button (C or 3)",
		"Button (D or 4)",
		"Button (K or 5)",
		"Button (P or 6)",
		"Button (S or 7)",
		"Button (8)",
		"Button (9)",
		"Button (10)",
		"Cursor",
	};

	int i;
	HWND hCtrl = GetDlgItem(hwnd, IDC_PALETTE_COMBO);

	if (hCtrl)
	{
		for (i = 0; i < MAX_COLORTABLE; i++)
		{
			const char *p;
			unsigned a, b, c;

			p = GetUIPaletteString(i);
			sscanf(p, "%u,%u,%u", &a, &b, &c);
			palette_tmp[i][0] = (unsigned char)a;
			palette_tmp[i][1] = (unsigned char)b;
			palette_tmp[i][2] = (unsigned char)c;

			ComboBox_AddString(hCtrl, _Unicode(_UI(palette_names[i])));
		}
	}

	for (i = 0; i < 3; i++)
	{
		SendMessage(GetDlgItem(hwnd, IDC_PALETTE_R + i), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 255));
	}

	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_PALETTE_COMBO), 0);

	PaletteSet(hwnd);
}

static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	char          buf[100];
	unsigned char nValue;
	int           i;

	for (i = 0; i < 3; i++)
		if (hwndCtl == GetDlgItem(hwnd, IDC_PALETTE_R + i))
		{
			/* Get the current value of the control */
			nValue = (unsigned char)SendMessage(GetDlgItem(hwnd, IDC_PALETTE_R + i), TBM_GETPOS, 0, 0);
			palette_rgb[i] = nValue;

			/* Set the static display to the new value */
			sprintf(buf, "%u", nValue);
			Static_SetTextA(GetDlgItem(hwnd, IDC_PALETTE_TEXTR + i), buf);

			return;
		}
}

static void PaletteSave(void)
{
	char buf[16];
	int i;

	for (i = 0; i < 3; i++)
		palette_tmp[palette_num][i] = palette_rgb[i];

	for (i = 0; i < MAX_COLORTABLE; i++)
	{
		sprintf(buf, "%u,%u,%u", palette_tmp[i][0], palette_tmp[i][1], palette_tmp[i][2]);
		SetUIPaletteString(i, buf);
	}
}

static void PaletteView(HWND hwnd)
{
	HWND hWnd = GetDlgItem(hwnd, IDC_PALETTE_VIEW);
	HDC  hDC  = GetDC(hWnd);
	HPEN hPen, hOldPen;
	RECT rt;

	if (hWnd == NULL)
		return;

	GetClientRect(hWnd, &rt);
	hPen = CreatePen(PS_INSIDEFRAME, 100, RGB(palette_rgb[0], palette_rgb[1], palette_rgb[2]));
	hOldPen = SelectObject(hDC, hPen);
	SelectObject(hDC, hPen);
	Rectangle(hDC, rt.left, rt.top, rt.right, rt.bottom);
	SelectObject(hDC, hOldPen);
	DeleteObject(hPen);
	ReleaseDC(hWnd, hDC);
}

static void PaletteChange(HWND hwnd)
{
	int i;

	for (i = 0; i < 3; i++)
		palette_tmp[palette_num][i] = palette_rgb[i];

	PaletteSet(hwnd);
}

static void PaletteSet(HWND hwnd)
{
	char buf[100];
	int i;

	palette_num = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PALETTE_COMBO));

	for (i = 0; i < 3; i++)
	{
		palette_rgb[i] = palette_tmp[palette_num][i];

		SendMessage(GetDlgItem(hwnd, IDC_PALETTE_R + i), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)palette_rgb[i]);

		sprintf(buf, "%u", palette_rgb[i]);
		Static_SetTextA(GetDlgItem(hwnd, IDC_PALETTE_TEXTR + i), buf);
	}
}

/* End of source file */