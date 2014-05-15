/****************************************************************************
**  Copyright (C) 2004  Miles Rausch                                      **
**                                                                        **
**  Alarm Docklet.  A docklet devoted to all your alarming needs,         **
**  featuring several different notification options and recurrence       **
**  options.                                                              **
**                                                                        **
**  This program is free software; you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation; either version 2 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program; if not, write to the Free Software           **
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA              **
**                                                02111-1307  USA         **
****************************************************************************/
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <commctrl.h>

#include "DockletSDK.h"
#include "resource1.h"


enum RecurHow { once, daily, week, weekend, recurnotset };
enum NotifyHow { song, popup, runprogram, shutdownwindows, notifynotset };


/****************************************************************
**  This struct, DOCKLET_DATA, is used to store information   **
**  in an "ini" file.  In this way, the user can have more    **
**  than one instance of the docklet running at a time and    **
**  the information won't get confused.  A pretty good idea.  **
****************************************************************/
typedef struct
{
	HWND hwndDocklet;
	HINSTANCE hInstanceDll;
	int intActive;
	int ALARM_ID;
	char stringName[50];
	char stringTimeAlarm[50];
	int intTimeAlarm;
	RecurHow szRecurValue;
	NotifyHow szNotifyAction;
	char stringNotifyActionValue[MAX_PATH];
} DOCKLET_DATA;


/****************************************************************
**  This area is where I defined all of my local functions    **
**  for use in my program.  Prototypes.  Variables.           **
****************************************************************/
int CALLBACK ConfigureDocklet(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

int randomID();
int GetTheTime();
BOOL MySystemShutdown();

void setName(DOCKLET_DATA *lpData);
void StartOver(DOCKLET_DATA *lpData);
void CancelAlarm(DOCKLET_DATA *lpData);
void UpdateAlarm(DOCKLET_DATA *lpData);


/****************************************************************
**  This is used in the dll creation.  I know nothing more.   **
****************************************************************/
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}


/****************************************************************
**  This function, "OnGetInformation", is called by the dock  **
**  when you go to add a docklet to your Objectdock. It will  **
**  display this text in the info section to the right.       **
****************************************************************/
void CALLBACK OnGetInformation(char *szName, char *szAuthor, int *iVersion, char *szNotes)
{
	strcpy(szName, "Alarming! Docklet");
	strcpy(szAuthor, "Miles Rausch");
	(*iVersion) = 100;
	strcpy(szNotes,   "A docklet devoted to all your alarming needs, featuring several different notification and recurrence options.  Please email me if you have any problems or suggestions: miles@awayken.com");
}


/****************************************************************
**  When the Docklet is created (added to Objectdock) it has  **
**  to create the ini and inigroup and to set up the docklet  **
**  with specifications you set.  Just a basic startup fun-   **
**  ction including the pointers for use in your other fun-   **
**  ctions as well.                                           **
****************************************************************/
DOCKLET_DATA *CALLBACK OnCreate(HWND hwndDocklet, HINSTANCE hInstance, char *szIni, char *szIniGroup)
{
	DOCKLET_DATA *lpData = new DOCKLET_DATA;
	ZeroMemory(lpData, sizeof(DOCKLET_DATA));

	lpData->hwndDocklet = hwndDocklet;
	lpData->hInstanceDll = hInstance;

	char szImageToLoad[MAX_PATH+10];
	strcpy(szImageToLoad, "");

	srand(GetTheTime());

	if(szIni && szIniGroup)
	{
		//Load options from INI
		GetPrivateProfileString(szIniGroup, "stringName", "", lpData->stringName, MAX_PATH, szIni);
		DockletSetLabel(lpData->hwndDocklet, "Click to Configure Alarming!");
	}
	else
	{
		//Set default options
		DockletGetRelativeFolder(lpData->hwndDocklet, szImageToLoad);
		strcat(szImageToLoad, "alarm.png");
		DockletSetImageFile(lpData->hwndDocklet, szImageToLoad);

		StartOver(lpData);
	}

	//Return handle to our plugin's personal data
	return lpData;
}

/****************************************************************
**  The docklet needs to monitor messages coming through the  **
**  Objectdock.  So, when the docklet processes a message,    **
**  it checks to see if the docklet is active, then it will   **
**  see if the timer is up - if it is, then it does it.       **
****************************************************************/
void CALLBACK OnProcessMessage(DOCKLET_DATA *lpData, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lpData->intActive == 1)
	{

		switch(uMsg)
		{
		case WM_TIMER:
			if(wParam == lpData->ALARM_ID)
				UpdateAlarm(lpData);

			break;
		}
	}

	return;
}


/****************************************************************
**  A basic "save" function that would have you specify all   **
**  the information you wish to maintain during the dock's    **
**  docklet's life.                                           **
****************************************************************/
void CALLBACK OnSave(DOCKLET_DATA *lpData, char *szIni, char *szIniGroup, BOOL bIsForExport)
{
	WritePrivateProfileString(szIniGroup, "stringName", lpData->stringName, szIni);
}


/****************************************************************
**  When you drag a docklet off of the dock, make sure that   **
**  you give your memory back so others can use it.           **
****************************************************************/
void CALLBACK OnDestroy(DOCKLET_DATA *lpData, HWND hwndDocklet)
{
	delete lpData;
}


/****************************************************************
**  What should I do when they want to configure?             **
****************************************************************/
int CALLBACK ConfigureDocklet (HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	DOCKLET_DATA *lpData = (DOCKLET_DATA *) GetProp(hDlg, "lpData");

	char stringQuick[20];
	strcpy(stringQuick, "");

	char stringNotify[20];
	strcpy(stringNotify, "");

	switch(iMsg)
	{

		/****************************************************************
		 **  When the dialog box is first initialized, then this case  **
		 **  will deal with how to set up the dialog box, including    **
		 **  remembering some previously set options.                  **
		 ****************************************************************/
	case WM_INITDIALOG:
		SetProp(hDlg, "lpData", (HANDLE) (char*) lParam);
		lpData = (DOCKLET_DATA *) GetProp(hDlg, "lpData");

		if (!lpData)
			return true;

		SetWindowText(GetDlgItem(hDlg, TEXT_NAME), lpData->stringName);

		SetWindowText(GetDlgItem(hDlg, TEXT_PARAM), lpData->stringNotifyActionValue);

		Button_SetCheck(GetDlgItem(hDlg, RADIO_STANDARD), TRUE);

		SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_RESETCONTENT, (WPARAM) NULL, (LPARAM) NULL);
		SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_INSERTSTRING, 0, (LPARAM) "10 min later");
		SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_INSERTSTRING, 1, (LPARAM) "30 min later");
		SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_INSERTSTRING, 2, (LPARAM) "1 hour later");
		SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_INSERTSTRING, 3, (LPARAM) "3 hours later");

		SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_RESETCONTENT, (WPARAM) NULL, (LPARAM) NULL);
		SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_INSERTSTRING, 0, (LPARAM) "Play a song file");
		SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_INSERTSTRING, 1, (LPARAM) "Pop up a message");
		SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_INSERTSTRING, 2, (LPARAM) "Run a program");
		SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_INSERTSTRING, 3, (LPARAM) "Shutdown Windows");

		switch(lpData->szRecurValue)
		{
		case once: Button_SetCheck(GetDlgItem(hDlg, RADIO_ONCE), TRUE);
			break;
		case daily: Button_SetCheck(GetDlgItem(hDlg, RADIO_DAILY), TRUE);
			break;
		case week: Button_SetCheck(GetDlgItem(hDlg, RADIO_WEEK), TRUE);
			break;
		case weekend: Button_SetCheck(GetDlgItem(hDlg, RADIO_WEEKEND), TRUE);
			break;
		case recurnotset: Button_SetCheck(GetDlgItem(hDlg, RADIO_ONCE), TRUE);
			break;
		}

		switch(lpData->szNotifyAction)
		{
		case song: SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_SETCURSEL, 0, (LPARAM) 0);
			break;
		case popup: SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_SETCURSEL, 1, (LPARAM) 0);
			break;
		case runprogram: SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_SETCURSEL, 2, (LPARAM) 0);
			break;
		case shutdownwindows: SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_SETCURSEL, 3, (LPARAM) 0);
			break;
		}

		break;


		/****************************************************************
		 **  This case decides what should be done with "Ok" is being  **
		 **  clicked.  It involved saving values and setting the       **
		 **  alarm, but it also has to deal with the fact that maybe   **
		 **  the user has already set the docklet and just wants to    **
		 **  to change some options.                                   **
		 ****************************************************************/
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (lpData->intActive == 0)
			{
				GetWindowText(GetDlgItem(hDlg, TEXT_NAME), lpData->stringName, sizeof(lpData->stringName));
				GetWindowText(GetDlgItem(hDlg, TEXT_PARAM), lpData->stringNotifyActionValue, sizeof(lpData->stringNotifyActionValue));

				lpData->ALARM_ID = randomID();
				char szTime[50];
				strcpy(szTime, "");

				if ( Button_GetCheck(GetDlgItem (hDlg, RADIO_STANDARD)) )
				{
					GetWindowText(GetDlgItem(hDlg, 1010), szTime, sizeof(szTime));

					int no = 0 ;
					int i = 0 ;

					while ( szTime[i] != '\0' ) {
						no = no * 10 ;
						no = no + ( szTime[i] - '0' ) ;
						i++ ;
					}

					lpData->intTimeAlarm = no;
					strcpy(lpData->stringTimeAlarm, szTime);
					setName(lpData);

					SetTimer(lpData->hwndDocklet, lpData->ALARM_ID, (1000 * 60) * lpData->intTimeAlarm, NULL);
					lpData->intActive = 1;
				}

				else if ( Button_GetCheck(GetDlgItem (hDlg, RADIO_QUICK)) )
				{
					SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_GETLBTEXT, SendMessage(GetDlgItem(hDlg, DROP_QUICK), CB_GETCURSEL, (WPARAM) 0, 0), (LPARAM) stringQuick);

					int no = 0;

					if(strlen(stringQuick) > 0)
					{
						if (strcmp(stringQuick, "10 min later") == 0) {
							no = 10;
							strcpy(szTime, "10"); }
						else if (strcmp(stringQuick, "30 min later") == 0) {
							no = 30;
							strcpy(szTime, "30"); }
						else if (strcmp(stringQuick, "1 hour later") == 0) {
							no = 60;
							strcpy(szTime, "60"); }
						else if (strcmp(stringQuick, "3 hours later") == 0) {
							no = 180;
							strcpy(szTime, "180"); }

					}

					lpData->intTimeAlarm = no;
					strcpy(lpData->stringTimeAlarm, szTime);
					setName(lpData);

					SetTimer(lpData->hwndDocklet, lpData->ALARM_ID, (1000 * 60) * lpData->intTimeAlarm, NULL);
					lpData->intActive = 1;
				}
			}

			if (lpData->intActive == 1)
			{
				if ( Button_GetCheck(GetDlgItem (hDlg, RADIO_ONCE)) ) { lpData->szRecurValue = once; }

				if ( Button_GetCheck(GetDlgItem (hDlg, RADIO_DAILY)) ) { lpData->szRecurValue = daily; }

				if ( Button_GetCheck(GetDlgItem (hDlg, RADIO_WEEK)) ) { lpData->szRecurValue = week; }

				if ( Button_GetCheck(GetDlgItem (hDlg, RADIO_WEEKEND)) ) { lpData->szRecurValue = weekend; }

				SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_GETLBTEXT, SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_GETCURSEL, (WPARAM) 0, 0), (LPARAM) stringNotify);
				if(strlen(stringNotify) > 0)
				{
					if (strcmp(stringNotify, "Play a song file") == 0) { lpData->szNotifyAction = song; }
					else if (strcmp(stringNotify, "Pop up a message") == 0) { lpData->szNotifyAction = popup; }
					else if (strcmp(stringNotify, "Run a program") == 0) { lpData->szNotifyAction = runprogram; }
					else if (strcmp(stringNotify, "Shutdown Windows") == 0) { lpData->szNotifyAction = shutdownwindows; }

				}
			}

			EndDialog(hDlg, 0);
			break;


		/****************************************************************
		 **  What happens when the user clicks the dropdown for the    **
		 **  quick alarm?                                              **
		 ****************************************************************/
		case DROP_QUICK:
			Button_SetCheck(GetDlgItem(hDlg, RADIO_STANDARD), FALSE);
			Button_SetCheck(GetDlgItem(hDlg, RADIO_QUICK), TRUE);
			break;


		/****************************************************************
		 **  What happens when the user clicks the dropdown for the    **
		 **  notification options.                                     **
		 ****************************************************************/
		case DROP_NOTIFY:
			SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_GETLBTEXT, SendMessage(GetDlgItem(hDlg, DROP_NOTIFY), CB_GETCURSEL, (WPARAM) 0, 0), (LPARAM) stringNotify);

			if (strcmp(stringNotify, "Play a song file") == 0) { SetWindowText(GetDlgItem(hDlg, TEXT_PARAM), "Type song file's exact path here."); }
			else if (strcmp(stringNotify, "Pop up a message") == 0) { SetWindowText(GetDlgItem(hDlg, TEXT_PARAM), "Type popup message here."); }
			else if (strcmp(stringNotify, "Run a program") == 0) { SetWindowText(GetDlgItem(hDlg, TEXT_PARAM), "Type program's exact path here."); }
			else if (strcmp(stringNotify, "Shutdown Windows") == 0) { SetWindowText(GetDlgItem(hDlg, TEXT_PARAM), "Leave this blank."); }
			break;


		/****************************************************************
		 **  What happens when the user clicks the cancel button?      **
		 ****************************************************************/
		case IDCLOSE:
			EndDialog(hDlg, 0);
			break;
		}
		break;


		/****************************************************************
		 **  When the dialog box is destroyed, this happens.           **
		 ****************************************************************/
	case WM_DESTROY:
		RemoveProp(hDlg, "lpdata");
		break;
	}

	return false;
}

/****************************************************************
**  What should I do when they want to configure?             **
****************************************************************/
void CALLBACK OnConfigure(DOCKLET_DATA *lpData)
{
	DialogBoxParam(lpData->hInstanceDll, MAKEINTRESOURCE(IDD_CONFIG), lpData->hwndDocklet, ConfigureDocklet, (LPARAM) lpData);
}

/****************************************************************
**  When the user clicks the docklet, it will either allow    **
**  then to configure the docklet or, if it is active, it     **
**  offers them the choice to cancel the alarm.               **
****************************************************************/
BOOL CALLBACK OnLeftButtonClick(DOCKLET_DATA *lpData, POINT *ptCursor, SIZE *sizeDocklet)
{
	if( lpData->intActive == 0 ) { OnConfigure(lpData); }
	else { CancelAlarm(lpData); }

	return TRUE;
}


/****************************************************************
**  What should I do when they click the right mouse button?  **
****************************************************************/
BOOL CALLBACK OnRightButtonClick(DOCKLET_DATA *lpData, POINT *ptCursor, SIZE *sizeDocklet)
{
	HMENU hMenu = CreatePopupMenu();

	AppendMenu(hMenu, MF_STRING | MF_ENABLED, 1, "Configure Your Alarm");
	AppendMenu(hMenu, MF_STRING | MF_ENABLED, 2, "Cancel Current Alarm");


	POINT ptMenu;
	GetCursorPos(&ptMenu);
	DockletLockMouseEffect(lpData->hwndDocklet, TRUE);	//Lock the dock's zooming while our menu is in use.
	int iCommand = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, ptMenu.x, ptMenu.y, 0, lpData->hwndDocklet, NULL);
	DockletLockMouseEffect(lpData->hwndDocklet, FALSE);
	DestroyMenu(hMenu);
	if(iCommand <= 0)
		return TRUE;


	switch(iCommand)
	{
	case 1:
		OnConfigure(lpData);
		break;
	case 2:
		CancelAlarm(lpData);
		break;
	}

	return TRUE;
}


/****************************************************************
**  This is the function that runs the notification.  It is   **
**  called when the timer is up.  All along the way we check  **
**  to see if the alarm is indeed active.  Assuming that the  **
**  timer is up, we then deal with the reoccurrence first,    **
**  then we deal with the notifications.  It is done in this  **
**  way for the reason that one of the notifications is to    **
**  shut your system down.                                    **
****************************************************************/
void UpdateAlarm(DOCKLET_DATA *lpData)
{
	if (lpData->intActive == 1)
	{
		KillTimer(lpData->hwndDocklet, lpData->ALARM_ID);

		struct tm *theTime;
		time_t long_time;

		time( &long_time );
		theTime = localtime( &long_time );

		switch(lpData->szRecurValue)
		{
		case once:
			DockletSetLabel(lpData->hwndDocklet, "Click to Configure Alarming!");
			lpData->intActive = 0;
			break;
		case daily:
			SetTimer(lpData->hwndDocklet, lpData->ALARM_ID, ((1000 * 60) * 60) * 24, NULL);
			strcpy(lpData->stringTimeAlarm, "1440");
			setName(lpData);
			lpData->intActive = 1;
			break;
		case week:
			if ( (theTime->tm_wday > 0) && (theTime->tm_wday < 6) )
			{
				SetTimer(lpData->hwndDocklet, lpData->ALARM_ID, ((1000 * 60) * 60) * 24, NULL);
				strcpy(lpData->stringTimeAlarm, "1440");
				setName(lpData);
				lpData->intActive = 1;
			}
			break;
		case weekend:
			if ( (theTime->tm_wday == 0) || (theTime->tm_wday == 6) )
			{
				SetTimer(lpData->hwndDocklet, lpData->ALARM_ID, ((1000 * 60) * 60) * 24, NULL);
				strcpy(lpData->stringTimeAlarm, "1440");
				setName(lpData);
				lpData->intActive = 1;
			}
			break;
		}

		switch(lpData->szNotifyAction)
		{
		case song:	ShellExecute(lpData->hwndDocklet, "open", lpData->stringNotifyActionValue, NULL, NULL, SW_SHOWNORMAL);
			break;
		case popup: MessageBox(NULL, lpData->stringNotifyActionValue, lpData->stringName, MB_OK | MB_ICONEXCLAMATION);
			break;
		case runprogram: ShellExecute(lpData->hwndDocklet, "open", lpData->stringNotifyActionValue, NULL, NULL, SW_SHOWNORMAL);
			break;
		case shutdownwindows: MySystemShutdown();
			break;
		}
	}
}


/****************************************************************
**  This function originally had other purposes but now it    **
**  is used to seed the random number generator.              **
****************************************************************/
int GetTheTime()
{
	struct tm *theTime;
	time_t long_time;
	int returnWhat;

	time( &long_time );
	theTime = localtime( &long_time );

	returnWhat = theTime->tm_sec;

	return returnWhat;
}


/****************************************************************
**  Makes sure that every docklet starts out the same way.    **
**  This is great for first initializing a docklet or for     **
**  resetting a docklet.                                      **
****************************************************************/
void StartOver(DOCKLET_DATA *lpData)
{
	DockletSetLabel(lpData->hwndDocklet, "Click to Configure Alarming!");
	lpData->intTimeAlarm = 0;
	lpData->intActive = 0;
	lpData->ALARM_ID = 0;
	strcpy(lpData->stringName, "New Alarm");
	strcpy(lpData->stringTimeAlarm, "");
	strcpy(lpData->stringNotifyActionValue, "");
	lpData->szNotifyAction = notifynotset;
	lpData->szRecurValue = recurnotset;
	KillTimer(lpData->hwndDocklet, lpData->ALARM_ID);
}


/****************************************************************
**  Offers up the choice to cancel the alarm or not.          **
****************************************************************/
void CancelAlarm(DOCKLET_DATA *lpData)
{
	char szMessage[1000];
	sprintf(szMessage, "Do you wish to cancel the current alarm?");
	if ( MessageBox(NULL, szMessage, "Alarming!", MB_YESNO | MB_ICONEXCLAMATION) == IDYES )
	{
		StartOver(lpData);
	}
}


/****************************************************************
**  Gives the docklet a random ID number (for if you have     **
**  more than one docklet.)                                   **
****************************************************************/
int randomID()
{
	return rand();
}


/****************************************************************
**  Neat little function that changes the name that shows up  **
**  when you mouse over the docklet.  It will tell you the    **
**  name of the alarm and how many minutes it should last.    **
****************************************************************/
void setName(DOCKLET_DATA *lpData)
{
	char szName[1000];
	strcpy(szName, lpData->stringName);
	strcat(szName, " lasts ");
	strcat(szName, lpData->stringTimeAlarm);
	strcat(szName, " minutes.");
	DockletSetLabel(lpData->hwndDocklet, szName);
}


/****************************************************************
**  This code was found on MSDN.  I don't understand it very  **
**  well, but they said that this would shut down windows,    **
**  and it does.                                              **
****************************************************************/
BOOL MySystemShutdown()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	// Get a token for this process.

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return( FALSE );

	// Get the LUID for the shutdown privilege.

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get the shutdown privilege for this process.

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	if (GetLastError() != ERROR_SUCCESS)
		return FALSE;

	// Shut down the system and force all applications to close.

	if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0))
		return FALSE;

	return TRUE;
}