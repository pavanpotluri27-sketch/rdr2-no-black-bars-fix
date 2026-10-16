// NoBlackBarsFix - removes cutscene black bars ONLY while a cutscene is really running.
// Replacement for NoBlackBars.asi (2020), which removed bars every frame and made some
// missions hang before their cutscene (mission waits for the bars to appear).
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*fnScriptRegister)(HMODULE, void (*)(void));
typedef void (*fnScriptUnregister)(HMODULE);
typedef void (*fnNativeInit)(uint64_t);
typedef void (*fnNativePush64)(uint64_t);
typedef uint64_t *(*fnNativeCall)(void);
typedef void (*fnScriptWait)(DWORD);
typedef void (*KeyboardHandler)(DWORD, WORD, BYTE, BOOL, BOOL, BOOL, BOOL);
typedef void (*fnKbReg)(KeyboardHandler);

static fnScriptRegister pScriptRegister;
static fnScriptUnregister pScriptUnregister;
static fnNativeInit pNativeInit;
static fnNativePush64 pNativePush64;
static fnNativeCall pNativeCall;
static fnScriptWait pScriptWait;
static fnKbReg pKbRegister, pKbUnregister;

#define H_FORCE_LETTER_BOX_THIS_UPDATE 0xC64ABC0676AF262BULL // removes bars for this frame
#define H_IS_ANIM_SCENE_CAM_ACTIVE     0x20389408F0E93B9AULL // cutscene camera active
#define H_PLAYER_ID                    0x217E9DC48139933DULL
#define H_IS_PLAYER_CONTROL_ON         0x7964097FCE4C244BULL
#define H_HAS_LETTER_BOX               0x81DCFD13CF39920EULL // game wants bars (diagnostics)

static HMODULE g_self;
static char g_dir[MAX_PATH];
static int g_toggleVk = VK_F11;   // 0 = no toggle key
static DWORD g_delayMs = 750;      // wait after cutscene starts before removing bars
static int g_requireNoControl = 0; // also treat "player control off" as cutscene
static DWORD g_endGraceMs = 1000;  // both signals must be gone this long to end cutscene mode
static int g_log = 1;
static volatile LONG g_toggleReq = 0;

static void logf(const char *fmt, ...)
{
	if (!g_log) return;
	char path[MAX_PATH];
	snprintf(path, sizeof path, "%sNoBlackBarsFix.log", g_dir);
	FILE *f = fopen(path, "a");
	if (!f) return;
	SYSTEMTIME t; GetLocalTime(&t);
	fprintf(f, "[%02d:%02d:%02d] ", t.wHour, t.wMinute, t.wSecond);
	va_list a; va_start(a, fmt); vfprintf(f, fmt, a); va_end(a);
	fputc('\n', f);
	fclose(f);
}

static uint64_t call0(uint64_t h) { pNativeInit(h); return *pNativeCall(); }
static uint64_t call1(uint64_t h, uint64_t a) { pNativeInit(h); pNativePush64(a); return *pNativeCall(); }

static void onKey(DWORD key, WORD rep, BYTE sc, BOOL ext, BOOL alt, BOOL wasDown, BOOL isUp)
{
	(void)rep; (void)sc; (void)ext; (void)alt; (void)wasDown;
	if (g_toggleVk && key == (DWORD)g_toggleVk && isUp)
		InterlockedExchange(&g_toggleReq, 1);
}

static void scriptMain(void)
{
	int enabled = 1, inCutscene = 0, lastCam = -1, lastNoCtrl = -1, lastBox = -1, lastForcing = -1;
	ULONGLONG since = 0, quietSince = 0;
	logf("started: key=%d delay=%lums requireNoControl=%d endGrace=%lums", g_toggleVk, g_delayMs, g_requireNoControl, g_endGraceMs);
	for (;;) {
		if (InterlockedExchange(&g_toggleReq, 0)) {
			enabled = !enabled;
			logf("toggled: bar removal %s", enabled ? "ON" : "OFF");
		}
		int camActive = (int)call0(H_IS_ANIM_SCENE_CAM_ACTIVE) != 0;
		int noControl = (int)call1(H_IS_PLAYER_CONTROL_ON, call0(H_PLAYER_ID)) == 0;
		if (camActive != lastCam || noControl != lastNoCtrl) {
			logf("  animCam=%d noControl=%d", camActive, noControl);
			lastCam = camActive; lastNoCtrl = noControl;
		}

		// Start strictly (so missions can bring the bars in and enter the cutscene),
		// but stay in cutscene mode while EITHER signal holds: cutscenes switch camera
		// types mid-scene, which made the bars come back.
		ULONGLONG now = GetTickCount64();
		if (!inCutscene) {
			if (camActive && (noControl || !g_requireNoControl)) {
				inCutscene = 1; since = now; quietSince = 0;
				logf("cutscene start");
			}
		} else if (camActive || noControl) {
			quietSince = 0;
		} else {
			if (!quietSince) quietSince = now;
			if (now - quietSince >= g_endGraceMs) { inCutscene = 0; logf("cutscene end"); }
		}

		int forcing = enabled && inCutscene && now - since >= g_delayMs;
		if (forcing != lastForcing) { logf("  removing bars: %s", forcing ? "YES" : "no"); lastForcing = forcing; }
		if (!forcing) { // while forcing, the game's own flag is overridden, so only log it when idle
			int box = (int)call0(H_HAS_LETTER_BOX) != 0;
			if (box != lastBox) { logf("  game bars=%d", box); lastBox = box; }
		} else lastBox = -1;
		if (forcing)
			call0(H_FORCE_LETTER_BOX_THIS_UPDATE);
		pScriptWait(0);
	}
}

static void loadIni(void)
{
	char ini[MAX_PATH];
	snprintf(ini, sizeof ini, "%sNoBlackBarsFix.ini", g_dir);
	int fkey = GetPrivateProfileIntA("Settings", "ToggleKeyF", 11, ini); // 1..12 = F1..F12, 0 = off
	g_toggleVk = (fkey >= 1 && fkey <= 12) ? VK_F1 + fkey - 1 : 0;
	g_delayMs = GetPrivateProfileIntA("Settings", "StartDelayMs", 750, ini);
	g_requireNoControl = GetPrivateProfileIntA("Settings", "RequirePlayerControlOff", 0, ini);
	g_endGraceMs = GetPrivateProfileIntA("Settings", "EndGraceMs", 1000, ini);
	g_log = GetPrivateProfileIntA("Settings", "Log", 1, ini);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r)
{
	(void)r;
	if (reason == DLL_PROCESS_ATTACH) {
		g_self = h;
		GetModuleFileNameA(h, g_dir, MAX_PATH);
		char *s = strrchr(g_dir, '\\'); if (s) s[1] = 0;
		loadIni();
		HMODULE sh = LoadLibraryA("ScriptHookRDR2.dll");
		if (!sh) return TRUE;
		pScriptRegister   = (fnScriptRegister)GetProcAddress(sh, "?scriptRegister@@YAXPEAUHINSTANCE__@@P6AXXZ@Z");
		pScriptUnregister = (fnScriptUnregister)GetProcAddress(sh, "?scriptUnregister@@YAXPEAUHINSTANCE__@@@Z");
		pNativeInit       = (fnNativeInit)GetProcAddress(sh, "?nativeInit@@YAX_K@Z");
		pNativePush64     = (fnNativePush64)GetProcAddress(sh, "?nativePush64@@YAX_K@Z");
		pNativeCall       = (fnNativeCall)GetProcAddress(sh, "?nativeCall@@YAPEA_KXZ");
		pScriptWait       = (fnScriptWait)GetProcAddress(sh, "?scriptWait@@YAXK@Z");
		pKbRegister       = (fnKbReg)GetProcAddress(sh, "?keyboardHandlerRegister@@YAXP6AXKGEHHHH@Z@Z");
		pKbUnregister     = (fnKbReg)GetProcAddress(sh, "?keyboardHandlerUnregister@@YAXP6AXKGEHHHH@Z@Z");
		if (!pScriptRegister || !pNativeInit || !pNativePush64 || !pNativeCall || !pScriptWait) return TRUE;
		pScriptRegister(h, scriptMain);
		if (pKbRegister) pKbRegister(onKey);
	} else if (reason == DLL_PROCESS_DETACH) {
		if (pScriptUnregister) pScriptUnregister(h);
		if (pKbUnregister) pKbUnregister(onKey);
	}
	return TRUE;
}
