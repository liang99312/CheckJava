// CheckJava.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "CheckJava.h"
#include <stdio.h>
#include <string>
#include <Commctrl.h>
#pragma comment(lib, "comctl32") //InitCommonControls
#define MAX_LOADSTRING 100
using namespace std;
// 全局变量: 
HANDLE pbHandle;
HWND hwndPB, hMainWnd;
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


int cmdLine() {
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		//printf("Error On CreatePipe()\n");
		return -1;
	}
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfoA(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	if (!CreateProcessA(NULL, "c:\\windows\\system32\\cmd.exe /c java", NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		//printf("Error on CreateProcess()\n");
		return -1;
	}
	CloseHandle(hWrite);

	char databuf[65536] = { 0 };
	char *p = databuf;
	char buffer[4096] = { 0 };
	DWORD bytesRead;
	while (true) {
		if (ReadFile(hRead, buffer, 4095, &bytesRead, NULL) == NULL)
			break;
		memcpy(p, buffer, bytesRead);
		p += bytesRead;
		memset(buffer, 0, 4096);   //  增加该句 
		Sleep(100);
	}
	std::string s(databuf);
	std::string dst = "'java'";
	if (strncmp(databuf, dst.c_str(), dst.length()) == 0) {
		return -1;
	}
	return 0;
}

int substr(char cstr[], char csub[])
{
	string str(cstr), sub(csub);
	int nRet = 0, nStart = 0;
	while (-1 != (nStart = str.find(sub, nStart)))
	{
		nStart += sub.length();
		++nRet;
	}
	return nRet;
}

int setJavaPath(std::string path) {
	const char *szValue = path.c_str();//要设置的值
	char *strKey = "System\\CurrentControlSet\\Control\\Session Manager\\Environment";
	char *szName = "JAVA_HOME";//环境变量名
	HKEY hResult;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, strKey, 0, KEY_WRITE, &hResult) != ERROR_SUCCESS)
	{
		if (RegCreateKeyA(HKEY_LOCAL_MACHINE, szName, &hResult) != ERROR_SUCCESS) {

		}
	}

	if (RegSetValueExA(hResult, szName, NULL, REG_SZ,
		(const unsigned char *)szValue, strlen(szValue)) != ERROR_SUCCESS)
	{
		return -1;
	}
	szName = "PATH";//环境变量名
	const char* python_home = getenv("PATH");
	string p(python_home);
	p = p + "%JAVA_HOME%\\bin;%JAVA_HOME%\\lib\\tools.jar;";
	if (RegSetValueExA(hResult, szName, NULL, REG_SZ,
		(const unsigned char *)p.c_str(), strlen(p.c_str())) != ERROR_SUCCESS)
	{
		return -1;
	}
	DWORD dwRet = 0;
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, (LPARAM)"Environment", SMTO_NOTIMEOUTIFNOTHUNG, 1000, &dwRet);
	return 0;
}

int copydir(char* src, char* dst)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	char tmpsrc[256];
	strcpy(tmpsrc, src);
	strcat(tmpsrc, "\\*.*");
	hFind = FindFirstFileA(tmpsrc, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return -1;
	CreateDirectoryA(dst, 0);
	do
	{
		char newdst[256];
		strcpy(newdst, dst);
		if (newdst[strlen(newdst)] != '\\')
			strcat(newdst, "\\");
		strcat(newdst, FindFileData.cFileName);

		char newsrc[256];
		strcpy(newsrc, src);
		if (newsrc[strlen(newsrc)] != '\\')
			strcat(newsrc, "\\");
		strcat(newsrc, FindFileData.cFileName);
		if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(FindFileData.cFileName, ".") != 0 && strcmp(FindFileData.cFileName, "..") != 0)
			{
				copydir(newsrc, newdst);
			}
		}
		else
		{
			CopyFileA(newsrc, newdst, false);
		}
	} while (FindNextFileA(hFind, &FindFileData));
	FindClose(hFind);
	return 0;
}

int setJava()
{
	int r = cmdLine();
	if (r < 0) {
		SetWindowText(hMainWnd,L"启动HR-环境配置");
		char exeFullPath[256];
		GetCurrentDirectoryA(256, exeFullPath);
		string p(exeFullPath);
		p = p + "\\java";
		char *tmp = new char[p.length() + 1];
		strcpy(tmp, p.c_str());
		r = copydir(tmp, "C:\\java");
		if (r < 0) {
			MessageBoxA(NULL, "复制java文件失败", "提示", 0);
			return -1;
		}
		r = setJavaPath("C:\\java");
		if (r < 0) {
			MessageBoxA(NULL, "配置java环境变量失败", "提示", 0);
			return -1;
		}
		WinExec("cmd.exe /C C:\\java\\bin\\java -jar startHR.jar", SW_HIDE);
		return 0;
	}
	//system("java -jar startHR.jar");
	WinExec("cmd.exe /C java -jar startHR.jar", SW_HIDE);
	return 0;
}

DWORD WINAPI SetEnvProc(LPVOID lpParameter) {
	setJava();
	CloseHandle(pbHandle);
	::SendMessage(hMainWnd, WM_CLOSE, NULL, NULL);
	return 0;
}

DWORD WINAPI PBThreadProc(LPVOID lpParameter)
{
	HWND hwndPB = (HWND)lpParameter;    //进度条的窗口句柄  
	PBRANGE range;                        //进度条的范围  

	SendMessage(hwndPB, PBM_SETRANGE,    //设置进度条的范围  
		(WPARAM)0, (LPARAM)(MAKELPARAM(0, 100)));

	SendMessage(hwndPB, PBM_GETRANGE,    //获取进度条的范围  
		(WPARAM)TRUE,                    //TRUE 表示返回值为范围的最小值,FALSE表示返回最大值  
		(LPARAM)&range);
	while (TRUE)
	{
		SendMessage(hwndPB, PBM_DELTAPOS, //设置进度条的新位置为当前位置加上范围的1/20  
			(WPARAM)((range.iHigh - range.iLow) / 100), (LPARAM)0);
		if (SendMessage(hwndPB, PBM_GETPOS, (WPARAM)0, (LPARAM)0) //取得进度条当前位置  
			== range.iHigh)
		{
			SendMessage(hwndPB, PBM_SETPOS, (WPARAM)range.iLow, (LPARAM)0); //将进度条复位  
		}
		Sleep(20);
	}
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CHECKJAVA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
	pbHandle = CreateThread(            //操作进度条的线程
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SetEnvProc,
		NULL,
		0,
		0
	);
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHECKJAVA));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHECKJAVA));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中
   int cx = GetSystemMetrics(SM_CXFULLSCREEN) / 2 - 150;
   int cy = GetSystemMetrics(SM_CYFULLSCREEN) / 2 - 29;
   hMainWnd = CreateWindowW(szWindowClass, szTitle, NULL,
	   cx, cy, 300, 58, NULL, NULL, hInstance, NULL);
   //hMainWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hMainWnd)
   {
      return FALSE;
   }

   ShowWindow(hMainWnd, nCmdShow);
   UpdateWindow(hMainWnd);
   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{
		InitCommonControls(); //确保已注册了进度条类PROGRESS_CLASS
		hwndPB = CreateWindowEx(
			0,
			PROGRESS_CLASS,
			NULL,
			WS_CHILD | WS_VISIBLE,
			0, 0, 300, 20,            //位置和大小在WM_SIZE中设置
			hWnd,
			(HMENU)0,
			((LPCREATESTRUCT)lParam)->hInstance,
			NULL);
		CreateThread(            //操作进度条的线程
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)PBThreadProc,
			hwndPB,
			0,
			0
		);
		return 0;
	}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
            case 0:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
