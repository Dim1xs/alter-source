#include "cbase.h"
#include "gamemounts.h"
#include "steam/steam_api.h"
#include "tier1/strtools.h"
#include "tier1/fmtstr.h"
#include "lua/luahandle.h"
#include "filesystem.h"

#define MAX_LINE_LENGTH 256

ConVar portal_mounted("portal_mounted", "0", FCVAR_REPLICATED, "Indicates if Portal is mounted");
ConVar tf2_mounted("tf2_mounted", "0", FCVAR_REPLICATED, "Indicates if Team Fortress 2 is mounted");

/*void FreePath(char* path) {
	free(path);
}*/

char* GetPath() {
	return "C:\\Program Files (x86)\\Steam\\steamapps\\sourcemods\\altersrc\\addons";
	// according to my calculations everyone must have the mod installed
	// in this path because otherwise addons wont work :nerd:
}

void LoadAddons()
{
	//FreePath(GetPath());

	char* addonsFolder = GetPath();
	if (!addonsFolder)
	{
		Warning("addon folder not found\n");
		return;
	}

	const char *pszAddonDir = "addons\\*";

	FileFindHandle_t findHandle;
	const char *pszAddonName = filesystem->FindFirst(pszAddonDir, &findHandle);

	if (!pszAddonName)
	{
		ConMsg("no addons found in directory: %s\n", pszAddonDir);
		return;
	}

	do
	{
		if (V_stricmp(pszAddonName, ".") != 0 && V_stricmp(pszAddonName, "..") != 0 && V_stricmp(pszAddonName, "checkers") != 0 && V_stricmp(pszAddonName, "chess") != 0
			&& V_stricmp(pszAddonName, "common") != 0 && V_stricmp(pszAddonName, "go") != 0 && V_stricmp(pszAddonName, "hearts") != 0 && V_stricmp(pszAddonName, "spades") != 0
			&& V_stricmp(pszAddonName, "���mportal") != 0 && V_stricmp(pszAddonName, "addons") != 0 && V_stricmp(pszAddonName, "maps") != 0 && V_stricmp(pszAddonName, "materials") != 0
			&& V_stricmp(pszAddonName, "sound") != 0 && V_stricmp(pszAddonName, "models") != 0 && V_stricmp(pszAddonName, "particles") != 0)
		{
			char szFullPath[MAX_PATH];
			Q_snprintf(szFullPath, sizeof(szFullPath), "%s\\%s", addonsFolder, pszAddonName);
			ConMsg("found addon dir: %s\n", szFullPath);
			MountAddon(szFullPath);
			ConMsg("mounted addon: %s\n", pszAddonName);

			char szInitLuaPath[MAX_PATH];
			Q_snprintf(szInitLuaPath, sizeof(szInitLuaPath), "%s\\%s\\lua\\init.lua", addonsFolder, pszAddonName);

			if (filesystem->FileExists(szInitLuaPath))
			{
				ConMsg("found init.lua for addon %s at %s\n", pszAddonName, szInitLuaPath);
				Lua()->InitDll();
				LuaHandle* lua = new LuaHandle();
				lua->LoadLua(szInitLuaPath);
			}
			else
			{
				ConMsg("init.lua not found for addon %s\n", pszAddonName);
				return;
			}
		}
		pszAddonName = filesystem->FindNext(findHandle);
	} while (pszAddonName);

	filesystem->FindClose(findHandle);

	//FreePath(addonsFolder);
}

void MountAddon(const char *pszAddonName)
{
	char szFullPath[MAX_PATH];
	Q_snprintf(szFullPath, sizeof(szFullPath), "%s", pszAddonName);

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/models", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/models", szFullPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/sound", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/sound", szFullPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/materials", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/materials", szFullPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/particles", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/particles", szFullPath), "GAME");

	g_pFullFileSystem->AddSearchPath(CFmtStr("%s/maps", szFullPath), "GAME");
	g_pFullFileSystem->AddSearchPath(CFmtStr("addons/%s/maps", szFullPath), "GAME");

	g_pFullFileSystem->AddSearchPath(szFullPath, "GAME");
}

void MountGames()
{
	if (!steamapicontext || !steamapicontext->SteamApps())
	{
		Warning("Steam API context is not available\n");
		return;
	}

	FileHandle_t hFile = filesystem->Open("cfg/mounts.cfg", "rt");

	if (!hFile)
	{
		Warning("failed to open mounts.cfg file\n");
		return;
	}

	char szLine[MAX_LINE_LENGTH];
	while (filesystem->ReadLine(szLine, sizeof(szLine), hFile))
	{
		if (szLine[0] == '\0' || szLine[0] == '/' || szLine[0] == '#')
			continue;

		int appID;
		char szGameName[MAX_LINE_LENGTH];
		char szPath[MAX_PATH * 2];
		if (sscanf(szLine, "%d : %s \"%[^\"]\"", &appID, szGameName, szPath) == 3)
		{
			if (Q_stricmp(szGameName, "portal") == 0)
			{
				portal_mounted.SetValue(1);
			}

			if (Q_stricmp(szGameName, "tf") == 0)
			{
				tf2_mounted.SetValue(1);
			}

			if (Q_stricmp(szGameName, "episodic") == 0)
			{
				cvar->FindVar("hl2_episodic")->SetValue(1);
			}

			// Adding main VPK files
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_textures.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_misc.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_misc.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_pak_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/fallbacks_pak_dir.vpk", szPath, szGameName), "GAME");

			// Adding directories
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s", szPath, szGameName), "GAME");

			// Add other common VPK files
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_textures_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_misc_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_dir.vpk", szPath, szGameName, szGameName), "GAME");

			// Adding potential localization files
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/scenes", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_english_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_french_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_german_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_russian_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_spanish_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_italian_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_japanese_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_chinese_dir.vpk", szPath, szGameName, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/%s_sound_vo_korean_dir.vpk", szPath, szGameName, szGameName), "GAME");

			// Add more specific paths as necessary
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/maps", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/models", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/materials", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/sounds", szPath, szGameName), "GAME");

			// Add search paths for mod support if applicable
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/custom", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/addons", szPath, szGameName), "GAME");

			// Add any other known or commonly used directories
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/updates", szPath, szGameName), "GAME");
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s/%s/dlc", szPath, szGameName), "GAME");

			// Ensure the root path is also included as a fallback
			g_pFullFileSystem->AddSearchPath(CFmtStr("%s", szPath), "GAME");

			ConMsg("successfuly mounted appID %d (%s)\n", appID, szGameName);

			//numGamesLoaded++;
		}
		else
		{
			Warning("invalid line format in mounts.cfg: %s\n", szLine);
			//numGamesLoaded = 0;
		}
	}

	//ConMsg("successfuly loaded %s games\n", numGamesLoaded);

	filesystem->Close(hFile);
}