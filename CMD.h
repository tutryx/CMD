#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

/*
	Communicate with a CMD process.
*/
class CMD
{
public:
	CMD();
	std::string Exec(std::string command);
	~CMD();

private:
	void WriteToPipe(std::string command);
	std::string ReadFromPipe();

	HANDLE m_hChildStd_IN_Rd = 0;
	HANDLE m_hChildStd_IN_Wr = 0;
	HANDLE m_hChildStd_OUT_Rd = 0;
	HANDLE m_hChildStd_OUT_Wr = 0;
};

