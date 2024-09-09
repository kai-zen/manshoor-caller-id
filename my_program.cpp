#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <cstdio>
#include "httplib.h"
// Successfully compiled with: g++ -static -std=c++11 -o my_program my_program.cpp -lws2_32 -lpthread

typedef int (*GetNewAnswer)(char *);
typedef void (*SendLastCommandToNetwork)(const char *);
typedef void (*GetCallerID)(int *, int *, char *);
typedef void (*GetAnswer)(int *, int *);
typedef void (*GetHockOn)(int *, int *, short *);
typedef void (*GetMissedCall)(int *, int *, short *);
typedef void (*GetHockOff)(int *, int *);
typedef void (*GetDialNumber2)(int *, int *, short *);
typedef void (*GetRing)(int *, int *, short *);
typedef void (*GetLineDisconnect)(int *, int *);
typedef void (*GetDeviceConnect)(int *, short *, short *);
typedef void (*GetDeviceDisConnect)(int *);

std::string latestData = "No data yet.";

void handleGetRequest(const httplib::Request &req, httplib::Response &theResponse)
{
    theResponse.set_content(latestData, "text/html");
    theResponse.set_header("Access-Control-Allow-Origin", "*");
    theResponse.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    theResponse.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    theResponse.status = 200;
}

void runServer()
{
    httplib::Server svr;
    if (!svr.is_valid())
        std::cout << "server has an error...\n";
    svr.Get("/", handleGetRequest);
    svr.listen("0.0.0.0", 7070);
}

int main()
{
    std::string dllPath = "DllTechnoCaller.dll";
    SetDllDirectory(dllPath.c_str());

    static HINSTANCE hGetProcIDDLL = LoadLibrary(dllPath.c_str());
    if (!hGetProcIDDLL)
    {
        std::cout << "Could not load the DLL" << std::endl;
        return EXIT_FAILURE;
    }

    GetNewAnswer getNewAnswer = (GetNewAnswer)GetProcAddress(hGetProcIDDLL, "GetNewAnswer");
    SendLastCommandToNetwork sendLastCommandToNetwork = (SendLastCommandToNetwork)GetProcAddress(hGetProcIDDLL, "SendLastCommandToNetwork");
    GetCallerID getCallerID = (GetCallerID)GetProcAddress(hGetProcIDDLL, "GetCallerID");
    GetAnswer getAnswer = (GetAnswer)GetProcAddress(hGetProcIDDLL, "GetAnswer");
    GetHockOn getHockOn = (GetHockOn)GetProcAddress(hGetProcIDDLL, "GetHockOn");
    GetMissedCall getMissedCall = (GetMissedCall)GetProcAddress(hGetProcIDDLL, "GetMissedCall");
    GetHockOff getHockOff = (GetHockOff)GetProcAddress(hGetProcIDDLL, "GetHockOff");
    GetDialNumber2 getDialNumber2 = (GetDialNumber2)GetProcAddress(hGetProcIDDLL, "GetDialNumber2");
    GetRing getRing = (GetRing)GetProcAddress(hGetProcIDDLL, "GetRing");
    GetLineDisconnect getLineDisconnect = (GetLineDisconnect)GetProcAddress(hGetProcIDDLL, "GetLineDisconnect");
    GetDeviceConnect getDeviceConnect = (GetDeviceConnect)GetProcAddress(hGetProcIDDLL, "GetDeviceC42onnect");
    GetDeviceDisConnect getDeviceDisConnect = (GetDeviceDisConnect)GetProcAddress(hGetProcIDDLL, "GetDeviceDisConnect");

    if (!getNewAnswer || !sendLastCommandToNetwork || !getCallerID)
    {
        std::cout << "Could not locate the functions" << std::endl;
        return EXIT_FAILURE;
    }

    std::thread serverThread(runServer);

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    while (true)
    {
        char data[300] = {0};
        int res = getNewAnswer(data);
        if (res != 0)
        {
            int deviceIndex = 1;
            int lineIndex = 1;
            std::string logMessage;
            if (res == 2)
            {
                char callerIdNumber[300] = {0};
                getCallerID(&deviceIndex, &lineIndex, callerIdNumber);
                std::string phoneNumber(callerIdNumber);
                phoneNumber.erase(std::remove(phoneNumber.begin(), phoneNumber.end(), '\0'), phoneNumber.end());
                logMessage = "-Device:" + std::to_string(deviceIndex) + "-Line:" + std::to_string(lineIndex) + "-PhoneNumber:" + phoneNumber + "-Res:2";
                latestData = logMessage;
            }
            else if (res == 8)
            {
                short ring = -1;
                getMissedCall(&deviceIndex, &lineIndex, &ring);
                char callerIdNumber[300] = {0};
                getCallerID(&deviceIndex, &lineIndex, callerIdNumber);

                logMessage = "-Device:" + std::to_string(deviceIndex) + "-Line:" + std::to_string(lineIndex) + "-Rang:" + std::to_string(ring) + "-Res:8"; // missed
                latestData = logMessage;
            }
            std::cout << logMessage << std::endl;
        }
        else
            std::cout << "No new data" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    FreeLibrary(hGetProcIDDLL);
    return EXIT_SUCCESS;
}
