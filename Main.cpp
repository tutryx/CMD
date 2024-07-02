#include "CMD.h"
#include <iostream>

int main()
{
    CMD* test;

    try
    {
        test = new CMD;
    }
    catch (const std::exception& t)
    {
        std::cout << t.what() << " has failed" << std::endl;
        return -1;
    }

    std::cout << test->Exec("dir");
    std::cout << test->Exec("cd ..");
    std::cout << test->Exec("dir");

    delete test;
}