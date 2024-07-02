#include "CMD.h"
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <algorithm>
#include <stdexcept>

constexpr auto BUFSIZE = 4096 ;

CMD::CMD()
{
    //        PARENT       |       CHILD
    // hStdInPipeWrite    ->   hStdInPipeRead
    // hStdOutPipeRead    <-   hStdOutPipeWrite

    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited. 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0))
        throw std::runtime_error("Stdout CreatePipe");

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
        throw std::runtime_error("Stdout SetHandleInformation");

    // Create a pipe for the child process's STDIN. 
    if (!CreatePipe(&m_hChildStd_IN_Rd, &m_hChildStd_IN_Wr, &saAttr, 0))
        throw std::runtime_error("Stdin CreatePipe");

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if (!SetHandleInformation(m_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
        throw std::runtime_error("Stdin SetHandleInformation");

    TCHAR szCmdline[] = TEXT("cmd.exe");
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = m_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = m_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = m_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process. 
    bSuccess = CreateProcess(NULL,
        szCmdline,     // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION 

    // If an error occurs, exit the application. 
    if (!bSuccess) 
        throw std::runtime_error("CreateProcess");
    else
    {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example. 
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        // Close handles to the stdin and stdout pipes no longer needed by the child process.
        // If they are not explicitly closed, there is no way to recognize that the child process has ended.
        CloseHandle(m_hChildStd_OUT_Wr);
        CloseHandle(m_hChildStd_IN_Rd);
    }
}

std::string CMD::Exec(std::string command)
{
    WriteToPipe(command);
    // Send sentinel
    WriteToPipe("echo COMMAND_ENDED");

    return ReadFromPipe();
}

// Writes the command to the pipe for the child's STDIN.
// Stop when there is no more data. 
void CMD::WriteToPipe(std::string command)
{
    DWORD dwRead = 0, dwWritten = 0;
    CHAR chBuf[BUFSIZE];
    command.append("\n");
    WriteFile(m_hChildStd_IN_Wr, command.c_str(), command.size(), &dwWritten, NULL);
}

// Read output from the child process's pipe for STDOUT
// and return it. 
// Stop when there is no more data. 
std::string CMD::ReadFromPipe()
{
    DWORD dwRead = 0, dwWritten = 0, dwAvail = 0;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    std::string output;

    // Sentinel used to check if the command has been processed
    const std::string sentinel = "echo COMMAND_ENDED";

    for (;;)
    {
        // Check if there's any data available to read
        bSuccess = PeekNamedPipe(m_hChildStd_OUT_Rd, NULL, 0, NULL, &dwAvail, NULL);
        if (!bSuccess) break; // Error ocurred

        if (dwAvail > 0)
        {
            // Read
            bSuccess = ReadFile(m_hChildStd_OUT_Rd, chBuf, std::min((int)dwAvail, BUFSIZE), &dwRead, NULL);
            if (!bSuccess || dwRead == 0) break;

            output.append(chBuf, dwRead);

            // Check if the sentinel value is in the output
            if (output.find(sentinel) != std::string::npos)
            {
                // Remove the sentinel value from the output
                output = output.substr(0, output.find(sentinel));
                break;
            }

        }
        // No data available, wait and re-check
        else Sleep(100);
    }

    return output;
}

CMD::~CMD()
{
    WriteToPipe("exit");
    CloseHandle(m_hChildStd_IN_Wr);
    CloseHandle(m_hChildStd_OUT_Rd);
}