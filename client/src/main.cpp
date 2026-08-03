/*
	Updated to 0.3.7 by P3ti
*/

#include "main.h"

RakClientInterface *pRakClient = NULL;
int iAreWeConnected = 0, iConnectionRequested = 0, iSpawned = 0, iGameInited = 0, iSpawnsAvailable = 0;
int iReconnectTime = 2 * 1000, iNotificationDisplayedBeforeSpawn = 0;

PLAYERID g_myPlayerID;
char g_szNickName[32];

struct stPlayerInfo playerInfo[MAX_PLAYERS];
struct stVehiclePool vehiclePool[MAX_VEHICLES];

FILE *flLog = NULL, *flTextDrawsLog = NULL;

DWORD dwAutoRunTick = GetTickCount();

extern int iMoney, iDrunkLevel, iLocalPlayerSkin;
extern BOOL bIsSpectating;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	srand((unsigned int)GetTickCount());

	char szModulePath[MAX_PATH];
	GetModuleFileNameA(NULL, szModulePath, MAX_PATH);
	char *pLastSlash = strrchr(szModulePath, '\\');
	if (pLastSlash)
	{
		*pLastSlash = '\0';
		SetCurrentDirectoryA(szModulePath);
	}

	char szConfigFile[256] = "RakSAMPClient.xml";
	char szOverrideNick[32] = "";
	char szOverrideAddr[256] = "";
	int iOverridePort = 0;
	char szOverridePass[32] = "";

	int argc = 0;
	LPWSTR *argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argvW)
	{
		for (int i = 1; i < argc; i++)
		{
			char arg[512];
			wcstombs(arg, argvW[i], sizeof(arg));
			if ((!strcmp(arg, "-c") || !strcmp(arg, "--config")) && i + 1 < argc)
			{
				wcstombs(szConfigFile, argvW[++i], sizeof(szConfigFile));
			}
			else if ((!strcmp(arg, "-n") || !strcmp(arg, "--nick")) && i + 1 < argc)
			{
				wcstombs(szOverrideNick, argvW[++i], sizeof(szOverrideNick));
			}
			else if ((!strcmp(arg, "-h") || !strcmp(arg, "--host")) && i + 1 < argc)
			{
				wcstombs(szOverrideAddr, argvW[++i], sizeof(szOverrideAddr));
			}
			else if ((!strcmp(arg, "-p") || !strcmp(arg, "--port")) && i + 1 < argc)
			{
				iOverridePort = _wtoi(argvW[++i]);
			}
			else if ((!strcmp(arg, "-pass") || !strcmp(arg, "--password")) && i + 1 < argc)
			{
				wcstombs(szOverridePass, argvW[++i], sizeof(szOverridePass));
			}
		}
		LocalFree(argvW);
	}

	// load up settings
	if(!LoadSettings(szConfigFile))
	{
		Log("Failed to load settings");
		getchar();
		return 0;
	}

	if (szOverrideNick[0])
		strncpy(settings.server.szNickname, szOverrideNick, sizeof(settings.server.szNickname) - 1);
	if (szOverrideAddr[0])
		strncpy(settings.server.szAddr, szOverrideAddr, sizeof(settings.server.szAddr) - 1);
	if (iOverridePort > 0)
		settings.server.iPort = iOverridePort;
	if (szOverridePass[0])
		strncpy(settings.server.szPassword, szOverridePass, sizeof(settings.server.szPassword) - 1);

	if(settings.iConsole)
		SetUpConsole();
	else
	{
		SetUpWindow(hInstance);
		Sleep(500); // wait a bit for the dialog to create
	}

	// RCON mode
	if(settings.runMode == RUNMODE_RCON)
	{
		if(RCONReceiveLoop())
		{
			if(flLog != NULL)
			{
				fclose(flLog);
				flLog = NULL;
			}

			return 0;
		}
	}	

	// set up networking
	pRakClient = RakNetworkFactory::GetRakClientInterface();
	if(pRakClient == NULL)
		return 0;

	pRakClient->SetMTUSize(576);

	resetPools(1, 0);
	RegisterRPCs(pRakClient);

	SYSTEMTIME time;
	GetLocalTime(&time);
	if(settings.iConsole)
	{
		Log(" ");
		Log("* ===================================================== *");
		Log("  RakSAMP " RAKSAMP_VERSION " initialized on %02d/%02d/%02d %02d:%02d:%02d",
			time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond);
		Log("  Authors: " AUTHOR "");
		Log("* ===================================================== *");
		Log(" ");
	}

	char szInfo[400];
	char szLastInfo[400];
	
	int iLastMoney = iMoney;
	int iLastDrunkLevel = iDrunkLevel;

	int iLastStatsUpdate = GetTickCount();
	
	while(1)
	{
		UpdateNetwork(pRakClient);

		if(settings.bSpam)
			sampSpam();

		if (settings.bFakeKill)
			sampFakeKill();

		if (settings.bLag)
			sampLag();

		if (settings.bJoinFlood)
			sampJoinFlood();

		if (settings.bChatFlood)
			sampChatFlood();

		if (settings.bClassFlood)
			sampClassFlood();

		processPulsator();
		processBulletFlood();

		if (!iConnectionRequested)
		{
			if(!iGettingNewName)
				sampConnect(settings.server.szAddr, settings.server.iPort, settings.server.szNickname, settings.server.szPassword, pRakClient);
			else
				sampConnect(settings.server.szAddr, settings.server.iPort, g_szNickName, settings.server.szPassword, pRakClient);

			iConnectionRequested = 1;
		}

		if (iAreWeConnected && iGameInited)
		{
			static DWORD dwLastInfoUpdate = GetTickCount();
			if(dwLastInfoUpdate && dwLastInfoUpdate < (GetTickCount() - 1000))
			{
				char szHealthText[16], szArmourText[16];

				if(settings.fPlayerHealth > 200.0f)
					sprintf_s(szHealthText, sizeof(szHealthText), "N/A");
				else
					sprintf_s(szHealthText, sizeof(szHealthText), "%.2f", settings.fPlayerHealth);

				if(settings.fPlayerArmour > 200.0f)
					sprintf_s(szArmourText, sizeof(szArmourText), "N/A");
				else
					sprintf_s(szArmourText, sizeof(szArmourText), "%.2f", settings.fPlayerArmour);

				sprintf_s(szInfo, 400, "Hostname: %s     Players: %d     Ping: %d     Authors: %s\nHealth: %s     Armour: %s     Skin: %d     X: %.4f     Y: %.4f     Z: %.4f     Rotation: %.4f",
				g_szHostName, getPlayerCount(), playerInfo[g_myPlayerID].dwPing, AUTHOR, szHealthText, szArmourText, iLocalPlayerSkin, settings.fNormalModePos[0], settings.fNormalModePos[1], settings.fNormalModePos[2], settings.fNormalModeRot);
				
				if(strcmp(szInfo, szLastInfo) != 0)
				{
					SetWindowText(texthwnd, szInfo);
					sprintf_s(szLastInfo, szInfo);
				}
			}

			if (settings.iUpdateStats)
			{
				if((GetTickCount() - iLastStatsUpdate >= 1000) || iMoney != iLastMoney || iDrunkLevel != iLastDrunkLevel)
				{
					RakNet::BitStream bsSend;

					bsSend.Write((BYTE)ID_STATS_UPDATE);

					iDrunkLevel -= (rand() % settings.iMaxFPS + settings.iMinFPS);

					if(iDrunkLevel < 0)
						iDrunkLevel = 0;

					bsSend.Write(iMoney);
					bsSend.Write(iDrunkLevel);

					pRakClient->Send(&bsSend, HIGH_PRIORITY, RELIABLE, 0);

					iLastMoney = iMoney;
					iLastDrunkLevel = iDrunkLevel;

					iLastStatsUpdate = GetTickCount();
				}
			}

			if(settings.runMode == RUNMODE_BARE)
				goto bare;

			if(!iSpawned)
			{
				if(settings.iManualSpawn != 0)
				{
					if(!iNotificationDisplayedBeforeSpawn)
					{
						sampRequestClass(settings.iClassID);
						
						Log("Please write !spawn into the console when you're ready to spawn.");

						iNotificationDisplayedBeforeSpawn = 1;
					}
				}
				else
				{
					sampRequestClass(settings.iClassID);
					sampSpawn();

					iSpawned = 1;
					iNotificationDisplayedBeforeSpawn = 1;
				}
			}
			else
			{
				if(settings.runMode == RUNMODE_STILL)
				{
					// Nothing left to do. :-)
				}

				if(settings.runMode == RUNMODE_NORMAL)
				{
					if(!bIsSpectating)
					{
						if(settings.AutoGotoCP && settings.CurrentCheckpoint.bActive)
						{
							settings.fNormalModePos[0] = settings.CurrentCheckpoint.fPosition[0];
							settings.fNormalModePos[1] = settings.CurrentCheckpoint.fPosition[1];
							settings.fNormalModePos[2] = settings.CurrentCheckpoint.fPosition[2];
						}

						onFootUpdateAtNormalPos();
					}
					else
						spectatorUpdate();
				}

				// Run autorun commands
				if(settings.iAutorun)
				{
					if(dwAutoRunTick && dwAutoRunTick < (GetTickCount() - 2000))
					{
						static int autorun;
						if(!autorun)
						{
							Log("Loading autorun...");
							for(int i = 0; i < MAX_AUTORUN_CMDS; i++)
								if(settings.autoRunCMDs[i].iExists)
									RunCommand(settings.autoRunCMDs[i].szCMD, 1);

							autorun = 1;
						}
					}
				}

				// Following player mode.
				if(settings.runMode == RUNMODE_FOLLOWPLAYER)
				{
					PLAYERID copyingID = getPlayerIDFromPlayerName(settings.szFollowingPlayerName);
					if(copyingID != (PLAYERID)-1)
						onFootUpdateFollow(copyingID);
				}

				// Following a player with a vehicle mode.
				if(settings.runMode == RUNMODE_FOLLOWPLAYERSVEHICLE)
				{
					PLAYERID copyingID = getPlayerIDFromPlayerName(settings.szFollowingPlayerName);
					if(copyingID != (PLAYERID)-1)
						inCarUpdateFollow(copyingID, (VEHICLEID)settings.iFollowingWithVehicleID);
				}

			}
		}

bare:;
		Sleep(30);
	}

	if(flLog != NULL)
	{
		fclose(flLog);
		flLog = NULL;
	}

	if(flTextDrawsLog != NULL)
	{
		fclose(flTextDrawsLog);
		flTextDrawsLog = NULL;
	}

	return 0;
}

void Log(char *fmt, ...)
{
	if(flLog == NULL)
	{
		flLog = fopen("RakSAMPClient.log", "a");

		if(flLog == NULL)
			return;
	}

	SYSTEMTIME time;
	GetLocalTime(&time);

	fprintf(flLog, "[%02d:%02d:%02d.%03d] ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

	if(settings.iPrintTimestamps && settings.iConsole)
		printf("[%02d:%02d:%02d.%03d] ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

	char buffer[512];
	memset(buffer, 0, 512);

	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, 512, fmt, args);
	va_end(args);

	fprintf(flLog, buffer);

	if(settings.iConsole)
	{
		printf(buffer);
	}
	else
	{
		LPTSTR tbuf = new TCHAR[512];
		wsprintf(tbuf, buffer);

		int lbCount = SendMessage(loghwnd, LB_GETCOUNT, 0, 0);
		WPARAM idx = SendMessage(loghwnd, LB_ADDSTRING, 0, (LPARAM)tbuf);

		SendMessage(loghwnd, LB_SETCURSEL, lbCount - 1, 0);
		SendMessage(loghwnd, LB_SETTOPINDEX, idx, 0);
	}

	fprintf(flLog, "\n");

	if(settings.iConsole)
		printf("\n");

	fflush(flLog);
}

void SaveTextDrawData ( WORD wTextID, TEXT_DRAW_TRANSMIT *pData, CHAR* cText )
{
	if ( flTextDrawsLog == NULL )
	{
		flTextDrawsLog = fopen( "TextDraws.log", "a" );

		if ( flTextDrawsLog == NULL )
			return;
	}

	fprintf( flTextDrawsLog, "TextDraw ID: %d, Text: %s\n", wTextID, cText );
	fprintf( flTextDrawsLog, "Flags: box(%i), left(%i), right(%i), center(%i), proportional(%i), padding(%i)\n", pData->byteBox, pData->byteLeft, pData->byteRight, pData->byteCenter, pData->byteProportional, pData->bytePadding );
	fprintf( flTextDrawsLog, "LetterWidth: %.3f, LetterHeight: %.3f, LetterColor: %X, LineWidth: %.3f, LineHeight: %.3f\n", pData->fLetterWidth, pData->fLetterHeight, pData->dwLetterColor, pData->fLineWidth, pData->fLineHeight );
	fprintf( flTextDrawsLog, "BoxColor: %X, Shadow: %i, Outline: %i, BackgroundColor: %X, Style: %i, Selectable: %i\n", pData->dwBoxColor, pData->byteShadow, pData->byteOutline, pData->dwBackgroundColor, pData->byteStyle, pData->byteSelectable );
	fprintf( flTextDrawsLog, "X: %.3f, Y: %.3f, ModelID: %d, RotX: %.3f, RotY: %.3f, RotZ: %.3f, Zoom: %.3f, Colors: %d, %d", pData->fX, pData->fY, pData->wModelID, pData->fRotX, pData->fRotY, pData->fRotZ, pData->fZoom, pData->wColor1, pData->wColor2 );

	fprintf( flTextDrawsLog, "\n\n" );

	fflush( flTextDrawsLog );
}

void gen_random(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i)
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	s[len] = 0;
}
