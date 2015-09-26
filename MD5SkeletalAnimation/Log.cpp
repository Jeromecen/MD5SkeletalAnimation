#include "Log.h"
#include "windows.h"

CLog::CLog()
{
	mLogFile.open("log.txt", ios::out | ios::trunc);
}

CLog::~CLog()
{

}

void CLog::Print(const char* pFormat, ...)
{
	char szBuffer[256];
	va_list args;
	va_start(args, pFormat);
	vsprintf(szBuffer, pFormat, args);
	mLogFile << szBuffer << endl;
}