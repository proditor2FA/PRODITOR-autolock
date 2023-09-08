#pragma once
#include "framework.h"
#include "resource.h"
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <cstdio>
#include <Shellapi.h>

std::string logPath;//Location of log file in the user system
std::string exePath;//Location of EXE file in the user system
std::string errorLog;//File name of error log TODO
//std::string logText;

HHOOK callbackhook;//Callback Hook

// Timer ID
#define TIMER_ID 1

HWND hwnd;
HICON hIcon;
#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATA nid;


#define PRODITOR_ACTIVATION 4
#define PRODITOR_TIMEOUT 70

int prodActivationCounter = PRODITOR_ACTIVATION;
int prodTimer = PRODITOR_TIMEOUT;
bool prodActive = false;

void enableOnAutoBoot(bool flag);
void createDirectoryAndFiles();
void setLogStorePath();
void logTimeEntry(std::string logText);
void startTracker(HINSTANCE hInstance);
void releaseHookRoutine();
void CleanupFunction();


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddTrayIcon(HWND hwnd, HINSTANCE hInstance);
void RemoveTrayIcon(HWND hwnd);


// Timer callback function
VOID CALLBACK TimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	if(prodActive) {
		//logTimeEntry("Timer event");
		if (prodTimer > 0) {
			prodTimer--;
			
			std::wstring tooltipText = L"PRODITOR autolock\nstatus: [autolock:" + std::to_wstring(prodTimer) + L"]";
			// Copy the tooltip text to nid.szTip
			wcsncpy_s(nid.szTip, tooltipText.c_str(), _TRUNCATE);
			//_tcscpy_s(nid.szTip, _T("Proditor: [autolock]"));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
			
			if (prodTimer == 0) {
			logTimeEntry("PRODITOR is missing, lock windows screen!");
			_tcscpy_s(nid.szTip, _T("PRODITOR autolock\nstatus: [missing]"));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
			prodActive = false;
			prodActivationCounter = PRODITOR_ACTIVATION;
			LockWorkStation();
			}
		}
	}
}

LRESULT CALLBACK hookCallbackProc(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT keyboardHook = *((KBDLLHOOKSTRUCT*)lParam);
	DWORD vkCode = keyboardHook.vkCode;
	std::string y = std::to_string(vkCode);
	if (vkCode == 0x91) { //roll_key 145
		if (prodActivationCounter == 1) {
			prodActive = true;
			//logTimeEntry("PRODITOR autolock enabled!");
			prodTimer = PRODITOR_TIMEOUT;
		} else {
			//logTimeEntry("PRODITOR stick is present!");
			_tcscpy_s(nid.szTip, _T("PRODITOR autolock\nstatus: [present]"));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
			if (prodActivationCounter > 1) {
				prodActivationCounter--;
			}
		}
	
	}
	return CallNextHookEx(callbackhook, nCode, wParam, lParam);
}


// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI setHookRoutine(HINSTANCE hInstance) {
	HINSTANCE hinst = GetModuleHandle(NULL);
	if (hinst != NULL) {
		callbackhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)hookCallbackProc, hinst, 0);
		
		UINT_PTR timerId = SetTimer(NULL, TIMER_ID, 1000, TimerCallback);
		if (timerId == 0) {
			// Failed to create the timer
			DWORD errorCode = GetLastError();
			// Handle the error accordingly
		}
		
		AddTrayIcon(hwnd, hInstance);
		
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) != 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		releaseHookRoutine();
		RemoveTrayIcon(hwnd);
		KillTimer(NULL, TIMER_ID);
	}
	return 0;
}

void AddTrayIcon(HWND hwnd, HINSTANCE hInstance)
{
	//NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	// Load the application's icon
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
	_tcscpy_s(nid.szTip, _T("PRODITOR autolock\nstatus: [ready]"));
	
	Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;

	Shell_NotifyIcon(NIM_DELETE, &nid);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow) {
	
	// Create a message-only window
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WindowProc, 0, 0, hInstance, NULL, NULL, NULL, NULL, _T("WindowClass"), NULL };

	RegisterClassEx(&wc);
	hwnd = CreateWindowEx(0, _T("WindowClass"), _T(""), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

//	setLogStorePath();
//	createDirectoryAndFiles();
	enableOnAutoBoot(true);//Disable Auto Run after Boot by setting false
	logTimeEntry("PRODITOR autolock running");
	startTracker(hInstance);
	logTimeEntry("PRODITOR autolock stop");
	//RemoveTrayIcon(hwnd);

	return 0;
}

void setLogStorePath() {
	char lpBuffer[MAX_PATH];//lptstr buffer
	GetEnvironmentVariableA("APPDATA", lpBuffer, MAX_PATH);
	logPath = lpBuffer;
	logPath += "\\Proditor\\";
	GetModuleFileNameA(NULL, lpBuffer, MAX_PATH);
	exePath = lpBuffer;
}

void createDirectoryAndFiles() {
	std::string tempLocPath = logPath;
	std::string execFile = tempLocPath;
	execFile += "\\Proditor.exe";
	char lpDir[MAX_PATH];
	GetModuleFileNameA(GetModuleHandle(0), lpDir, MAX_PATH);
	std::string b = lpDir;
	CreateDirectoryA(tempLocPath.c_str(), NULL);
}

void enableOnAutoBoot(bool flag) {
	if (!flag) {
		return;
	}
	char lpData[MAX_PATH];
	HKEY hkeyVar;
	std::string processPath = exePath;
	logTimeEntry("PRODITOR start: " + exePath);
	strcpy(lpData, processPath.c_str());
	long a = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkeyVar);
	if (a != ERROR_SUCCESS) {
		//TODO Error log
	}
	long b = RegSetValueExA(hkeyVar, "Proditor", 0, REG_SZ, (const unsigned char*)lpData, MAX_PATH);
	if (b != ERROR_SUCCESS) {
		//TODO Error log
	}
	RegCloseKey(hkeyVar);
}

void logTimeEntry(std::string logText) {
/*
	SYSTEMTIME systime;

	GetLocalTime(&systime);
	int second = systime.wSecond;
	int minute = systime.wMinute;
	int hour = systime.wHour;
	int date = systime.wDay;
	int month = systime.wMonth;
	int year = systime.wYear;
	std::string timeStamp = "--> ";
	timeStamp += std::to_string(date) + "-";
	timeStamp += std::to_string(month) + "-";
	timeStamp += std::to_string(year) + " ";
	timeStamp += std::to_string(hour) + ":";
	timeStamp += std::to_string(minute) + ":";
	timeStamp += std::to_string(second) + " [" + logText + "]\n";

	char lpDate[MAX_PATH];
	strcpy(lpDate, timeStamp.c_str());
	std::string tempLocPath = logPath;
	char lpDir[MAX_PATH];
	strcpy(lpDir, tempLocPath.c_str());
	strcat(lpDir, "Proditor.log");
	FILE* tempFile;
	tempFile = fopen(lpDir, "a+");
	if (tempFile != NULL) {
		//TODO
	}
	fputs(lpDate, tempFile);
	fclose(tempFile);
*/
}

void startTracker(HINSTANCE hInstance) {
	HANDLE lpThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)setHookRoutine(hInstance), NULL, 0, NULL);
	if (lpThread) {
		WaitForSingleObject(lpThread, INFINITE);
	}
	else {
		//Write Error log TODO
	}
}

void releaseHookRoutine() {
	UnhookWindowsHookEx(callbackhook);
}
