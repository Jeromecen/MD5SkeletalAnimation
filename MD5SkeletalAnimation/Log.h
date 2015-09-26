#ifndef __MD5Log_h__
#define __MD5Log_h__
#include <fstream>
using namespace std;
class CLog
{
public:
	CLog();
	~CLog();
	static CLog* Inst()
	{
		static CLog *pInstance = new CLog();
		return pInstance;
	}
	void Print(const char* pFormat, ...);
private:
	ofstream mLogFile;
};
#define g_Log CLog::Inst()
#endif