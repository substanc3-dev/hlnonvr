// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Minhook.h>
#include <thread>
#include <cstring>

// To update the game for future hexadecimal changes, use Ghidra and offset memory from an older version of the game.
// Example being the header of a dll file, 0x180000000. Finding where memory maps used to be and then rerouting them
// to new places seems to be the only way to update this file. This is currently patched for 5/9/2021.

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)
// This value is not needed anymore due to the new memory address for ConvarSetupHook's MH hook not requiring such.
//const char* message = "Created by Jan4V, half-life:alyx only, not other source 2 games";

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
    //TODO: Fix maxmaxplayers crash. Doing anything with this will cause a server crash upon join.
    //*(DWORD*)(rdx + 100) = 33; //MaxMaxPlayers = 33;
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

//TODO: Add back server DLL patches, requires patch threading fix.
// This is not being used due to the above.
void ServerDLLPatches()
{
    HMODULE server = 0;
    do {
        server = GetModuleHandleA("server.dll");
    } while (!server);

    //TODO: Fix older versions

    auto curr = GetCurrentProcess();

    const char patch1[7] = { 0xB9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90 };
    // 0x778ABB, 0x773B27, 0x773E67
    // The last version specifies these memory addresses as: 0x77911B, 0x774187, 0x7744C7
    // The current version specifies these memory addresses as: 0x74FC9b, 0x74AD37, 0x74B077
    WriteProcessMemory(curr, (void*)(((unsigned long long)server) + 0x74FC9b), patch1, 7, NULL);
    WriteProcessMemory(curr, (void*)(((unsigned long long)server) + 0x74AD37), patch1, 7, NULL);
    WriteProcessMemory(curr, (void*)(((unsigned long long)server) + 0x74B077), patch1, 7, NULL);

    const char patch2[3] = { 0x78, 0xC6, 0x2F };
    // 0x778AB4
    // The last version specifies this memory address as: 0x779114
    // The current version specifies this memory address as: 0x74FC94
    WriteProcessMemory(curr, (void*)(((unsigned long long)server) + 0x74FC94), patch2, 3, NULL);
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
    // The last version specifies this memory address as: 0x9AB1 + strlen(message)
    // The current version specifies this memory address as: 0x9B60
    // Calculating in hexadecimal confirms that the sum of the old memory address and strlen(message) is the first value above.
    // For reference, strlen(message) is 63 in decimal.
    MH_CreateHook((LPVOID)(((unsigned long long)lib2) + 0x9B60), &ConvarSetupHook, &origConvarSetup);
    MH_EnableHook((LPVOID)(((unsigned long long)lib2) + 0x9B60));

    auto lib = LoadLibraryExA("engine2.dll", NULL, 8);
    source2 init = (source2)GetProcAddress(lib, "Source2Main");

    char* line = GetCommandLineA();

    if (!GetCommandLineParam(line, "-dedicated"))
    {
        if (!GetCommandLineParam(line, "-vr"))
        {
            //0x103760
            //TODO: Fix older versions
            // The last version specifies this memory address as: 0x103650
            // The current version specifies this memory address as: 0x1014E0
            MH_CreateHook((LPVOID)(((unsigned long long)lib) + 0x1014E0), &VRInitHook, &origVRInit);
            MH_EnableHook((LPVOID)(((unsigned long long)lib) + 0x1014E0));
        }
    }
    else
    {
        //0x636D0
        //TODO: Fix older versions
        // The last version specifies this memory address as: 0x636F0
        // The current version specifies this memory address as: 0x63AF0
        MH_CreateHook((LPVOID)(((unsigned long long)lib) + 0x63AF0), &DedicatedServerHook, &origDedicatedServer);
        MH_EnableHook((LPVOID)(((unsigned long long)lib) + 0x63AF0));

        //TODO: Fix patch threading, this causes a server crash upon join.  
        //std::thread t(&ServerDLLPatches);
        //t.detach();
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