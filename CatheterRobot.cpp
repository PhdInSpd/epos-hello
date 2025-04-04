//============================================================================
// Name        : HelloEposCmd.cpp
// Author      : Dawid Sienkiewicz
// Version     :
// Copyright   : maxon motor ag 2014-2021
// Description : Hello Epos in C++
//============================================================================
#include <iostream>
#include <limits>
#include <chrono>
#include <thread> // For sleep_for
#include <string.h>
#include <sstream>

//

#ifdef __linux__
// call when linux
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#elif _WIN32 || _WIN64
#include <conio.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <math.h>
#include <sys/types.h>
// #include <unistd.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <map>

// #include <sys/times.h>
// #include <sys/time.h>
#include <functional>

#include "xplatform.h"
#ifdef __linux__
#include "linux/Definitions.h"
#elif _WIN32 || _WIN64
#include "win32/Definitions.h"
#undef max // Undefine the max macro
#endif

// #include <unistd.h>
#include "getopt.h"
#include "Scaling.h"
#include "JodoCanopenMotion.h"
#include "JodoApplication.h"
#include "testgamecontroller.h"
#include "PLC.h"
#include "TeachData.h"
#include "RecipeRead.h"
using namespace std;

enum EAppMode
{
    AM_UNKNOWN,
    AM_DEMO,
    AM_INTERFACE_LIST,
    AM_PROTOCOL_LIST,
    AM_VERSION_INFO
};

void *g_pKeyHandle = 0;
unsigned short g_usNodeId;
std::string g_deviceName;
std::string g_protocolStackName;
std::string g_interfaceName;
std::string g_portName;
int g_baudrate = 0;

// subdevice
HANDLE subkeyHandle = 0;
std::string subdeviceName = "EPOS4";
std::string subprotocolStackName = "CANopen";

EAppMode g_eAppMode = AM_DEMO;

const std::string g_programName = "HelloEposCmd";

#ifndef MMC_SUCCESS
#define MMC_SUCCESS 0
#endif

#ifndef MMC_FAILED
#define MMC_FAILED 1
#endif

#ifndef MMC_MAX_LOG_MSG_SIZE
#define MMC_MAX_LOG_MSG_SIZE 512
#endif

void PrintUsage();
void PrintHeader();
void PrintSettings();
int OpenDevice(DWORD *p_pErrorCode);
bool CloseDevice(DWORD *p_pErrorCode);
void SetDefaultParameters();
bool ParseArguments(int argc, char **argv);
int DemoProfilePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, DWORD &p_rlErrorCode);

bool Demo(DWORD *p_pErrorCode);
bool clearFaultsAndEnable(DWORD *p_pErrorCode, WORD nodeId);
int PrintAvailableInterfaces();
int PrintAvailablePorts(char *p_pInterfaceNameSel);
int PrintAvailableProtocols();
void showMenu(const std::string &header, const std::vector<std::string> &menuOptions, const int &selectedOption);
int main(int argc, char **argv);
bool isFilenameValid(const std::string &filename);
bool PrintDeviceVersion();

/****************************************************************************/
/*	jodo application 														*/
void showDrivesStatus(HANDLE keyHandle);
JoyRsp runEnableMode(HANDLE pDevice,
                     std::string msg,
                     DWORD &rErrorCode);
JoyRsp runHomeMode(HANDLE pDevice,
                   std::string msg,
                   int axis,
                   DWORD &rErrorCode);

JoyRsp forceHome(HANDLE pDevice, int axis, DWORD &rErrorCode);

JoyRsp runEnableMode(HANDLE pDevice,
                     std::string msg,
                     DWORD &rErrorCode)
{
    JoyRsp rsp = JoyRsp::RUNNING;

    LogInfo(msg.c_str());

    FTrigger ftEnable[NUM_AXES], ftAccept, ftReject;
    bool buttEnable[MAX_NUM_AXES] = {0};

    while (rsp == RUNNING)
    {
        // get joystick left analog
        handlelGamecontrollerEvents();

        // accept?
        if (ftAccept.CLK(gameControllerGetButton(SDL_CONTROLLER_BUTTON_A)))
        {
            LogInfo("ACCEPT");
            rsp = JoyRsp::ACCEPT;
            continue;
        }

        // reject?
        if (ftReject.CLK(gameControllerGetButton(SDL_CONTROLLER_BUTTON_B)))
        {
            LogInfo("REJECt");
            rsp = JoyRsp::REJECT;
            continue;
        }

        // buttons to AXES
        buttEnable[0] = gameControllerGetButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        buttEnable[1] = gameControllerGetButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        buttEnable[2] = gameControllerGetButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        buttEnable[3] = gameControllerGetButton(SDL_CONTROLLER_BUTTON_DPAD_UP);
        buttEnable[4] = gameControllerGetButton(SDL_CONTROLLER_BUTTON_BACK);
        buttEnable[5] = gameControllerGetButton(SDL_CONTROLLER_BUTTON_START);

        // process enable
        for (int i = 0; i < NUM_AXES; i++)
        {
            if (ftEnable[i].CLK(buttEnable[i]))
            {
                BOOL isEnabled = false;
                if (!VCS_GetEnableState(pDevice, 1 + i, &isEnabled, &rErrorCode))
                {
                    LogError("VCS_GetEnableState",
                             false,
                             rErrorCode);
                    rsp = JoyRsp::FAULT;
                }
                if (isEnabled)
                {
                    if (!VCS_SetDisableState(pDevice, 1 + i, &rErrorCode))
                    {
                        LogError("VCS_SetDisableState",
                                 false,
                                 rErrorCode);
                        rsp = JoyRsp::FAULT;
                    }
                }
                else
                {
                    if (!VCS_SetEnableState(pDevice, 1 + i, &rErrorCode))
                    {
                        LogError("VCS_SetEnableState",
                                 false,
                                 rErrorCode);
                        rsp = JoyRsp::FAULT;
                    }
                }
            }
        }

        this_thread::sleep_for(std::chrono::milliseconds((16))); // 60 updates/sec
    }
    return rsp;
}

JoyRsp runHomeMode(HANDLE pDevice,
                   std::string msg,
                   int axis,
                   DWORD &rErrorCode)
{
    std::vector<bool> joyEnable;
    for (size_t i = 0; i < NUM_AXES; i++)
    {
        joyEnable.push_back(false);
    }
    joyEnable[axis] = true;
    JoyRsp rsp = runJoystickMode(pDevice,
                                 joyEnable,
                                 msg + std::string("axis: ") + std::to_string(axis),
                                 rErrorCode,
                                 showDrivesStatus);

    if (rsp != ACCEPT)
    {
        return rsp;
    }
    rsp = forceHome(pDevice, axis, rErrorCode);
    return rsp;
}

JoyRsp forceHome(HANDLE pDevice, int axis, DWORD &rErrorCode)
{
    JoyRsp rsp = ACCEPT;
    if (!VCS_ActivateHomingMode(pDevice, 1 + axis, &rErrorCode))
    {
        LogError("VCS_ActivateHomingMode",
                 false,
                 rErrorCode);
        rsp = JoyRsp::FAULT;
        return rsp;
    }

    DWORD acceleration,
        speedSwitch,
        speedIndex;
#ifdef __linux__
    int homeOffset;
    int homePos;
#else
    long homeOffset;
    long homePos;
#endif
    WORD currentThreshold;

    if (!VCS_GetHomingParameter(pDevice, 1 + axis,
                                &acceleration,
                                &speedSwitch,
                                &speedIndex,
                                &homeOffset,
                                &currentThreshold,
                                &homePos, &rErrorCode))
    {
        LogError("VCS_GetHomingParameter",
                 false,
                 rErrorCode);
        rsp = JoyRsp::FAULT;
        return rsp;
    }

    homePos = 0;
    if (!VCS_SetHomingParameter(pDevice, 1 + axis, acceleration,
                                speedSwitch, speedIndex, homeOffset, currentThreshold, homePos, &rErrorCode))
    {
        LogError("VCS_SetHomingParameter",
                 false,
                 rErrorCode);
        rsp = JoyRsp::FAULT;
        return rsp;
    }

    int8_t method = HM_ACTUAL_POSITION;
    if (!VCS_FindHome(pDevice, 1 + axis, method, &rErrorCode))
    {
        LogError("VCS_FindHome",
                 false,
                 rErrorCode);
        rsp = JoyRsp::FAULT;
        return rsp;
    }

    if (!VCS_WaitForHomingAttained(pDevice, 1 + axis, 100, &rErrorCode))
    {
        LogError("VCS_WaitForHomingAttained",
                 false,
                 rErrorCode);
        rsp = JoyRsp::FAULT;
        return rsp;
    }
    return rsp;
}

bool jodoPathDemo(HANDLE keyHandle, std::string &recipeSelection, DWORD *pErrorCode)
{

    ////(REVSm / sec)/sec
    // double pathAcceleration = 20;

    // std::vector<double> pathVelocity;
    // pathVelocity.push_back(1.0);
    // pathVelocity.push_back(0.05);
    // pathVelocity.push_back(0.1);
    // pathVelocity.push_back(0.15);
    // pathVelocity.push_back(0.2);
    // pathVelocity.push_back(0.25);
    // pathVelocity.push_back(0.30);
    // pathVelocity.push_back(0.35);
    // pathVelocity.push_back(0.40);

    //
    // std::vector<double> pos[NUM_AXES];

    //// units in motor revs
    // pos[0].push_back(0.0f);
    // pos[0].push_back(0.2f);
    // pos[0].push_back(0.5f);
    // pos[0].push_back(0.7f);
    // pos[0].push_back(1.0f);
    // pos[0].push_back(1.3f);
    // pos[0].push_back(1.7f);
    // pos[0].push_back(1.7f);
    // pos[0].push_back(1.7f);

    // pos[1].push_back(0.0f);
    // pos[1].push_back(0.1f * 2.0f);
    // pos[1].push_back(0.2f * 2);
    // pos[1].push_back(0.3f * 2);
    // pos[1].push_back(0.4f * 2);
    // pos[1].push_back(-0.5f * 2);
    // pos[1].push_back(0.6f * 2);
    // pos[1].push_back(-0.6f * 2);
    // pos[1].push_back(-2.8f);

    TeachData data;
    if (!loadDataFromXML(data, recipeSelection))
    {
        std::cerr << "Error loading data." << std::endl;
        return false;
    }

    bool success = jodoContinuousCatheterPath(keyHandle,
                                              data,
                                              *pErrorCode);
    if (!success)
    {
        LogError("jodoDemoProfilePositionMode", success, *pErrorCode);
        return success;
    }
    LogInfo("halt position movement");
    haltPositionMovementDrives(subkeyHandle, pErrorCode);
    // disableDrives(keyHandle, pErrorCode);
    return success;
}

bool jodoTeach(HANDLE keyHandle, TeachData &teach, double defaultPathVel, DWORD *pErrorCode)
{
    JoyRsp rsp = JoyRsp::RUNNING;
    std::vector<bool> joyEnable;
    std::string msg = "jog to teach point. accept(A) exit(B)";

    for (size_t i = 0; i < NUM_AXES; i++)
    {
        joyEnable.push_back(true);
    }

    while (rsp == JoyRsp::RUNNING)
    {
        setAll(joyEnable, true);
        rsp = runJoystickMode(keyHandle,
                              joyEnable,
                              msg + "point: " + std::to_string(teach.points.size()),
                              *pErrorCode,
                              showDrivesStatus);

        if (rsp == JoyRsp::ACCEPT)
        {
            long teachPos[NUM_AXES] = {0};
            getActualPositionDrives(subkeyHandle, teachPos);
            double teachPosUserUnits[NUM_AXES] = {0};
            scalePosition(teachPos, teachPosUserUnits);
            printPosition("teachPos= ", teachPos);
            printPosition("finalPos user units= ", teachPosUserUnits);

            // add teach point
            std::vector<double> row;
            for (size_t i = 0; i < MAX_NUM_AXES; i++)
            {
                row.push_back(0);
            }
            row.push_back(defaultPathVel);

            // copy position data
            for (size_t i = 0; i < NUM_AXES; i++)
            {
                row[i] = teachPosUserUnits[i];
            }
            teach.points.push_back(row);
            rsp = JoyRsp::RUNNING;
        }
    }

    /*success = jodoContunuousCatheterPath(keyHandle,
                                        teach,
                                        *pErrorCode);
    if (!success) {
        LogError("jodoContunuousCatheterPath", success, *pErrorCode);
        return success;
    }*/
    LogInfo("halt position movement");
    haltPositionMovementDrives(subkeyHandle, pErrorCode);
    // disableDrives(keyHandle, pErrorCode);
    return teach.points.size() > 0;
}

void showDrivesStatus(HANDLE keyHandle)
{
    static int show = 0;
    static double average = 0;
    static vector<string> headers = {"enc(rev)", "cmd(rev)"};
    static vector<double> p0(NUM_AXES);
    static vector<double> p1(NUM_AXES);
    double start = SEC_TIME();
    std::vector<double> cmd(NUM_AXES);
    for (size_t i = 0; i < NUM_AXES; i++)
    {
        cmd[i] = getCommandPosition(keyHandle, 1 + i) / scld[i];
    }
    std::vector<double> enc(NUM_AXES);
    for (size_t i = 0; i < NUM_AXES; i++)
    {
        enc[i] = getActualPosition(keyHandle, 1 + i) / scld[i];
    }
    double end = SEC_TIME();
    //.7ms in win32 per command
    // 2.33 ms in linux
    double delaySec = end - start;
    average += delaySec;
    show++;
    if (show > 100)
    {
        average = average / (double)(show * 2 * NUM_AXES);
        setTextPosition(100, 8);
        cout << average;
        average = 0;
        show = 0;
    }
    displayMotion(headers, enc, cmd);
}
/****************************************************************************/

/****************************************************************************/
/*	cross platform 														*/
void clearScreen()
{
#ifdef __linux__
    system("clear");
#else
    system("cls");
#endif
}

// int getch_crossplatform()
// {
// #ifdef _WIN32
//     int ch = _getch();
//     if (ch == 0 || ch == 224)
//     {                              // Extended key
//         return ch << 8 | _getch(); // Combine the two codes
//     }
//     return ch;
// #else // Linux
//     int ch;
//     struct termios oldt, newt;

//     tcgetattr(STDIN_FILENO, &oldt);
//     newt = oldt;
//     newt.c_lflag &= ~(ICANON | ECHO);
//     newt.c_cc[VMIN] = 0;
//     newt.c_cc[VTIME] = 1; // Set a small timeout (in .1 seconds)

//     tcsetattr(STDIN_FILENO, TCSANOW, &newt);

//     fd_set fds;
//     FD_ZERO(&fds);
//     FD_SET(STDIN_FILENO, &fds);
//     struct timeval tv;
//     tv.tv_sec = 0;
//     tv.tv_usec = 100000; // micro second timeout

//     int retval = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

//     if (retval == -1)
//     {
//         perror("select()");
//         ch = -1;
//     }
//     else if (retval > 0)
//     {
//         ch = getchar();
//     }
//     else
//     {
//         ch = -1; // No key pressed or timeout
//     }

//     tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
//     return ch;
// #endif
// }

#ifdef __linux__
static struct termios old, current;

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
    tcgetattr(0, &old);         /* grab old terminal i/o settings */
    current = old;              /* make new settings same as old settings */
    current.c_lflag &= ~ICANON; /* disable buffered i/o */
    if (echo)
    {
        current.c_lflag |= ECHO; /* set echo mode */
    }
    else
    {
        current.c_lflag &= ~ECHO; /* set no echo mode */
    }
    tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
    tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo)
{
    char ch;
    initTermios(echo);
    ch = getchar();
    resetTermios();
    return ch;
}

/* Read 1 character without echo */
char _getch(void)
{
    return getch_(0);
}

/* Read 1 character with echo */
char getche(void)
{
    return getch_(1);
}
bool _kbhit()
{
    int ch;
    struct termios oldt, newt;

    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // Set terminal to non-canonical mode, no echo, minimum 0 characters to read
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 1; // Set timeout to 0.1 seconds

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Use select() to check for input without blocking
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 0.1 second timeout

    int retval = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (retval == -1)
    {
        perror("select()"); // Handle error
        return false;
    }
    else if (retval > 0)
    {
        return true; // Key was pressed
    }
    else
    {
        return false; // No key was pressed within timeout
    }
}
#endif

/****************************************************************************/

void PrintUsage()
{
    std::cout << "Usage: HelloEposCmd" << std::endl;
    std::cout << "\t-h : this help" << std::endl;
    std::cout << "\t-n : node id (default 1)" << std::endl;
    std::cout << "\t-d   : device name (EPOS2, EPOS4, default - EPOS4)" << std::endl;
    std::cout << "\t-s   : protocol stack name (MAXON_RS232, CANopen, MAXON SERIAL V2, default - MAXON SERIAL V2)" << std::endl;
    std::cout << "\t-i   : interface name (RS232, USB, CAN_ixx_usb 0, CAN_kvaser_usb 0,... default - USB)" << std::endl;
    std::cout << "\t-p   : port name (COM1, USB0, CAN0,... default - USB0)" << std::endl;
    std::cout << "\t-b   : baudrate (115200, 1000000,... default - 1000000)" << std::endl;
    std::cout << "\t-l   : list available interfaces (valid device name and protocol stack required)" << std::endl;
    std::cout << "\t-r   : list supported protocols (valid device name required)" << std::endl;
    std::cout << "\t-v   : display device version" << std::endl;
}

void SeparatorLine()
{
    const int lineLength = 65;
    for (int i = 0; i < lineLength; i++)
    {
        std::cout << "-";
    }
    std::cout << std::endl;
}

void PrintSettings()
{
    std::stringstream msg;

    msg << "default settings:" << std::endl;
    msg << "node id             = " << g_usNodeId << std::endl;
    msg << "device name         = '" << g_deviceName << "'" << std::endl;
    msg << "protocal stack name = '" << g_protocolStackName << "'" << std::endl;
    msg << "interface name      = '" << g_interfaceName << "'" << std::endl;
    msg << "port name           = '" << g_portName << "'" << std::endl;
    msg << "baudrate            = " << g_baudrate;

    LogInfo(msg.str());

    SeparatorLine();
}

void SetDefaultParameters()
{
    // USB
    g_usNodeId = 1;
    g_deviceName = "EPOS4";
    g_protocolStackName = "MAXON SERIAL V2";
    g_interfaceName = "USB";
    g_portName = "USB0";
    g_baudrate = 1000000;

    g_usNodeId = 1;
    g_deviceName = "EPOS4";
    g_protocolStackName = "CANopen";
#ifdef __linux__
    g_interfaceName = "CAN_ixx_usb 0";
#else
    g_interfaceName = "IXXAT_USB-to-CAN V2 compact 0";
#endif
    g_portName = "CAN0";
    g_baudrate = 1000000;
}

int OpenDevice(DWORD *pErrorCode)
{
    int success = false;

    const int SIZE = 255;
    char pDeviceName[SIZE] = {0};
    char pProtocolStackName[SIZE] = {0};
    char pInterfaceName[SIZE] = {0};
    char pPortName[SIZE] = {0};

    strcpy(pDeviceName, g_deviceName.c_str());
    strcpy(pProtocolStackName, g_protocolStackName.c_str());
    strcpy(pInterfaceName, g_interfaceName.c_str());
    strcpy(pPortName, g_portName.c_str());

    LogInfo("Open device...");

    subkeyHandle = 0;
    g_pKeyHandle = VCS_OpenDevice(pDeviceName,
                                  pProtocolStackName,
                                  pInterfaceName,
                                  pPortName,
                                  pErrorCode);

    if (g_pKeyHandle == 0 || *pErrorCode > 0)
    {
        return success;
    }

    DWORD lBaudrate = 0;
    DWORD lTimeout = 0;

    if (!VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, pErrorCode))
    {
        return success;
    }
    if (!VCS_SetProtocolStackSettings(g_pKeyHandle, g_baudrate, lTimeout, pErrorCode))
    {
        return success;
    }
    if (!VCS_GetProtocolStackSettings(g_pKeyHandle, &lBaudrate, &lTimeout, pErrorCode))
    {
        return success;
    }
    if (g_baudrate != (int)lBaudrate)
    {
        return success;
    }
    success = true;
    return success;

    subkeyHandle = VCS_OpenSubDevice(g_pKeyHandle,
                                     (char *)subdeviceName.c_str(),
                                     (char *)subprotocolStackName.c_str(),
                                     pErrorCode);
    if (subkeyHandle == 0)
    {
        return success;
    }

    if (!VCS_GetGatewaySettings(g_pKeyHandle, &lBaudrate, pErrorCode))
    {
        return success;
    }
    printf("Gateway baudrate = %u\r\n", g_baudrate);
    success = true;
    return success;
}

bool CloseDevice(DWORD *pErrorCode)
{
    bool success = false;

    *pErrorCode = 0;

    if (subkeyHandle)
    {
        LogInfo("Close subdevice");
        if (!VCS_CloseSubDevice(subkeyHandle, pErrorCode))
        {
            return success;
        }
    }

    LogInfo("Close device");
    if (!VCS_CloseDevice(g_pKeyHandle, pErrorCode))
    {
        return success;
    }

    if (*pErrorCode != 0)
    {
        return success;
    }
    success = true;
    return success;
}

bool ParseArguments(int argc, char **argv)
{
    int lOption;
    bool success = true;

    opterr = 0;

    while ((lOption = getopt(argc, argv, "hlrvd:s:i:p:b:n:")) != -1)
    {
        switch (lOption)
        {
        case 'h':
            PrintUsage();
            success = false;
            break;
        case 'd':
            g_deviceName = optarg;
            break;
        case 's':
            g_protocolStackName = optarg;
            break;
        case 'i':
            g_interfaceName = optarg;
            break;
        case 'p':
            g_portName = optarg;
            break;
        case 'b':
            g_baudrate = atoi(optarg);
            break;
        case 'n':
            g_usNodeId = (unsigned short)atoi(optarg);
            break;
        case 'l':
            g_eAppMode = AM_INTERFACE_LIST;
            break;
        case 'r':
            g_eAppMode = AM_PROTOCOL_LIST;
            break;
        case 'v':
            g_eAppMode = AM_VERSION_INFO;
            break;
        case '?': // unknown option...
            std::stringstream msg;
            msg << "Unknown option: '" << char(optopt) << "'!";
            LogInfo(msg.str());
            PrintUsage();
            success = false;
            break;
        }
    }

    return success;
}

int DemoProfilePositionMode(HANDLE p_DeviceHandle, unsigned short p_usNodeId, DWORD &p_rlErrorCode)
{
    int lResult = MMC_SUCCESS;
    std::stringstream msg;

    msg << "set profile position mode, node = " << p_usNodeId;
    LogInfo(msg.str());

    if (VCS_ActivateProfilePositionMode(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
    {
        LogError("VCS_ActivateProfilePositionMode", lResult, p_rlErrorCode);
        lResult = MMC_FAILED;
    }
    else
    {
        std::list<long> positionList;

        positionList.push_back(50000);
        positionList.push_back(-100000);
        positionList.push_back(50000);
        positionList.push_back(-100006);

        for (std::list<long>::iterator it = positionList.begin(); it != positionList.end(); it++)
        {
            long targetPosition = (*it);
            std::stringstream msg;
            msg << "move to position = " << targetPosition << ", node = " << p_usNodeId;
            LogInfo(msg.str());

            bool immediate = true;
            bool absolute = true;
            if (VCS_MoveToPosition(p_DeviceHandle, p_usNodeId, targetPosition, absolute, immediate, &p_rlErrorCode) == 0)
            {
                LogError("VCS_MoveToPosition", lResult, p_rlErrorCode);
                lResult = MMC_FAILED;
                break;
            }

            while (!getTargetReached(p_DeviceHandle, p_usNodeId))
            {

                // if (VCS_GetPositionMust(p_DeviceHandle, p_usNodeId, &cmdPos, &p_rlErrorCode)) {
                // if (VCS_GetTargetPosition(p_DeviceHandle, p_usNodeId, &cmdPos, &p_rlErrorCode)) {
                long cmdPos = getCommandPosition(p_DeviceHandle, p_usNodeId);
                std::stringstream msg;
                msg << "cmd position = " << cmdPos << ", node = " << p_usNodeId;
                LogInfo(msg.str());
                this_thread::sleep_for(std::chrono::milliseconds((1)));
            }

#ifdef __linux__
            int finalPos = 0;
#else
            long finalPos = 0;
#endif

            if (VCS_GetPositionIs(p_DeviceHandle, p_usNodeId, &finalPos, &p_rlErrorCode))
            {
                std::stringstream msg;
                msg << "final position = " << finalPos << ", node = " << p_usNodeId;
                LogInfo(msg.str());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds((1)));
        }

        if (lResult == MMC_SUCCESS)
        {
            LogInfo("halt position movement");

            if (VCS_HaltPositionMovement(p_DeviceHandle, p_usNodeId, &p_rlErrorCode) == 0)
            {
                LogError("VCS_HaltPositionMovement", lResult, p_rlErrorCode);
                lResult = MMC_FAILED;
            }
        }
    }

    return lResult;
}

bool clearFaultsAndEnable(DWORD *pErrorCode, WORD nodeId)
{
    bool success = true;
    BOOL oIsFault = 0;

    if (!VCS_GetFaultState(g_pKeyHandle, nodeId, &oIsFault, pErrorCode))
    {
        LogError("VCS_GetFaultState", success, *pErrorCode);
        success = false;
        return success;
    }

    if (oIsFault)
    {
        std::stringstream msg;
        msg << "clear fault, node = '" << nodeId << "'";
        LogInfo(msg.str());

        if (!VCS_ClearFault(g_pKeyHandle, nodeId, pErrorCode))
        {
            LogError("VCS_ClearFault", success, *pErrorCode);
            success = false;
            return success;
        }
    }

    BOOL oIsEnabled = 0;
    if (!VCS_GetEnableState(g_pKeyHandle, nodeId, &oIsEnabled, pErrorCode))
    {
        LogError("VCS_GetEnableState", success, *pErrorCode);
        success = false;
        return success;
    }

    if (!oIsEnabled)
    {
        if (VCS_SetEnableState(g_pKeyHandle, nodeId, pErrorCode) == 0)
        {
            LogError("VCS_SetEnableState", success, *pErrorCode);
            success = false;
        }
    }
    return success;
}

int MaxFollowingErrorDemo(DWORD &p_rlErrorCode)
{
    int lResult = MMC_SUCCESS;
    const unsigned int EXPECTED_ERROR_CODE = 0x8611;
    DWORD lDeviceErrorCode = 0;

    lResult = VCS_ActivateProfilePositionMode(subkeyHandle, g_usNodeId, &p_rlErrorCode);

    if (lResult)
    {
        lResult = VCS_SetMaxFollowingError(subkeyHandle, g_usNodeId, 1, &p_rlErrorCode);
    }

    if (lResult)
    {
        lResult = VCS_MoveToPosition(subkeyHandle, g_usNodeId, 1000, 1, 1, &p_rlErrorCode);
    }

    if (lResult)
    {
        lResult = VCS_GetDeviceErrorCode(subkeyHandle, g_usNodeId, 1, &lDeviceErrorCode, &p_rlErrorCode);
    }

    if (lResult)
    {
        lResult = lDeviceErrorCode == EXPECTED_ERROR_CODE ? MMC_SUCCESS : MMC_FAILED;
    }

    return lResult;
}

bool Demo(DWORD *pErrorCode)
{
    bool success = true;
    std::vector<bool> joyEnable;
    for (size_t i = 0; i < NUM_AXES; i++)
    {
        joyEnable.push_back(true);
    }

    success = runJoystickMode(subkeyHandle, joyEnable, "Accept or reject", *pErrorCode, showDrivesStatus);
    if (!success)
    {
        LogError("DemoProfileVelocityMode", success, *pErrorCode);
        return success;
    }

    success = DemoProfilePositionMode(subkeyHandle, g_usNodeId, *pErrorCode);
    if (!success)
    {
        LogError("DemoProfilePositionMode", success, *pErrorCode);
        return success;
    }

    if (!VCS_SetDisableState(subkeyHandle, g_usNodeId, pErrorCode))
    {
        LogError("VCS_SetDisableState", success, *pErrorCode);
        success = false;
    }

    return success;
}

void PrintHeader()
{
    SeparatorLine();
    LogInfo("Epos Command Library Example Program, (c) maxonmotor ag 2014-2019");
    SeparatorLine();
}

int PrintAvailablePorts(char *p_pInterfaceNameSel)
{
    int lResult = MMC_FAILED;
    int lStartOfSelection = 1;
    const int lMaxStrSize = 255;
    char pPortNameSel[lMaxStrSize] = {0};
    int lEndOfSelection = 0;
    DWORD ulErrorCode = 0;

    do
    {
        if (!VCS_GetPortNameSelection((char *)g_deviceName.c_str(), (char *)g_protocolStackName.c_str(), p_pInterfaceNameSel, lStartOfSelection, pPortNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
        {
            lResult = MMC_FAILED;
            LogError("GetPortNameSelection", lResult, ulErrorCode);
            break;
        }
        else
        {
            lResult = MMC_SUCCESS;
            printf("            port = %s\n", pPortNameSel);
        }

        lStartOfSelection = 0;
    } while (lEndOfSelection == 0);

    return lResult;
}

int PrintAvailableInterfaces()
{
    int lResult = MMC_FAILED;
    int lStartOfSelection = 1;
    const int lMaxStrSize = 255;
    char pInterfaceNameSel[lMaxStrSize] = {0};
    int lEndOfSelection = 0;
    DWORD ulErrorCode = 0;

    do
    {
        if (!VCS_GetInterfaceNameSelection((char *)g_deviceName.c_str(),
                                           (char *)g_protocolStackName.c_str(),
                                           lStartOfSelection,
                                           pInterfaceNameSel,
                                           lMaxStrSize,
                                           &lEndOfSelection,
                                           &ulErrorCode))
        {
            lResult = MMC_FAILED;
            LogError("GetInterfaceNameSelection", lResult, ulErrorCode);
            break;
        }
        else
        {
            lResult = MMC_SUCCESS;

            printf("interface = %s\n", pInterfaceNameSel);

            PrintAvailablePorts(pInterfaceNameSel);
        }

        lStartOfSelection = 0;
    } while (lEndOfSelection == 0);

    SeparatorLine();

    return lResult;
}

bool PrintDeviceVersion()
{
    int success = false;
    unsigned short usHardwareVersion = 0;
    unsigned short usSoftwareVersion = 0;
    unsigned short usApplicationNumber = 0;
    unsigned short usApplicationVersion = 0;
    DWORD ulErrorCode = 0;

    if (VCS_GetVersion(g_pKeyHandle, g_usNodeId, &usHardwareVersion, &usSoftwareVersion, &usApplicationNumber, &usApplicationVersion, &ulErrorCode))
    {
        printf("%s Hardware Version    = 0x%04x\n      Software Version    = 0x%04x\n      Application Number  = 0x%04x\n      Application Version = 0x%04x\n",
               g_deviceName.c_str(), usHardwareVersion, usSoftwareVersion, usApplicationNumber, usApplicationVersion);
        success = true;
    }

    return success;
}

int PrintAvailableProtocols()
{
    int lResult = MMC_FAILED;
    int lStartOfSelection = 1;
    const int lMaxStrSize = 255;
    char pProtocolNameSel[lMaxStrSize];
    int lEndOfSelection = 0;
    DWORD ulErrorCode = 0;

    do
    {
        if (!VCS_GetProtocolStackNameSelection((char *)g_deviceName.c_str(), lStartOfSelection, pProtocolNameSel, lMaxStrSize, &lEndOfSelection, &ulErrorCode))
        {
            lResult = MMC_FAILED;
            LogError("GetProtocolStackNameSelection", lResult, ulErrorCode);
            break;
        }
        else
        {
            lResult = MMC_SUCCESS;

            printf("protocol stack name = %s\n", pProtocolNameSel);
        }

        lStartOfSelection = 0;
    } while (lEndOfSelection == 0);

    SeparatorLine();

    return lResult;
}

void showRnDMenu()
{
    clearScreen();

    std::cout << "1] disable/enable drive\r\n"
                 "2] Manually home unit\r\n"
                 "3] joystick jog.\r\n"
                 "4] teach points recipe\r\n"
                 "5] run continuous trajectory\r\n"
                 "6] exit\r\n\r\n";
}

std::string selectMenu(std::string header, int &selectedOption, std::vector<std::string> menuOptions)
{
    ; // Initialize selection to the first option
    bool update = true;
    FTrigger ftUp, ftDown, ftEnter;
    while (true)
    {
        if (update)
        {
            showMenu(header, menuOptions, selectedOption);
            update = false;
        }

        handlelGamecontrollerEvents();

        // Get character input without echoing to the console
        int ch = -1;
        if (_kbhit())
        {
            ch = _getch();
#ifdef __linux__
            if (ch == '\033')
            { // Escape sequence (often for special keys)
                int nextKey = _getch();
                if (nextKey == '[')
                {
                    int finalKey = _getch();
                    switch (finalKey)
                    {
                    case 'A':
                        // Up arrow (Linux)
                        ch = 72;
                        break;
                    case 'B':
                        // Down arrow (Linux)
                        ch = 80;
                        break;
                    default:
                        break;
                    }
                }
            }
#endif
            update = true;
        }

        // translate to keyboard input
        if (ftDown.CLK(gameControllerGetButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN)))
        {
            ch = 80;
            update = true;
        }
        if (ftUp.CLK(gameControllerGetButton(SDL_CONTROLLER_BUTTON_DPAD_UP)))
        {
            ch = 72;
            update = true;
        }
        if (ftEnter.CLK(gameControllerGetButton(SDL_CONTROLLER_BUTTON_A)))
        {
            ch = 10;
            update = true;
        }

        switch (ch)
        {
        case 72: // Up arrow key
            selectedOption = (selectedOption - 1 + menuOptions.size()) % menuOptions.size();
            break;
        case 80: // Down arrow key
            selectedOption = (selectedOption + 1) % menuOptions.size();
            break;
        case 10: // Enter key linux
            return menuOptions[selectedOption];
        case 13: // Enter key win32
            return menuOptions[selectedOption];
        }
        this_thread::sleep_for(std::chrono::milliseconds((2)));
    }
}

void showMenu(const std::string &header, const std::vector<std::string> &menuOptions, const int &selectedOption)
{

    // Clear the console (Windows-specific)
    clearScreen();
    // setTextPosition(0, 1);

    // Display the menu with highlighting
    std::cout << header;
    for (size_t i = 0; i < menuOptions.size(); ++i)
    {
        if (i == selectedOption)
        {
            std::cout << "\033[1;32m"; // Green highlight (ANSI escape codes)
            std::cout << "> ";         // Add a selection indicator
        }
        std::cout << i + 1 << "] " << menuOptions[i] << std::endl;

        if (i == selectedOption)
        {
            std::cout << "\033[0m"; // Reset color
        }
    }
}

std::string yesNoUpdateProfile()
{
    static int selection = 0;
    std::vector<std::string> profileActions = {
        "yes",
        "no",
    };
    std::string selectedAction = selectMenu(
        "\nupdate recipe profile\n"
        "press enter for final selection :\n",
        selection,
        profileActions);
    return selectedAction;
}

void yawn(int ms)
{
    this_thread::sleep_for(std::chrono::milliseconds((ms)));
}

void hitIt()
{
    while (!_kbhit())
    {
        yawn(1);
    }
}
int main(int argc, char **argv)
{

    if (!initializeGamecontroller())
    {
        return 1;
    }
    // read a list of recipe files
    const std::string g_recipeDir = "./recipes/";
    namespace fs = std::filesystem;
    bool found = fs::exists(g_recipeDir) && fs::is_directory(g_recipeDir);

    if (!found)
    {
        fs::create_directory(g_recipeDir);
    }
    std::vector<std::string> recipes = readRecipeFiles(g_recipeDir);

    if (recipes.size() == 0)
    {
        std::cout << "no recipes selected\r\n";
        return 1;
    }
    int recipeSelection = 0;
    // std::string selectedRecipe = selectMenu(
    //     "\nuse arrow key to select recipe\n"
    //     "press enter for final selection :\n",
    //     recipeSelection,
    //     recipes);

    PrintHeader();

    SetDefaultParameters();

    PrintAvailableInterfaces();
    // hitIt();
    PrintAvailableProtocols();
    // hitIt();
    bool success = false;
    if (!(success = ParseArguments(argc, argv)))
    {
        return success;
    }

    PrintSettings();

    DWORD errorCode = 0;
    switch (g_eAppMode)
    {
    case AM_DEMO:
    {
        if (!(success = OpenDevice(&errorCode)))
        {
            LogError("OpenDevice", success, errorCode);
            return success;
        }

        for (size_t i = 0; i < NUM_AXES; i++)
        {
            if (!(success = clearFaultsAndEnable(&errorCode, 1 + i)))
            {
                LogError("PrepareDemo", success, errorCode);
                return success;
            }
        }

        // region map-menu-string-to-action
        bool done = false;
        std::vector<std::string> actions = {
            "disable/enable drive",
            "Manually home unit",
            "joystick jog.",
            "teach points recipe",
            "run continuous trajectory",
            "run jog trajectory",
            "exit",
        };
        std::map<std::string, std::function<void()>> menuActions = {
            {actions[0], [&errorCode]()
             {
                 clearScreen();
                 JoyRsp rsp = runEnableMode(g_pKeyHandle,
                                            "Enable/DisabLe left(1) right(2) down(3) up(4) share(5) option(6)",
                                            errorCode);

                 if (!(rsp == ACCEPT))
                 {
                     LogError("runEnableMode", false, errorCode);
                 };
             }},
            {actions[1], [&errorCode]()
             {
                 clearScreen();
                 for (size_t i = 0; i < NUM_AXES; i++)
                 {
                     JoyRsp rsp = runHomeMode(g_pKeyHandle,
                                              "Jog to position and home(A) or skip(B)",
                                              i,
                                              errorCode);
                     if (!((rsp != FAULT)))
                     {
                         LogError("runHomeMode", false, errorCode);
                     }
                 }

                 long finalPos[NUM_AXES] = {0};
                 getActualPositionDrives(g_pKeyHandle, finalPos);
                 double finalPosUserUnits[NUM_AXES] = {0};
                 scalePosition(finalPos, finalPosUserUnits);
                 printPosition("finalPos= ", finalPos);
                 printPosition("finalPos user units= ", finalPosUserUnits);
             }},
            {actions[2], [&errorCode]()
             {
                 clearScreen();
                 std::vector<bool> joyEnable(NUM_AXES, true);
                 JoyRsp rsp = runJoystickMode(g_pKeyHandle,
                                              joyEnable,
                                              "accept(A) or reject(B)",
                                              errorCode,
                                              showDrivesStatus);

                 if (!((rsp == ACCEPT)))
                 {
                     LogError("runJoystickMode", false, errorCode);
                 }
             }},
            {actions[3], [&errorCode, &recipes, &g_recipeDir]()
             {
                 clearScreen();
                 std::string filename;
                 std::cout << "enter teach file name\n";
                 std::cin >> filename;

                 if (!isFilenameValid(filename))
                 {
                     return;
                 }

                 TeachData teach;
                 std::string fullFilename = g_recipeDir + filename;
                 teach.name = fullFilename;
                 teach.pathAcceleration = 20;
                 const double defaultPathVel = 0.5;

                 if (!jodoTeach(g_pKeyHandle, teach, defaultPathVel, &errorCode))
                 {
                     LogError("jodoTeach", false, errorCode);
                     return;
                 }

                 if (teach.points.size() > 0)
                 {
                     if (!saveDataToXML(teach, fullFilename))
                     {
                         std::cerr << "Error saving data." << std::endl;
                         return;
                     }
                     // update list of files
                     recipes = readRecipeFiles(g_recipeDir);
                 }
             }},
            {actions[4], [&errorCode, &recipeSelection, &recipes]()
             {
                 std::string selectedRecipe = selectMenu(
                     "\nuse arrow key to select recipe\n"
                     "press enter for final selection :\n",
                     recipeSelection,
                     recipes);
                 if (!jodoPathDemo(g_pKeyHandle, selectedRecipe, &errorCode))
                 {
                     LogError("jodoPathDemo", false, errorCode);
                 }
             }},
            {actions[5], [&errorCode, &recipeSelection, &recipes]()
             {
                 std::string selectedRecipe = selectMenu(
                     "\nuse arrow key to select recipe\n"
                     "press enter for final selection :\n",
                     recipeSelection,
                     recipes);
                 TeachData data;
                 if (!loadDataFromXML(data, selectedRecipe))
                 {
                     std::cerr << "Error loading data." << std::endl;
                     return;
                 }

                 TeachData copy;
                 bool success = jodoJogCatheterPath(g_pKeyHandle,
                                                    data,
                                                    copy,
                                                    showDrivesStatus,
                                                    errorCode);
                 if (!success)
                 {
                     LogError("jodoDemoProfilePositionMode", success, errorCode);
                     return;
                 }

                 haltPositionMovementDrives(g_pKeyHandle, &errorCode);
                 if (yesNoUpdateProfile() == "yes")
                 {
                     copy.name = data.name;
                     copy.pathAcceleration = data.pathAcceleration;
                     if (!saveDataToXML(copy, selectedRecipe))
                     {
                         std::cerr << "Error saving data." << std::endl;
                         return;
                     }
                 }
             }},
            {actions[6], [&done]()
             {
                 clearScreen();
                 done = true;
             }},
        };

        // endregion

        int actionSelection = 0;
        while (!done)
        {
            std::string selectedAction = selectMenu(
                "\nuse arrow key to select action\n"
                "press enter for final selection :\n",
                actionSelection,
                actions);
            menuActions[selectedAction](); // Call the associated function
        }

        disableDrives(g_pKeyHandle, &errorCode);

        if (!(success = CloseDevice(&errorCode)))
        {
            LogError("CloseDevice", success, errorCode);
            return success;
        }
    }
    break;
    case AM_INTERFACE_LIST:
        PrintAvailableInterfaces();
        break;
    case AM_PROTOCOL_LIST:
        PrintAvailableProtocols();
        break;
    case AM_VERSION_INFO:
    {
        if (!(success = OpenDevice(&errorCode)))
        {
            LogError("OpenDevice", success, errorCode);
            return success;
        }

        if (!(success = PrintDeviceVersion()))
        {
            LogError("PrintDeviceVersion", success, errorCode);
            return success;
        }

        if (!(success = CloseDevice(&errorCode)))
        {
            LogError("CloseDevice", success, errorCode);
            return success;
        }
    }
    break;
    case AM_UNKNOWN:
        printf("unknown option\n");
        break;
    }
    closeGamecontroller();
    return success;
}

bool isFilenameValid(const std::string &filename)
{

    if (filename.empty())
    {
        return false;
    }

    if (filename.find_first_of(" \t\n\r\f\v\\/*?\"<>|:") != std::string::npos)
    { // Check for invalid characters
        return false;
    }
    return true;
}
