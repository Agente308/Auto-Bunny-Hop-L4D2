#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <thread>
#include <iostream>
#include <Psapi.h>
#include "minhook/include/MinHook.h"
#include <chrono>
#include <atomic>

constexpr uintptr_t PLAYER_BASE_OFFSET = 0x726BD8;
constexpr uintptr_t MFLAGS_OFFSET = 0xF0;

enum PlayerFlags
{
    FL_ONGROUND = (1 << 0),
    FL_DUCKING = (1 << 1),
    FL_WATERJUMP = (1 << 2),
    FL_ONTRAIN = (1 << 3),
    FL_INRAIN = (1 << 4),
    FL_FROZEN = (1 << 5),
    FL_ATCONTROLS = (1 << 6),
    FL_CLIENT = (1 << 7),
    FL_FAKECLIENT = (1 << 8)
};

static uintptr_t g_ModuleBase = 0;
static std::atomic<bool> g_BhopEnabled(true);
static std::atomic<bool> g_Running(true);

inline uintptr_t GetLocalPlayer()
{
    if (!g_ModuleBase) return 0;

    __try
    {
        uintptr_t playerBase = g_ModuleBase + PLAYER_BASE_OFFSET;
        uintptr_t localPlayer = *(uintptr_t*)playerBase;

        if (localPlayer < 0x10000 || localPlayer > 0x7FFFFFFF)
            return 0;

        return localPlayer;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

inline int GetPlayerFlags(uintptr_t player)
{
    if (!player) return -1;

    __try
    {
        return *(int*)(player + MFLAGS_OFFSET);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

inline bool IsOnGround(int flags)
{
    return flags != -1 && (flags & FL_ONGROUND) != 0;
}

inline void SendJump()
{
    keybd_event(VK_SPACE, 0x39, 0, 0);                   
    keybd_event(VK_SPACE, 0x39, KEYEVENTF_KEYUP, 0);    
}

void UltraFastBhop()
{
    static bool wasOnGround = false;

    if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000))
    {
        wasOnGround = false;
        return;
    }

    uintptr_t localPlayer = GetLocalPlayer();
    if (!localPlayer) return;

    int flags = GetPlayerFlags(localPlayer);
    if (flags == -1) return;

    bool isOnGround = IsOnGround(flags);

    if (isOnGround && !wasOnGround)
    {
        SendJump();
    }

    wasOnGround = isOnGround;
}

DWORD WINAPI BhopThread(LPVOID)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    std::cout << "[INFO] Hilo iniciado\n";

    bool ctrlWasPressed = false;

    while (g_Running)
    {
        if (!g_BhopEnabled)
        {
            Sleep(50);
            continue;
        }

        bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

        if (ctrlPressed)
        {
            UltraFastBhop();

            std::this_thread::yield();
        }
        else
        {
            Sleep(5);
        }

        ctrlWasPressed = ctrlPressed;
    }

    std::cout << "[INFO] Hilo bhop finalizado\n";
    return 0;
}

DWORD WINAPI MonitorThread(LPVOID)
{
    while (g_Running)
    {
        Sleep(3000);
    }
    return 0;
}

DWORD WINAPI MainThread(LPVOID hModule)
{
    timeBeginPeriod(1);

    AllocConsole();

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << u8R"(⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣠⣦⣤⣴⣤⣤⣄⣀⣀⣀⣀⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣤⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⡀⠀⠀⣀⣀⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣛⣛⣻⣿⣦⣀⠀⢀⣀⣀⣏⣹⠀
⢠⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠭⠭⠽⠽⠿⠿⠭⠭⠭⠽⠿⠿⠛
⠈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠛⠉⢻⣿⣿⣿⡟⠏⠉⠉⣿⢿⣿⣿⣿⣇⠀⠀⠀⠀⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⣿⣿⣿⣿⣿⣿⣿⡿⠿⠛⠉⠁⠀⠀⠀⢠⣿⣿⣿⠋⠑⠒⠒⠚⠙⠸⣿⣿⣿⣿⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⣿⣿⡿⠿⠛⠉⠁⠀⠀⠀⠀⠀⠀⠀⣰⣿⣿⡿⠃⠀⠀⠀⠀⠀⠀⠀⢻⣿⣿⣿⣿⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⠛⠛⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣿⣿⣿⣿⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⢿⣿⡿⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀)" << std::endl;

    std::cout << "========================================\n";
    std::cout << "     Auto Bunny Hop - Desarrollado por el Agente 308\n";
    std::cout << "        https://discord.gg/mnbuMWgZN9\n";
    std::cout << "========================================\n\n";

    Sleep(1000);

    g_ModuleBase = (uintptr_t)GetModuleHandleA("client.dll");
    if (!g_ModuleBase)
    {
        std::cout << "[ERROR] client.dll no encontrado\n";
        Sleep(3000);
        FreeLibraryAndExitThread((HMODULE)hModule, 0);
        return 0;
    }

    std::cout << "[OK] client.dll: valido\n";

    uintptr_t testPlayer = GetLocalPlayer();
    if (testPlayer)
    {
        std::cout << "[OK] Jugador detectado: valido\n";
        std::cout << "[OK] Flags: valido\n\n";
    }
    else
    {
        std::cout << "[INFO] Entra al juego para activar el bunny hop\n\n";
    }

    HANDLE hBhopThread = CreateThread(nullptr, 0, BhopThread, nullptr, 0, nullptr);
    HANDLE hMonitorThread = CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);

    if (hBhopThread)
        SetThreadPriority(hBhopThread, THREAD_PRIORITY_TIME_CRITICAL);

    std::cout << "========================================\n";
    std::cout << "CONTROLES:\n";
    std::cout << "  CTRL   - Mantener para bunny hop automatico\n";
    std::cout << "  INSERT - ON/OFF bunyy hop\n";
    std::cout << "  F11    - Salir\n";
    std::cout << "========================================\n\n";

    std::cout << "[STATUS] Bunyy Hop: " << (g_BhopEnabled ? "ACTIVADO" : "DESACTIVADO") << "\n";
    std::cout << "[LISTO] Manten CTRL presionado\n\n";

    while (true)
    {
        if (GetAsyncKeyState(VK_F11) & 1)
            break;

        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            g_BhopEnabled = !g_BhopEnabled;
            std::cout << "\n[STATUS] Bunyy Hop: " << (g_BhopEnabled ? "ACTIVADO" : "DESACTIVADO") << "\n\n";
            Sleep(200);
        }

        Sleep(50);
    }

    std::cout << "\n[INFO] Cerrando bhop...\n";
    g_Running = false;

    if (hBhopThread)
    {
        WaitForSingleObject(hBhopThread, 3000);
        CloseHandle(hBhopThread);
    }

    if (hMonitorThread)
    {
        WaitForSingleObject(hMonitorThread, 1000);
        CloseHandle(hMonitorThread);
    }

    std::cout << "[OK] Cerrado correctamente\n";
    Sleep(1000);

    timeEndPeriod(1);
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        g_Running = false;
    }
    return TRUE;
}
