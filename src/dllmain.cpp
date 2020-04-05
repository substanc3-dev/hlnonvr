// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Minhook.h>
#include <thread>
#include <cstring>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)
const char* message = "Created by Jan4V, half-life:alyx only, not other source 2 games";

typedef void(__fastcall* source2)(HINSTANCE, HINSTANCE, LPWSTR, long long, const char*, const char*);
typedef char(__fastcall* vrinit)(char*);
typedef __int64(__fastcall* dedicatedinit)(__int64, __int64, __int64);
typedef __int64(__fastcall* cl_cvar)(__int64, __int64);

LPVOID origVRInit, origDedicatedServer, origConvarSetup;


char __fastcall VRInitHook(char* settings)
{
    settings[299] = 0; //VRMode = false;

    return ((vrinit)origVRInit)(settings);
}
__int64 __fastcall DedicatedServerHook(__int64 rcx, __int64 rdx, __int64 r8)
{
    *(DWORD*)(rdx + 100) = 64; //MaxMaxPlayers = 64;
    *(char*)(rdx + 88) = 1; //SteamMode = true;

    char* line = GetCommandLineA();

    if (const char* param = strstr(line, "-maxplayers "))
    {
        int maxplayers = atoi(param + 12);
        *(DWORD*)(rdx + 96) = maxplayers; //MaxPlayers = maxplayers;
    }
    
    return ((dedicatedinit)origDedicatedServer)(rcx, rdx, r8);
}

__int64 __fastcall ConvarSetupHook(__int64 a1, __int64 a2)
{
    *(uint64_t*)(a2 + 40) &= ~3; //Flags &= ~3; // removes hidden and dev flags

    return ((cl_cvar)origConvarSetup)(a1, a2);
}

static const char* GetCommandLineParam(const char* commandline, const char* match) {
	const char* paramStart = strstr(commandline, match);
	if (!paramStart) return nullptr;
	// Is there something after this parameter that's not a space or null terminator?
	// If not, then this is part of a longer command
	const char lastChar = *(paramStart + strlen(match));
	if (!(lastChar == 0 || lastChar == ' ')) return nullptr;
	return paramStart;	
}
EXTERN_DLL_EXPORT void __fastcall Source2Main(HINSTANCE a, HINSTANCE b, LPWSTR c, long long d, const char* e, const char* f)
{
    auto lib2 = LoadLibraryExA("vstdlib.dll", NULL, 8);
    MH_Initialize();
    //0x9AF0
    MH_CreateHook((LPVOID)(((unsigned long long)lib2) + 0x9AB1 + strlen(message)), &ConvarSetupHook, &origConvarSetup);
    MH_EnableHook((LPVOID)(((unsigned long long)lib2) + 0x9AB1 + strlen(message)));

    auto lib = LoadLibraryExA("engine2.dll", NULL, 8);
    source2 init = (source2)GetProcAddress(lib, "Source2Main");

    char* line = GetCommandLineA();

    if (!GetCommandLineParam(line, "-dedicated"))
    {
        if (!GetCommandLineParam(line, "-vr"))
        {
            //0x103760
            MH_CreateHook((LPVOID)(((unsigned long long)lib) + 0x103721 + strlen(message)), &VRInitHook, &origVRInit);
            MH_EnableHook((LPVOID)(((unsigned long long)lib) + 0x103721 + strlen(message)));
        }
    }
    else
    {
        //0x636D0
        MH_CreateHook((LPVOID)(((unsigned long long)lib) + 0x63691 + strlen(message)), &DedicatedServerHook, &origDedicatedServer);
        MH_EnableHook((LPVOID)(((unsigned long long)lib) + 0x63691 + strlen(message)));
    }
    
    const char* modParam = strstr(line, "-game ");
    if (modParam)
    {
        modParam += 6;

		const char* strEnd = nullptr;
		if (*modParam == '\"') {
			modParam++;
			strEnd = strchr(modParam, '\"');
		}
		else {
			strEnd = strchr(modParam, ' ');
			if (!strEnd) strEnd = modParam + strlen(modParam) + 1;
		}

		if (strEnd) {
			char* modname = new char[(int)(strEnd - modParam) + 1];

			memcpy(modname, modParam, (int)(strEnd - modParam));
			modname[(int)(strEnd - modParam)] = '\0';

			init(a, b, c, d, e, modname);
			return;
		}  
    }
    init(a, b, c, d, e, f);

}