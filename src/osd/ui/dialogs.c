/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  dialogs.c

  Dialog box procedures go here

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string.h>

#include "mame32.h"
#include "bitmask.h"
#include "TreeView.h"
#include "M32Util.h"
#include "resource.h"
#include "directories.h"
#include "options.h"
#include "splitters.h"
#include "help.h"
#include "audit32.h"
#include "screenshot.h"
#include "mame32.h"
#include "properties.h"
#include "dialogs.h"
#ifdef USE_VIEW_PCBINFO
#include "file.h"
#endif /* USE_VIEW_PCBINFO */
#include "translate.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define FILTERTEXT_LEN 256

static WCHAR g_FilterText[FILTERTEXT_LEN];

#define NUM_EXCLUSIONS  9

/* Pairs of filters that exclude each other */
DWORD filterExclusion[NUM_EXCLUSIONS] =
{
	IDC_FILTER_CLONES,      IDC_FILTER_ORIGINALS,
	IDC_FILTER_NONWORKING,  IDC_FILTER_WORKING,
	IDC_FILTER_UNAVAILABLE, IDC_FILTER_AVAILABLE,
	IDC_FILTER_RASTER,      IDC_FILTER_VECTOR
};

static void DisableFilterControls(HWND hWnd, LPFOLDERDATA lpFilterRecord,
								  LPFILTER_ITEM lpFilterItem, DWORD dwFlags);
static void EnableFilterExclusions(HWND hWnd, DWORD dwCtrlID);
static DWORD ValidateFilters(LPFOLDERDATA lpFilterRecord, DWORD dwFlags);
static void OnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);

#ifdef USE_VIEW_PCBINFO
static HWND hPcbInfo = NULL;
static WNDPROC g_lpPcbInfoWndProc = NULL;
static LRESULT CALLBACK PcbInfoWndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif /* USE_VIEW_PCBINFO */

/***************************************************************************/

LPCWSTR GetFilterText(void)
{
	return g_FilterText;
}

INT_PTR CALLBACK ResetDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL resetFilters  = FALSE;
	BOOL resetGames    = FALSE;
	BOOL resetUI	   = FALSE;
	BOOL resetDefaults = FALSE;

	switch (Msg)
	{
	case WM_INITDIALOG:
		TranslateDialog(hDlg, lParam, FALSE);

		return TRUE;

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());

		break;

	case WM_COMMAND :
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK :
			resetFilters  = Button_GetCheck(GetDlgItem(hDlg, IDC_RESET_FILTERS));
			resetGames	  = Button_GetCheck(GetDlgItem(hDlg, IDC_RESET_GAMES));
			resetDefaults = Button_GetCheck(GetDlgItem(hDlg, IDC_RESET_DEFAULT));
			resetUI 	  = Button_GetCheck(GetDlgItem(hDlg, IDC_RESET_UI));
			if (resetFilters || resetGames || resetUI || resetDefaults)
			{
				WCHAR temp[1024];
				lstrcpy(temp, _UIW(TEXT(MAME32NAME " will now reset the selected\n")));
				lstrcat(temp, _UIW(TEXT("items to the original, installation\n")));
				lstrcat(temp, _UIW(TEXT("settings then exit.\n\n")));
				lstrcat(temp, _UIW(TEXT("The new settings will take effect\n")));
				lstrcat(temp, _UIW(TEXT("the next time "MAME32NAME " is run.\n\n")));
				lstrcat(temp, _UIW(TEXT("Do you wish to continue?")));

				if (MessageBox(hDlg, temp, _UIW(TEXT("Restore Settings")), IDOK) == IDOK)
				{
					if (resetFilters)
						ResetFilters();

					if (resetUI)
						ResetGUI();

					if (resetDefaults)
						ResetGameDefaults();

					if (resetGames)
						ResetAllGameOptions();

					EndDialog(hDlg, 1);
					return TRUE;
				}
			}
			/* Fall through if no options were selected
			 * or the user hit cancel in the popup dialog.
			 */
		case IDCANCEL :
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	return 0;
}

INT_PTR CALLBACK InterfaceDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	char tmp[4];
	int i;
	BOOL bRedrawList = FALSE;
	int nCurSelection = 0;
	int nHistoryTab = 0;
	int nCount = 0;
	int value;

	switch (Msg)
	{
	case WM_INITDIALOG:
		TranslateDialog(hDlg, lParam, FALSE);

		Button_SetCheck(GetDlgItem(hDlg,IDC_START_GAME_CHECK),GetGameCheck());
		Button_SetCheck(GetDlgItem(hDlg,IDC_JOY_GUI),GetJoyGUI());
		Button_SetCheck(GetDlgItem(hDlg,IDC_KEY_GUI),GetKeyGUI());
		Button_SetCheck(GetDlgItem(hDlg,IDC_BROADCAST),GetBroadcast());
		Button_SetCheck(GetDlgItem(hDlg,IDC_RANDOM_BG),GetRandomBackground());
		Button_SetCheck(GetDlgItem(hDlg,IDC_GAME_CAPTION),GetGameCaption());
#ifdef USE_SHOW_SPLASH_SCREEN
		Button_SetCheck(GetDlgItem(hDlg,IDC_DISPLAY_SPLASH_SCREEN),GetDisplaySplashScreen());
#endif /* USE_SHOW_SPLASH_SCREEN */
#ifdef TREE_SHEET
		Button_SetCheck(GetDlgItem(hDlg,IDC_SHOW_TREE_SHEET),GetShowTreeSheet());
#endif /* TREE_SHEET */

		Button_SetCheck(GetDlgItem(hDlg,IDC_HIDE_MOUSE),GetHideMouseOnStartup());

		// Get the current value of the control
		SendDlgItemMessageA(hDlg, IDC_CYCLETIMESEC, TBM_SETRANGE,
					(WPARAM)FALSE,
					(LPARAM)MAKELONG(0, 60)); /* [0, 60] */
		value = GetCycleScreenshot();
		SendDlgItemMessageA(hDlg,IDC_CYCLETIMESEC, TBM_SETPOS, TRUE, value);
		itoa(value,tmp,10);
		SendDlgItemMessageA(hDlg,IDC_CYCLETIMESECTXT,WM_SETTEXT,0, (WPARAM)tmp);

		Button_SetCheck(GetDlgItem(hDlg,IDC_STRETCH_SCREENSHOT_LARGER),
						GetStretchScreenShotLarger());
		Button_SetCheck(GetDlgItem(hDlg,IDC_FILTER_INHERIT),
						GetFilterInherit());
		Button_SetCheck(GetDlgItem(hDlg,IDC_NOOFFSET_CLONES),
						GetOffsetClones());
		Button_SetCheck(GetDlgItem(hDlg,IDC_BROKEN_ICON),
						!GetUseBrokenIcon());
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("Snapshot")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_SCREENSHOT);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("Flyer")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_FLYER);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("Cabinet")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_CABINET);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("Marquee")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_MARQUEE);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("Title")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_TITLE);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("Control Panel")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_CONTROL_PANEL);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("All")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_ALL);
		ComboBox_AddString(GetDlgItem(hDlg, IDC_HISTORY_TAB), _UIW(TEXT("None")));
		ComboBox_SetItemData(GetDlgItem(hDlg, IDC_HISTORY_TAB), nCount++, TAB_NONE);
		if( GetHistoryTab() < MAX_TAB_TYPES )
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_HISTORY_TAB), GetHistoryTab());
		else
			ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_HISTORY_TAB), GetHistoryTab()-TAB_SUBTRACT-(MAX_TAB_TYPES-TAB_HISTORY-1));

		SendDlgItemMessage(hDlg, IDC_SCREENSHOT_BORDERSIZE, TBM_SETRANGE,
					(WPARAM)FALSE,
					(LPARAM)MAKELONG(0, 100)); /* [0, 100] */
		value = GetScreenshotBorderSize();
		SendDlgItemMessage(hDlg,IDC_SCREENSHOT_BORDERSIZE, TBM_SETPOS, TRUE, value);
		itoa(value,tmp,10);
		SendDlgItemMessageA(hDlg,IDC_SCREENSHOT_BORDERSIZETXT,WM_SETTEXT,0, (WPARAM)tmp);
		//return TRUE;
		break;

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;
	case WM_HSCROLL:
		HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OnHScroll);
		break;
	case WM_COMMAND :
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDC_SCREENSHOT_BORDERCOLOR:
		{
			CHOOSECOLOR cc;
			COLORREF choice_colors[16];

			for (i=0;i<16;i++)
				choice_colors[i] = GetCustomColor(i);

			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.hwndOwner   = hDlg;
			cc.rgbResult   = GetScreenshotBorderColor();
			cc.lpCustColors = choice_colors;
			cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
			if (!ChooseColor(&cc))
				return TRUE;
 			for (i=0;i<16;i++)
 				SetCustomColor(i,choice_colors[i]);
			SetScreenshotBorderColor(cc.rgbResult);
			UpdateScreenShot();
			return TRUE;
		}

		case IDC_EDIT_BROKEN_COLOR:
		{
			CHOOSECOLOR cc;
			COLORREF choice_colors[16];

			for (i=0;i<16;i++)
				choice_colors[i] = GetCustomColor(i);

			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.hwndOwner   = hDlg;
			cc.rgbResult   = GetListBrokenColor();
			cc.lpCustColors = choice_colors;
			cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
			if (!ChooseColor(&cc))
				return TRUE;
 			for (i=0;i<16;i++)
 				SetCustomColor(i,choice_colors[i]);
			SetListBrokenColor(cc.rgbResult);
			// LineUpIcons does just a ResetListView(), which is what we want here
			PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, FALSE),(LPARAM)NULL);
			return TRUE;
		}

		case IDOK :
		{
			BOOL checked = FALSE;

			SetGameCheck(Button_GetCheck(GetDlgItem(hDlg, IDC_START_GAME_CHECK)));
			SetJoyGUI(Button_GetCheck(GetDlgItem(hDlg, IDC_JOY_GUI)));
			SetKeyGUI(Button_GetCheck(GetDlgItem(hDlg, IDC_KEY_GUI)));
			SetBroadcast(Button_GetCheck(GetDlgItem(hDlg, IDC_BROADCAST)));
			SetRandomBackground(Button_GetCheck(GetDlgItem(hDlg, IDC_RANDOM_BG)));
			SetGameCaption(Button_GetCheck(GetDlgItem(hDlg, IDC_GAME_CAPTION)));
#ifdef USE_SHOW_SPLASH_SCREEN
			SetDisplaySplashScreen(Button_GetCheck(GetDlgItem(hDlg, IDC_DISPLAY_SPLASH_SCREEN)));
#endif /* USE_SHOW_SPLASH_SCREEN */
#ifdef TREE_SHEET
			SetShowTreeSheet(Button_GetCheck(GetDlgItem(hDlg, IDC_SHOW_TREE_SHEET)));
#endif /* TREE_SHEET */

			SetHideMouseOnStartup(Button_GetCheck(GetDlgItem(hDlg,IDC_HIDE_MOUSE)));

			if( Button_GetCheck(GetDlgItem(hDlg,IDC_RESET_PLAYCOUNT ) ) )
			{
				ResetPlayCount( -1 );
				bRedrawList = TRUE;
			}
			if( Button_GetCheck(GetDlgItem(hDlg,IDC_RESET_PLAYTIME ) ) )
			{
				ResetPlayTime( -1 );
				bRedrawList = TRUE;
			}
			value = SendDlgItemMessageA(hDlg,IDC_CYCLETIMESEC, TBM_GETPOS, 0, 0);
			if( GetCycleScreenshot() != value )
			{
				SetCycleScreenshot(value);
			}
			value = SendDlgItemMessageA(hDlg,IDC_SCREENSHOT_BORDERSIZE, TBM_GETPOS, 0, 0);
			if( GetScreenshotBorderSize() != value )
			{
				SetScreenshotBorderSize(value);
				UpdateScreenShot();
			}
			value = SendDlgItemMessageA(hDlg,IDC_HIGH_PRIORITY, TBM_GETPOS, 0, 0);
			checked = Button_GetCheck(GetDlgItem(hDlg,IDC_STRETCH_SCREENSHOT_LARGER));
			if (checked != GetStretchScreenShotLarger())
			{
				SetStretchScreenShotLarger(checked);
				UpdateScreenShot();
			}
			checked = Button_GetCheck(GetDlgItem(hDlg,IDC_FILTER_INHERIT));
			if (checked != GetFilterInherit())
			{
				SetFilterInherit(checked);
				// LineUpIcons does just a ResetListView(), which is what we want here
				PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, FALSE),(LPARAM)NULL);
 			}
			checked = Button_GetCheck(GetDlgItem(hDlg,IDC_NOOFFSET_CLONES));
			if (checked != GetOffsetClones())
			{
				SetOffsetClones(checked);
				// LineUpIcons does just a ResetListView(), which is what we want here
				PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, FALSE),(LPARAM)NULL);
 			}
			checked = !Button_GetCheck(GetDlgItem(hDlg,IDC_BROKEN_ICON));
			if (checked != GetUseBrokenIcon())
			{
				SetUseBrokenIcon(checked);
				// LineUpIcons does just a ResetListView(), which is what we want here
				PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, FALSE),(LPARAM)NULL);
 			}
			nCurSelection = ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_HISTORY_TAB));
			if (nCurSelection != CB_ERR)
				nHistoryTab = ComboBox_GetItemData(GetDlgItem(hDlg,IDC_HISTORY_TAB), nCurSelection);
			EndDialog(hDlg, 0);
			if( GetHistoryTab() != nHistoryTab )
			{
				SetHistoryTab(nHistoryTab, TRUE);
				ResizePickerControls(GetMainWindow());
				UpdateScreenShot();
			}
			if( bRedrawList )
			{
				UpdateListView();
			}
			return TRUE;
		}
		case IDCANCEL :
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	return 0;
}

INT_PTR CALLBACK FilterDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static DWORD			dwFilters;
	static DWORD			dwpFilters;
	static LPFOLDERDATA		lpFilterRecord;
	WCHAR strText[250];
	int 					i;

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		LPTREEFOLDER folder = GetCurrentFolder();
		LPTREEFOLDER lpParent = NULL;
		LPFILTER_ITEM g_lpFilterList = GetFilterList();

		TranslateDialog(hDlg, lParam, FALSE);
		dwFilters = 0;

		if (folder != NULL)
		{
			WCHAR tmp[80];
			
			Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_EDIT), g_FilterText);
			Edit_SetSel(GetDlgItem(hDlg, IDC_FILTER_EDIT), 0, -1);
			// Mask out non filter flags
			dwFilters = folder->m_dwFlags & F_MASK;
			// Display current folder name in dialog titlebar
			snwprintf(tmp,sizeof(tmp), _UIW(TEXT("Filters for %s Folder")),folder->m_lpTitle);
			SetWindowText(hDlg, tmp);
			if ( GetFilterInherit() )
			{
				if( folder )
				{
					BOOL bShowExplanation = FALSE;
					lpParent = GetFolder( folder->m_nParent );
					if( lpParent )
					{
						/* Check the Parent Filters and inherit them on child,
						 * No need to promote all games to parent folder, works as is */
						dwpFilters = lpParent->m_dwFlags & F_MASK;
						/*Check all possible Filters if inherited solely from parent, e.g. not being set explicitly on our folder*/
						if( (dwpFilters & F_CLONES) && !(dwFilters & F_CLONES) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_CLONES), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_CLONES), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_NONWORKING) && !(dwFilters & F_NONWORKING) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_NONWORKING), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_NONWORKING), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_UNAVAILABLE) && !(dwFilters & F_UNAVAILABLE) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_UNAVAILABLE), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_UNAVAILABLE), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_VECTOR) && !(dwFilters & F_VECTOR) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_VECTOR), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_VECTOR), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_RASTER) && !(dwFilters & F_RASTER) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_RASTER), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_RASTER), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_ORIGINALS) && !(dwFilters & F_ORIGINALS) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_ORIGINALS), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_ORIGINALS), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_WORKING) && !(dwFilters & F_WORKING) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_WORKING), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_WORKING), strText);
							bShowExplanation = TRUE;
						}
						if( (dwpFilters & F_AVAILABLE) && !(dwFilters & F_AVAILABLE) )
						{
							/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
							Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_AVAILABLE), strText, 250);
							lstrcat(strText, TEXT(" (*)"));
							Edit_SetText(GetDlgItem(hDlg, IDC_FILTER_AVAILABLE), strText);
							bShowExplanation = TRUE;
						}
						/*Do not or in the Values of the parent, so that the values of the folder still can be set*/
						//dwFilters |= dwpFilters;
					}
					if( ! bShowExplanation )
					{
						ShowWindow(GetDlgItem(hDlg, IDC_INHERITED), FALSE );
					}
				}
			}
			else
			{
				ShowWindow(GetDlgItem(hDlg, IDC_INHERITED), FALSE );
			}
			// Find the matching filter record if it exists
			lpFilterRecord = FindFilter(folder->m_nFolderId);

			// initialize and disable appropriate controls
			for (i = 0; g_lpFilterList[i].m_dwFilterType; i++)
			{
				DisableFilterControls(hDlg, lpFilterRecord, &g_lpFilterList[i], dwFilters);
			}
		}
		SetFocus(GetDlgItem(hDlg, IDC_FILTER_EDIT));
		return FALSE;
	}
	case WM_HELP:
		// User clicked the ? from the upper right on a control
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP,
					 HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;

	case WM_COMMAND:
	{
		WORD wID		 = GET_WM_COMMAND_ID(wParam, lParam);
		WORD wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam);
		LPTREEFOLDER folder = GetCurrentFolder();
		LPFILTER_ITEM g_lpFilterList = GetFilterList();

		switch (wID)
		{
		case IDOK:
			dwFilters = 0;
			
			{
				WCHAR buf[FILTERTEXT_LEN];
				Edit_GetText(GetDlgItem(hDlg, IDC_FILTER_EDIT), buf, FILTERTEXT_LEN);
				StrTrim(buf, TEXT(" "));
				wcsncpy(g_FilterText, buf, FILTERTEXT_LEN);
			}
			
			// see which buttons are checked
			for (i = 0; g_lpFilterList[i].m_dwFilterType; i++)
			{
				if (Button_GetCheck(GetDlgItem(hDlg, g_lpFilterList[i].m_dwCtrlID)))
					dwFilters |= g_lpFilterList[i].m_dwFilterType;
			}

			// Mask out invalid filters
			dwFilters = ValidateFilters(lpFilterRecord, dwFilters);

			// Keep non filter flags
			folder->m_dwFlags &= ~F_MASK;

			// put in the set filters
			folder->m_dwFlags |= dwFilters;

			// Save the filters to the ini file
			SetFolderFlags(folder->m_lpOriginalTitle?
					folder->m_lpOriginalTitle:
					folder->m_lpTitle, dwFilters);

			EndDialog(hDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
			
		default:
			// Handle unchecking mutually exclusive filters
			if (wNotifyCode == BN_CLICKED)
				EnableFilterExclusions(hDlg, wID);
		}
	}
	break;
	}
	return 0;
}

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		TranslateDialog(hDlg, lParam, FALSE);

		{
			HBITMAP hBmp;
			hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL),
									  MAKEINTRESOURCE(IDB_ABOUT),
									  IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendDlgItemMessageA(hDlg, IDC_ABOUT, STM_SETIMAGE,
						(WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			Static_SetText(GetDlgItem(hDlg, IDC_VERSION), _Unicode(GetVersionString()));
		}
		return 1;

	case WM_COMMAND:
		EndDialog(hDlg, 0);
		return 1;
	}
	return 0;
}

INT_PTR CALLBACK AddCustomFileDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static LPTREEFOLDER default_selection = NULL;
	static int driver_index;

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		TREEFOLDER **folders;
		int num_folders;
		int i;
		TVINSERTSTRUCT tvis;
		TVITEM tvi;
		BOOL first_entry = TRUE;
		HIMAGELIST treeview_icons = GetTreeViewIconList();
		static HFONT hFont;
		LOGFONTW logfontW;

		TranslateDialog(hDlg, lParam, TRUE);

		if (hFont != NULL)
			DeleteObject(hFont);

		GetTranslatedFont(&logfontW);
		hFont = TranslateCreateFont(&logfontW);

		SetWindowFont(GetDlgItem(hDlg,IDC_CUSTOM_TREE), hFont, FALSE);

		// current game passed in using DialogBoxParam()
		driver_index = lParam;

		TreeView_SetImageList(GetDlgItem(hDlg,IDC_CUSTOM_TREE), treeview_icons, LVSIL_NORMAL);

		GetFolders(&folders,&num_folders);

		// should add "New..."

		// insert custom folders into our tree view
		for (i=0;i<num_folders;i++)
		{
			if (folders[i]->m_dwFlags & F_CUSTOM)
			{
				HTREEITEM hti;
				int jj;

				if (folders[i]->m_nParent == -1)
				{
				    tvis.hParent = TVI_ROOT;
					tvis.hInsertAfter = TVI_SORT;
					tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tvi.pszText = folders[i]->m_lpTitle;
					tvi.lParam = (LPARAM)folders[i];
					tvi.iImage = GetTreeViewIconIndex(folders[i]->m_nIconId);
					tvi.iSelectedImage = 0;
#if !defined(NONAMELESSUNION)
					tvis.item = tvi;
#else
					tvis.DUMMYUNIONNAME.item = tvi;
#endif
					
					hti = TreeView_InsertItem(GetDlgItem(hDlg,IDC_CUSTOM_TREE),&tvis);

					/* look for children of this custom folder */
					for (jj=0;jj<num_folders;jj++)
					{
						if (folders[jj]->m_nParent == i)
						{
							HTREEITEM hti_child;
							tvis.hParent = hti;
							tvis.hInsertAfter = TVI_SORT;
							tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
							tvi.pszText = folders[jj]->m_lpTitle;
							tvi.lParam = (LPARAM)folders[jj];
							tvi.iImage = GetTreeViewIconIndex(folders[jj]->m_nIconId);
							tvi.iSelectedImage = 0;
#if !defined(NONAMELESSUNION)
							tvis.item = tvi;
#else
							tvis.DUMMYUNIONNAME.item = tvi;
#endif							
							hti_child = TreeView_InsertItem(GetDlgItem(hDlg,IDC_CUSTOM_TREE),&tvis);
							if (folders[jj] == default_selection)
							    TreeView_SelectItem(GetDlgItem(hDlg,IDC_CUSTOM_TREE),hti_child);
						}
					}

					/*TreeView_Expand(GetDlgItem(hDlg,IDC_CUSTOM_TREE),hti,TVE_EXPAND);*/
					if (first_entry || folders[i] == default_selection)
					{
					    TreeView_SelectItem(GetDlgItem(hDlg,IDC_CUSTOM_TREE),hti);
						first_entry = FALSE;
					}

				}
				
			}
		}

		SetWindowText(GetDlgItem(hDlg,IDC_CUSTOMFILE_GAME),
		              UseLangList() ? _LSTW(driversw[driver_index]->description)
		                              : driversw[driver_index]->modify_the);

		return TRUE;
	}
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
		{
		   TVITEM tvi;
		   tvi.hItem = TreeView_GetSelection(GetDlgItem(hDlg,IDC_CUSTOM_TREE));
		   tvi.mask = TVIF_PARAM;
		   if (TreeView_GetItem(GetDlgItem(hDlg,IDC_CUSTOM_TREE),&tvi) == TRUE)
		   {
			  /* should look for New... */
		   
			  default_selection = (LPTREEFOLDER)tvi.lParam; /* start here next time */

			  AddToCustomFolder((LPTREEFOLDER)tvi.lParam,driver_index);
		   }

		   EndDialog(hDlg, 0);
		   return TRUE;

		   break;
		}
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;

		}
		break;
	}
	return 0;
}

INT_PTR CALLBACK DirectXDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND hEdit;

	const WCHAR *directx_help =
		TEXT(MAME32NAME " requires DirectX version 3 or later, which is a set of operating\r\n"
		"system extensions by Microsoft for Windows 9x, NT and 2000.\r\n\r\n"
		"Visit Microsoft's DirectX web page at http://www.microsoft.com/directx\r\n"
		"download DirectX, install it, and then run " MAME32NAME " again.\r\n");

	switch (Msg)
	{
	case WM_INITDIALOG:
		TranslateDialog(hDlg, lParam, FALSE);

		hEdit = GetDlgItem(hDlg, IDC_DIRECTX_HELP);
		if (OnNT())
		Edit_SetSel(hEdit, Edit_GetTextLength(hEdit), Edit_GetTextLength(hEdit));
		else
			Edit_SetSelA(hEdit, Edit_GetTextLengthA(hEdit), Edit_GetTextLengthA(hEdit));
		Edit_ReplaceSel(hEdit, _UIW(directx_help));
		return 1;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDB_WEB_PAGE)
			ShellExecuteA(GetMainWindow(), NULL, "http://www.microsoft.com/directx",
						 NULL, NULL, SW_SHOWNORMAL);

		if (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDB_WEB_PAGE)
			EndDialog(hDlg, 0);
		return 1;
	}
	return 0;
}

#ifdef USE_VIEW_PCBINFO
INT_PTR CALLBACK PCBInfoDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HBITMAP hBackground = GetBackgroundBitmap();

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			int  nGame;
			WCHAR szGame[MAX_PATH];
			WCHAR buf[MAX_PATH];
			char *stemp;
			const WCHAR *szDir = GetPcbinfoDir();
			mame_file *mfile;
			mame_file_error filerr;
			long filelen;
			int nParentIndex = -1;
			char *PcbData;
			LV_ITEM lvi;
			HWND hWndList = GetDlgItem(GetMainWindow(), IDC_LIST);
			LOGFONTW font;
			HFONT hPcbFont;
			HDC hDC;
			RECT rect;
			TEXTMETRIC tm;
			int nLines, nLineHeight;
			SCROLLINFO ScrollBar;

			TranslateDialog(hDlg, lParam, FALSE);

			hPcbInfo = hDlg;

			g_lpPcbInfoWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hDlg, IDC_PCBINFO), GWL_WNDPROC);
			SetWindowLong(GetDlgItem(hDlg, IDC_PCBINFO), GWL_WNDPROC, (LONG)PcbInfoWndProc);

			memset((void *)&font, 0, sizeof(font));
			font.lfHeight = -13;
			font.lfCharSet = ANSI_CHARSET;
			font.lfOutPrecision = OUT_DEFAULT_PRECIS;
			font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			font.lfQuality = DEFAULT_QUALITY;
			font.lfPitchAndFamily = FIXED_PITCH;
			lstrcpy(font.lfFaceName, TEXT("Courier New"));
//			lstrcpy(font.lfFaceName, TEXT("FixedSys"));

			hPcbFont = CreateFontIndirectW(&font);

			lvi.iItem = ListView_GetNextItem(hWndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
			if (lvi.iItem == -1)
				return 1;

			lvi.iSubItem = 0;
			lvi.mask	 = LVIF_PARAM;
			ListView_GetItem(hWndList, &lvi);

			nGame = lvi.lParam;
			nParentIndex = GetParentRomSetIndex(drivers[nGame]);
			if (nParentIndex >= 0)
				lstrcpy(szGame, driversw[nParentIndex]->name);
			else
				lstrcpy(szGame, driversw[nGame]->name);

			swprintf(buf, TEXT("%s\\%s.txt"), szDir, szGame);

			stemp = utf8_from_wstring(buf);
			filerr = mame_fopen(SEARCHPATH_DATAFILE, stemp, OPEN_FLAG_READ, &mfile);
			free(stemp);
			if (filerr != FILERR_NONE)
			{
				swprintf(buf, TEXT("%s\\pcb\\%s.txt"), szDir, szGame);
				stemp = utf8_from_wstring(buf);
				filerr = mame_fopen(SEARCHPATH_DATAFILE, stemp, OPEN_FLAG_READ, &mfile);
				free(stemp);
			}

			if (filerr == FILERR_NONE)
			{
				filelen = (long)mame_fsize(mfile);

				PcbData = (char *)malloc(filelen+1);

				if ( PcbData != NULL )
				{
					WCHAR buf[256];

					mame_fread(mfile, PcbData, filelen);
					
					PcbData[filelen] = '\0';

					swprintf(buf, 
						_UIW(TEXT(MAME32NAME " PCB Info: %s [%s]")), 
						ConvertAmpersandString(UseLangList() ?
							_LSTW(driversw[nGame]->description) :
							driversw[nGame]->modify_the), 
						driversw[nGame]->name);
					SetWindowText(hDlg, buf);
					SetWindowFont(GetDlgItem(hDlg, IDC_PCBINFO), hPcbFont, FALSE);
					Edit_SetTextA(GetDlgItem(hDlg, IDC_PCBINFO), PcbData);

					Edit_GetRect(GetDlgItem(hDlg, IDC_PCBINFO),&rect);
					nLines = Edit_GetLineCount(GetDlgItem(hDlg, IDC_PCBINFO) );
					GetListFont(&font);
					hDC = GetDC(GetDlgItem(hDlg, IDC_PCBINFO));
					GetTextMetrics (hDC, &tm);
					nLineHeight = tm.tmHeight - tm.tmInternalLeading;
					if( ( (rect.bottom - rect.top) / nLineHeight) < (nLines) )
					{
						//more than one Page, so show Scrollbar
						SetScrollRange(GetDlgItem(hDlg, IDC_PCBINFO), SB_VERT, 0, nLines, TRUE); 
					}
					else
					{
						//hide Scrollbar
						SetScrollRange(GetDlgItem(hDlg, IDC_PCBINFO),SB_VERT, 0, 0, TRUE);
					}

					ScrollBar.cbSize = sizeof(SCROLLINFO);
					ScrollBar.fMask = SIF_RANGE;
					GetScrollInfo(GetDlgItem(hDlg, IDC_PCBINFO), SB_HORZ, &ScrollBar);
					if( (ScrollBar.nMax - ScrollBar.nMin) < (rect.right - rect.left) )
					{
						//hide Scrollbar
						SetScrollRange(GetDlgItem(hDlg, IDC_PCBINFO),SB_HORZ, 0, 0, TRUE);
					}

//					ShowWindow(GetDlgItem(hDlg, IDC_PCBINFO), SW_SHOW);

					free(PcbData);
				}

				mame_fclose(mfile);
			}
			else
			{
				MessageBox(GetMainWindow(), _UIW(TEXT("No PCB Info available for this game.")), TEXT(MAME32NAME), MB_OK | MB_ICONEXCLAMATION);
				EndDialog(hDlg, 0);
			}

			return 1;
		}

		case WM_SIZE:
			{
				POINT p = {0, 0};
				RECT rect, fRect;
				DWORD dwStyle;
				DWORD dwStyleEx;
				SCROLLINFO ScrollBar;

				ClientToScreen(hDlg, &p);
				GetWindowRect(hDlg, &fRect);
				OffsetRect(&fRect, -p.x, -p.y);

				if (fRect.bottom < 50)
				{
					fRect.bottom = 50;
					SetWindowPos(hDlg, 0, 0, 0, fRect.right-fRect.left, fRect.bottom-fRect.top, SWP_NOMOVE);
				}

				if (fRect.right < 110)
				{
					fRect.right = 110;
					SetWindowPos(hDlg, 0, 0, 0, fRect.right-fRect.left, fRect.bottom-fRect.top, SWP_NOMOVE);
				}

				rect.left = 12;
				rect.top = 32;
				rect.right = fRect.right - 9;
				rect.bottom = fRect.bottom - 25;

				dwStyle   = GetWindowLong(GetDlgItem(hDlg, IDC_PCBINFO), GWL_STYLE);
				dwStyleEx = GetWindowLong(GetDlgItem(hDlg, IDC_PCBINFO), GWL_EXSTYLE);

				AdjustWindowRectEx(&rect, dwStyle, FALSE, dwStyleEx);
				MoveWindow(GetDlgItem(hDlg, IDC_PCBINFO),
						   fRect.left  + rect.left,
						   fRect.top   + rect.top,
						   rect.right  - rect.left,
						   rect.bottom - rect.top,
						   TRUE);

				GetWindowRect(GetDlgItem(hDlg, IDCANCEL), &rect);
				MoveWindow(GetDlgItem(hDlg, IDCANCEL),
						   (fRect.right - fRect.left)/2 - (rect.right-rect.left)/2 - 4,
						   fRect.bottom - (rect.bottom - rect.top) - 12,
						   rect.right - rect.left,
						   rect.bottom - rect.top,
						   TRUE);

				// Get H scoll bar useness
				ShowScrollBar(GetDlgItem(hDlg,IDC_PCBINFO), SB_HORZ, TRUE);
				ScrollBar.cbSize = sizeof(SCROLLINFO);
				ScrollBar.fMask = SIF_RANGE | SIF_PAGE;
				GetScrollInfo(GetDlgItem(hDlg,IDC_PCBINFO), SB_HORZ, &ScrollBar);
				if (ScrollBar.nPage > ScrollBar.nMax)
				{
					ShowScrollBar(GetDlgItem(hDlg,IDC_PCBINFO), SB_HORZ, FALSE);
				}

				// Get V scoll bar useness
				ShowScrollBar(GetDlgItem(hDlg,IDC_PCBINFO), SB_VERT, TRUE);
				ScrollBar.cbSize = sizeof(SCROLLINFO);
				ScrollBar.fMask = SIF_RANGE | SIF_PAGE;
				GetScrollInfo(GetDlgItem(hDlg,IDC_PCBINFO), SB_VERT, &ScrollBar);
				if (ScrollBar.nPage > ScrollBar.nMax)
				{
					ShowScrollBar(GetDlgItem(hDlg,IDC_PCBINFO), SB_VERT, FALSE);
				}

				ShowWindow(GetDlgItem(hDlg,IDC_PCBINFO),SW_SHOW);
				ShowWindow(GetDlgItem(hDlg,IDCANCEL),SW_SHOW);

				InvalidateRect(GetDlgItem(hDlg,IDC_PCBINFO),NULL,FALSE);
				return 1;
			}

		case WM_CTLCOLORSTATIC:
			if (GetBackgroundBitmap() && (HWND)lParam == GetDlgItem(hDlg, IDC_PCBINFO))
			{
				static HBRUSH hBrush=0;
				HDC hDC=(HDC)wParam;
				LOGBRUSH lb;

				if (hBrush)
					DeleteObject(hBrush);
				lb.lbStyle  = BS_HOLLOW;
				hBrush = CreateBrushIndirect(&lb);
				SetBkMode(hDC, TRANSPARENT);
				SetTextColor(hDC, GetListFontColor());
				return (LRESULT) hBrush;
			}
			break;

		case WM_PAINT:
			if (hBackground != NULL)
			{
				PaintBackgroundImage(hDlg, NULL, 0, 0);
				InvalidateRect(hDlg, NULL, FALSE);
			}
			break;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
			case IDCANCEL:
				EndDialog(hDlg, 0);
				return 1;
			}
	}
	return 0;
}

static LRESULT CALLBACK PcbInfoWndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HBITMAP hBackground = GetBackgroundBitmap();

	switch (Msg)
	{
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
			if (hBackground != NULL)
			{
				POINT p = { 0, 0 };

				/* get base point of background bitmap */
				MapWindowPoints(hDlg,hPcbInfo,&p,1);
				PaintBackgroundImage(hDlg, NULL, p.x, p.y);
				/* to ensure our parent procedure repaints the whole client area */
				InvalidateRect(hDlg, NULL, FALSE);
			}
			break;
	}
	return CallWindowProc(g_lpPcbInfoWndProc, hDlg, Msg, wParam, lParam);
}
#endif /* USE_VIEW_PCBINFO */

/***************************************************************************
    private functions
 ***************************************************************************/

static void DisableFilterControls(HWND hWnd, LPFOLDERDATA lpFilterRecord,
								  LPFILTER_ITEM lpFilterItem, DWORD dwFlags)
{
	HWND  hWndCtrl = GetDlgItem(hWnd, lpFilterItem->m_dwCtrlID);
	DWORD dwFilterType = lpFilterItem->m_dwFilterType;

	/* Check the appropriate control */
	if (dwFilterType & dwFlags)
		Button_SetCheck(hWndCtrl, MF_CHECKED);

	/* No special rules for this folder? */
	if (!lpFilterRecord)
		return;

	/* If this is an excluded filter */
	if (lpFilterRecord->m_dwUnset & dwFilterType)
	{
		/* uncheck it and disable the control */
		Button_SetCheck(hWndCtrl, MF_UNCHECKED);
		EnableWindow(hWndCtrl, FALSE);
	}

	/* If this is an implied filter, check it and disable the control */
	if (lpFilterRecord->m_dwSet & dwFilterType)
	{
		Button_SetCheck(hWndCtrl, MF_CHECKED);
		EnableWindow(hWndCtrl, FALSE);
	}
}

// Handle disabling mutually exclusive controls
static void EnableFilterExclusions(HWND hWnd, DWORD dwCtrlID)
{
	int 	i;
	DWORD	id;

	for (i = 0; i < NUM_EXCLUSIONS; i++)
	{
		// is this control in the list?
		if (filterExclusion[i] == dwCtrlID)
		{
			// found the control id
			break;
		}
	}

	// if the control was found
	if (i < NUM_EXCLUSIONS)
	{
		// find the opposing control id
		if (i % 2)
			id = filterExclusion[i - 1];
		else
			id = filterExclusion[i + 1];

		// Uncheck the other control
		Button_SetCheck(GetDlgItem(hWnd, id), MF_UNCHECKED);
	}
}

// Validate filter setting, mask out inappropriate filters for this folder
static DWORD ValidateFilters(LPFOLDERDATA lpFilterRecord, DWORD dwFlags)
{
	DWORD dwFilters;

	if (lpFilterRecord != (LPFOLDERDATA)0)
	{
		// Mask out implied and excluded filters
		dwFilters = lpFilterRecord->m_dwSet | lpFilterRecord->m_dwUnset;
		return dwFlags & ~dwFilters;
	}

	// No special cases - all filters apply
	return dwFlags;
}

static void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	int value;
	char tmp[4];
	if (hwndCtl == GetDlgItem(hwnd, IDC_CYCLETIMESEC))
	{
		value = SendDlgItemMessageA(hwnd,IDC_CYCLETIMESEC, TBM_GETPOS, 0, 0);
		itoa(value,tmp,10);
		SendDlgItemMessageA(hwnd,IDC_CYCLETIMESECTXT,WM_SETTEXT,0, (WPARAM)tmp);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_SCREENSHOT_BORDERSIZE))
	{
		value = SendDlgItemMessageA(hwnd,IDC_SCREENSHOT_BORDERSIZE, TBM_GETPOS, 0, 0);
		itoa(value,tmp,10);
		SendDlgItemMessageA(hwnd,IDC_SCREENSHOT_BORDERSIZETXT,WM_SETTEXT,0, (WPARAM)tmp);
	}
}