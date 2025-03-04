/*
 Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely.
*/

/* Simple program to test the SDL game controller routines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "testutils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include "testgamecontroller.h"

#define SDL_bool bool

//#ifdef SDL_JOYSTICK_DISABLED

#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 320

#define BUTTON_SIZE 50
#define AXIS_SIZE 50

#define BUTTON_SIZE 50
#define AXIS_SIZE 50

/* This is indexed by SDL_GameControllerButton. */
static const struct
{
    int x;
    int y;
} button_positions[] = {
    {387, 167}, /* SDL_CONTROLLER_BUTTON_A */
    {431, 132}, /* SDL_CONTROLLER_BUTTON_B */
    {342, 132}, /* SDL_CONTROLLER_BUTTON_X */
    {389, 101}, /* SDL_CONTROLLER_BUTTON_Y */
    {174, 132}, /* SDL_CONTROLLER_BUTTON_BACK */
    {232, 128}, /* SDL_CONTROLLER_BUTTON_GUIDE */
    {289, 132}, /* SDL_CONTROLLER_BUTTON_START */
    {75, 154},  /* SDL_CONTROLLER_BUTTON_LEFTSTICK */
    {305, 230}, /* SDL_CONTROLLER_BUTTON_RIGHTSTICK */
    {77, 40},   /* SDL_CONTROLLER_BUTTON_LEFTSHOULDER */
    {396, 36},  /* SDL_CONTROLLER_BUTTON_RIGHTSHOULDER */
    {154, 188}, /* SDL_CONTROLLER_BUTTON_DPAD_UP */
    {154, 249}, /* SDL_CONTROLLER_BUTTON_DPAD_DOWN */
    {116, 217}, /* SDL_CONTROLLER_BUTTON_DPAD_LEFT */
    {186, 217}, /* SDL_CONTROLLER_BUTTON_DPAD_RIGHT */
    {232, 174}, /* SDL_CONTROLLER_BUTTON_MISC1 */
    {132, 135}, /* SDL_CONTROLLER_BUTTON_PADDLE1 */
    {330, 135}, /* SDL_CONTROLLER_BUTTON_PADDLE2 */
    {132, 175}, /* SDL_CONTROLLER_BUTTON_PADDLE3 */
    {330, 175}, /* SDL_CONTROLLER_BUTTON_PADDLE4 */
    {0, 0},     /* SDL_CONTROLLER_BUTTON_TOUCHPAD */
    //{ 0, 0 },     /* SDL_CONTROLLER_BUTTON_RTOUCHPAD */
};
SDL_COMPILE_TIME_ASSERT(button_positions, SDL_arraysize(button_positions) == SDL_CONTROLLER_BUTTON_MAX);

/* This is indexed by SDL_GameControllerAxis. */
static const struct
{
    int x;
    int y;
    double angle;
} axis_positions[] = {
    {74, 153, 270.0},  /* LEFTX */
    {74, 153, 0.0},    /* LEFTY */
    {306, 231, 270.0}, /* RIGHTX */
    {306, 231, 0.0},   /* RIGHTY */
    {91, -20, 0.0},    /* TRIGGERLEFT */
    {375, -20, 0.0},   /* TRIGGERRIGHT */
};
SDL_COMPILE_TIME_ASSERT(axis_positions, SDL_arraysize(axis_positions) == SDL_CONTROLLER_AXIS_MAX);

/* This is indexed by SDL_JoystickPowerLevel + 1. */
static const char *power_level_strings[] = {
    "unknown", /* SDL_JOYSTICK_POWER_UNKNOWN */
    "empty",   /* SDL_JOYSTICK_POWER_EMPTY */
    "low",     /* SDL_JOYSTICK_POWER_LOW */
    "medium",  /* SDL_JOYSTICK_POWER_MEDIUM */
    "full",    /* SDL_JOYSTICK_POWER_FULL */
    "wired",   /* SDL_JOYSTICK_POWER_WIRED */
};
SDL_COMPILE_TIME_ASSERT(power_level_strings, SDL_arraysize(power_level_strings) == SDL_JOYSTICK_POWER_MAX + 1);

// static SDL_Window *window = NULL;
// static SDL_Renderer *screen = NULL;
static SDL_bool retval = SDL_FALSE;
static SDL_bool set_LED = SDL_FALSE;
static int trigger_effect = 0;
// static SDL_Texture *background_front, *background_back, *button_texture, *axis_texture;
static SDL_GameController *gamecontroller;
static SDL_GameController **gamecontrollers;
static int num_controllers = 0;
static SDL_Joystick *virtual_joystick = NULL;
static SDL_GameControllerAxis virtual_axis_active = SDL_CONTROLLER_AXIS_INVALID;
static int virtual_axis_start_x;
static int virtual_axis_start_y;
static SDL_GameControllerButton virtual_button_active = SDL_CONTROLLER_BUTTON_INVALID;

// static void UpdateWindowTitle(void)
//{
//     if (!window) {
//         return;
//     }
//
//     if (gamecontroller) {
//         const char *name = SDL_GameControllerName(gamecontroller);
//         const char *serial = SDL_GameControllerGetSerial(gamecontroller);
//         const char *basetitle = "Game Controller Test: ";
//         const size_t titlelen = SDL_strlen(basetitle) + (name ? SDL_strlen(name) : 0) + (serial ? 3 + SDL_strlen(serial) : 0) + 1;
//         char *title = (char *)SDL_malloc(titlelen);
//
//         retval = SDL_FALSE;
//         done = SDL_FALSE;
//
//         if (title) {
//             SDL_strlcpy(title, basetitle, titlelen);
//             if (name) {
//                 SDL_strlcat(title, name, titlelen);
//             }
//             if (serial) {
//                 SDL_strlcat(title, " (", titlelen);
//                 SDL_strlcat(title, serial, titlelen);
//                 SDL_strlcat(title, ")", titlelen);
//             }
//             SDL_SetWindowTitle(window, title);
//             SDL_free(title);
//         }
//     } else {
//         SDL_SetWindowTitle(window, "Waiting for controller...");
//     }
// }

static const char *GetSensorName(SDL_SensorType sensor)
{
    switch (sensor)
    {
    case SDL_SENSOR_ACCEL:
        return "accelerometer";
    case SDL_SENSOR_GYRO:
        return "gyro";
    case SDL_SENSOR_ACCEL_L:
        return "accelerometer (L)";
    case SDL_SENSOR_GYRO_L:
        return "gyro (L)";
    case SDL_SENSOR_ACCEL_R:
        return "accelerometer (R)";
    case SDL_SENSOR_GYRO_R:
        return "gyro (R)";
    default:
        return "UNKNOWN";
    }
}

static int FindController(SDL_JoystickID controller_id)
{
    int i;

    for (i = 0; i < num_controllers; ++i)
    {
        if (controller_id == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamecontrollers[i])))
        {
            return i;
        }
    }
    return -1;
}

static void AddController(int device_index, SDL_bool verbose)
{
    SDL_JoystickID controller_id = SDL_JoystickGetDeviceInstanceID(device_index);
    SDL_GameController *controller;
    SDL_GameController **controllers;
    Uint16 firmware_version;
    SDL_SensorType sensors[] = {
        SDL_SENSOR_ACCEL,
        SDL_SENSOR_GYRO,
        SDL_SENSOR_ACCEL_L,
        SDL_SENSOR_GYRO_L,
        SDL_SENSOR_ACCEL_R,
        SDL_SENSOR_GYRO_R,
        // SDL_SENSOR_LPAD,
        // SDL_SENSOR_RPAD,
    };
    unsigned int i;

    controller_id = SDL_JoystickGetDeviceInstanceID(device_index);
    if (controller_id < 0)
    {
        SDL_Log("Couldn't get controller ID: %s\n", SDL_GetError());
        return;
    }

    if (FindController(controller_id) >= 0)
    {
        /* We already have this controller */
        return;
    }

    controller = SDL_GameControllerOpen(device_index);
    if (!controller)
    {
        SDL_Log("Couldn't open controller: %s\n", SDL_GetError());
        return;
    }

    controllers = (SDL_GameController **)SDL_realloc(gamecontrollers, (num_controllers + 1) * sizeof(*controllers));
    if (!controllers)
    {
        SDL_GameControllerClose(controller);
        return;
    }

    controllers[num_controllers++] = controller;
    gamecontrollers = controllers;
    gamecontroller = controller;
    trigger_effect = 0;

    if (verbose)
    {
        const char *name = SDL_GameControllerName(gamecontroller);
        const char *path = SDL_GameControllerPath(gamecontroller);
        SDL_Log("Opened game controller %s%s%s\n",
                name,
                path ? ", " : "",
                path ? path : "");
        SDL_Log("Mapping: %s\n", SDL_GameControllerMapping(gamecontroller));
    }

    firmware_version = SDL_GameControllerGetFirmwareVersion(gamecontroller);
    if (firmware_version)
    {
        if (verbose)
        {
            SDL_Log("Firmware version: 0x%x (%d)\n", firmware_version, firmware_version);
        }
    }

    for (i = 0; i < SDL_arraysize(sensors); ++i)
    {
        SDL_SensorType sensor = sensors[i];

        if (SDL_GameControllerHasSensor(gamecontroller, sensor))
        {
            if (verbose)
            {
                SDL_Log("Enabling %s at %.2f Hz\n", GetSensorName(sensor), SDL_GameControllerGetSensorDataRate(gamecontroller, sensor));
            }
            SDL_GameControllerSetSensorEnabled(gamecontroller, sensor, SDL_TRUE);
        }
    }

    if (SDL_GameControllerHasRumble(gamecontroller))
    {
        SDL_Log("Rumble supported");
    }

    if (SDL_GameControllerHasRumbleTriggers(gamecontroller))
    {
        SDL_Log("Trigger rumble supported");
    }
    int noAxis = SDL_JoystickNumAxes(SDL_GameControllerGetJoystick(gamecontroller));
    SDL_Log("Has %d Axis\r\n", noAxis);

    int noTouchpad = SDL_GameControllerGetNumTouchpads((gamecontroller));
    SDL_Log("Has %d Touch\r\n", noTouchpad);

    int noButts = SDL_JoystickNumButtons(SDL_GameControllerGetJoystick(gamecontroller));
    SDL_Log("Has %d Butts\r\n", noButts);
    // UpdateWindowTitle();
    // UpdateWindowTitle();
}

static void SetController(SDL_JoystickID controller)
{
    int i = FindController(controller);

    if (i < 0)
    {
        return;
    }

    if (gamecontroller != gamecontrollers[i])
    {
        gamecontroller = gamecontrollers[i];
        // UpdateWindowTitle();
    }
}

static void DelController(SDL_JoystickID controller)
{
    int i = FindController(controller);

    if (i < 0)
    {
        return;
    }

    SDL_GameControllerClose(gamecontrollers[i]);

    --num_controllers;
    if (i < num_controllers)
    {
        SDL_memcpy(&gamecontrollers[i], &gamecontrollers[i + 1], (num_controllers - i) * sizeof(*gamecontrollers));
    }

    if (num_controllers > 0)
    {
        gamecontroller = gamecontrollers[0];
    }
    else
    {
        gamecontroller = NULL;
    }
    // UpdateWindowTitle();
}

static Uint16 ConvertAxisToRumble(Sint16 axisval)
{
    /* Only start rumbling if the axis is past the halfway point */
    const Sint16 half_axis = (Sint16)SDL_ceil(SDL_JOYSTICK_AXIS_MAX / 2.0f);
    if (axisval > half_axis)
    {
        return (Uint16)(axisval - half_axis) * 4;
    }
    else
    {
        return 0;
    }
}

/* PS5 trigger effect documentation:
   https://controllers.fandom.com/wiki/Sony_DualSense#FFB_Trigger_Modes
*/
typedef struct
{
    Uint8 ucEnableBits1;              /* 0 */
    Uint8 ucEnableBits2;              /* 1 */
    Uint8 ucRumbleRight;              /* 2 */
    Uint8 ucRumbleLeft;               /* 3 */
    Uint8 ucHeadphoneVolume;          /* 4 */
    Uint8 ucSpeakerVolume;            /* 5 */
    Uint8 ucMicrophoneVolume;         /* 6 */
    Uint8 ucAudioEnableBits;          /* 7 */
    Uint8 ucMicLightMode;             /* 8 */
    Uint8 ucAudioMuteBits;            /* 9 */
    Uint8 rgucRightTriggerEffect[11]; /* 10 */
    Uint8 rgucLeftTriggerEffect[11];  /* 21 */
    Uint8 rgucUnknown1[6];            /* 32 */
    Uint8 ucLedFlags;                 /* 38 */
    Uint8 rgucUnknown2[2];            /* 39 */
    Uint8 ucLedAnim;                  /* 41 */
    Uint8 ucLedBrightness;            /* 42 */
    Uint8 ucPadLights;                /* 43 */
    Uint8 ucLedRed;                   /* 44 */
    Uint8 ucLedGreen;                 /* 45 */
    Uint8 ucLedBlue;                  /* 46 */
} DS5EffectsState_t;

static void CyclePS5TriggerEffect(void)
{
    DS5EffectsState_t state;

    Uint8 effects[3][11] = {
        /* Clear trigger effect */
        {0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        /* Constant resistance across entire trigger pull */
        {0x01, 0, 110, 0, 0, 0, 0, 0, 0, 0, 0},
        /* Resistance and vibration when trigger is pulled */
        {0x06, 15, 63, 128, 0, 0, 0, 0, 0, 0, 0},
    };

    trigger_effect = (trigger_effect + 1) % SDL_arraysize(effects);

    SDL_zero(state);
    state.ucEnableBits1 |= (0x04 | 0x08); /* Modify right and left trigger effect respectively */
    SDL_memcpy(state.rgucRightTriggerEffect, effects[trigger_effect], sizeof(effects[trigger_effect]));
    SDL_memcpy(state.rgucLeftTriggerEffect, effects[trigger_effect], sizeof(effects[trigger_effect]));
    SDL_GameControllerSendEffect(gamecontroller, &state, sizeof(state));
}

static SDL_bool ShowingFront(void)
{
    SDL_bool showing_front = SDL_TRUE;
    int i;

    if (gamecontroller)
    {
        /* Show the back of the controller if the paddles are being held */
        for (i = SDL_CONTROLLER_BUTTON_A; i <= SDL_CONTROLLER_BUTTON_PADDLE4; ++i)
        {
            if (SDL_GameControllerGetButton(gamecontroller, (SDL_GameControllerButton)i) == SDL_PRESSED)
            {
                showing_front = SDL_FALSE;
                break;
            }
        }
    }
    if ((SDL_GetModState() & KMOD_SHIFT) != 0)
    {
        showing_front = SDL_FALSE;
    }
    return showing_front;
}

static void SDLCALL VirtualControllerSetPlayerIndex(void *userdata, int player_index)
{
    SDL_Log("Virtual Controller: player index set to %d\n", player_index);
}

static int SDLCALL VirtualControllerRumble(void *userdata, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    SDL_Log("Virtual Controller: rumble set to %d/%d\n", low_frequency_rumble, high_frequency_rumble);
    return 0;
}

static int SDLCALL VirtualControllerRumbleTriggers(void *userdata, Uint16 left_rumble, Uint16 right_rumble)
{
    SDL_Log("Virtual Controller: trigger rumble set to %d/%d\n", left_rumble, right_rumble);
    return 0;
}

static int SDLCALL VirtualControllerSetLED(void *userdata, Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_Log("Virtual Controller: LED set to RGB %d,%d,%d\n", red, green, blue);
    return 0;
}

static void OpenVirtualController(void)
{
    SDL_VirtualJoystickDesc desc;
    int virtual_index;

    SDL_zero(desc);
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.SetPlayerIndex = VirtualControllerSetPlayerIndex;
    desc.Rumble = VirtualControllerRumble;
    desc.RumbleTriggers = VirtualControllerRumbleTriggers;
    desc.SetLED = VirtualControllerSetLED;

    virtual_index = SDL_JoystickAttachVirtualEx(&desc);
    if (virtual_index < 0)
    {
        SDL_Log("Couldn't open virtual device: %s\n", SDL_GetError());
    }
    else
    {
        virtual_joystick = SDL_JoystickOpen(virtual_index);
        if (!virtual_joystick)
        {
            SDL_Log("Couldn't open virtual device: %s\n", SDL_GetError());
        }
    }
}

static void CloseVirtualController(void)
{
    int i;

    for (i = SDL_NumJoysticks(); i--;)
    {
        if (SDL_JoystickIsVirtual(i))
        {
            SDL_JoystickDetachVirtual(i);
        }
    }

    if (virtual_joystick)
    {
        SDL_JoystickClose(virtual_joystick);
        virtual_joystick = NULL;
    }
}

static SDL_GameControllerButton FindButtonAtPosition(int x, int y)
{
    SDL_Point point;
    int i;
    SDL_bool showing_front = ShowingFront();

    point.x = x;
    point.y = y;
    for (i = 0; i < SDL_CONTROLLER_BUTTON_TOUCHPAD; ++i)
    {
        SDL_bool on_front = (i < SDL_CONTROLLER_BUTTON_PADDLE1 || i > SDL_CONTROLLER_BUTTON_PADDLE4);
        if (on_front == showing_front)
        {
            SDL_Rect rect;
            rect.x = button_positions[i].x;
            rect.y = button_positions[i].y;
            rect.w = BUTTON_SIZE;
            rect.h = BUTTON_SIZE;
            if (SDL_PointInRect(&point, &rect))
            {
                return (SDL_GameControllerButton)i;
            }
        }
    }
    return SDL_CONTROLLER_BUTTON_INVALID;
}

static SDL_GameControllerAxis FindAxisAtPosition(int x, int y)
{
    SDL_Point point;
    int i;
    SDL_bool showing_front = ShowingFront();

    point.x = x;
    point.y = y;
    for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
    {
        if (showing_front)
        {
            SDL_Rect rect;
            rect.x = axis_positions[i].x;
            rect.y = axis_positions[i].y;
            rect.w = AXIS_SIZE;
            rect.h = AXIS_SIZE;
            if (SDL_PointInRect(&point, &rect))
            {
                return (SDL_GameControllerAxis)i;
            }
        }
    }
    return SDL_CONTROLLER_AXIS_INVALID;
}

static void VirtualControllerMouseMotion(int x, int y)
{
    if (virtual_button_active != SDL_CONTROLLER_BUTTON_INVALID)
    {
        if (virtual_axis_active != SDL_CONTROLLER_AXIS_INVALID)
        {
            const int MOVING_DISTANCE = 2;
            if (SDL_abs(x - virtual_axis_start_x) >= MOVING_DISTANCE ||
                SDL_abs(y - virtual_axis_start_y) >= MOVING_DISTANCE)
            {
                SDL_JoystickSetVirtualButton(virtual_joystick, virtual_button_active, SDL_RELEASED);
                virtual_button_active = SDL_CONTROLLER_BUTTON_INVALID;
            }
        }
    }

    if (virtual_axis_active != SDL_CONTROLLER_AXIS_INVALID)
    {
        if (virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
            virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        {
            int range = (SDL_JOYSTICK_AXIS_MAX - SDL_JOYSTICK_AXIS_MIN);
            float distance = SDL_clamp(((float)y - virtual_axis_start_y) / AXIS_SIZE, 0.0f, 1.0f);
            Sint16 value = (Sint16)(SDL_JOYSTICK_AXIS_MIN + (distance * range));
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, value);
        }
        else
        {
            float distanceX = SDL_clamp(((float)x - virtual_axis_start_x) / AXIS_SIZE, -1.0f, 1.0f);
            float distanceY = SDL_clamp(((float)y - virtual_axis_start_y) / AXIS_SIZE, -1.0f, 1.0f);
            Sint16 valueX, valueY;

            if (distanceX >= 0)
            {
                valueX = (Sint16)(distanceX * SDL_JOYSTICK_AXIS_MAX);
            }
            else
            {
                valueX = (Sint16)(distanceX * -SDL_JOYSTICK_AXIS_MIN);
            }
            if (distanceY >= 0)
            {
                valueY = (Sint16)(distanceY * SDL_JOYSTICK_AXIS_MAX);
            }
            else
            {
                valueY = (Sint16)(distanceY * -SDL_JOYSTICK_AXIS_MIN);
            }
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, valueX);
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active + 1, valueY);
        }
    }
}

static void VirtualControllerMouseDown(int x, int y)
{
    SDL_GameControllerButton button;
    SDL_GameControllerAxis axis;

    button = FindButtonAtPosition(x, y);
    if (button != SDL_CONTROLLER_BUTTON_INVALID)
    {
        virtual_button_active = button;
        SDL_JoystickSetVirtualButton(virtual_joystick, virtual_button_active, SDL_PRESSED);
    }

    axis = FindAxisAtPosition(x, y);
    if (axis != SDL_CONTROLLER_AXIS_INVALID)
    {
        virtual_axis_active = axis;
        virtual_axis_start_x = x;
        virtual_axis_start_y = y;
    }
}

static void VirtualControllerMouseUp(int x, int y)
{
    if (virtual_button_active != SDL_CONTROLLER_BUTTON_INVALID)
    {
        SDL_JoystickSetVirtualButton(virtual_joystick, virtual_button_active, SDL_RELEASED);
        virtual_button_active = SDL_CONTROLLER_BUTTON_INVALID;
    }

    if (virtual_axis_active != SDL_CONTROLLER_AXIS_INVALID)
    {
        if (virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
            virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        {
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, SDL_JOYSTICK_AXIS_MIN);
        }
        else
        {
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, 0);
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active + 1, 0);
        }
        virtual_axis_active = SDL_CONTROLLER_AXIS_INVALID;
    }
}

static void showSensorData(SDL_GameController *gamepad)
{
    SDL_bool has_accel, has_gyro, has_lpad, has_rpad;
    has_accel = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_ACCEL);
    has_gyro = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_GYRO);
    /*has_lpad = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_LPAD);
    has_rpad = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_RPAD);*/
    has_lpad = false;
    has_rpad = false;
    if (!(has_accel || has_gyro || has_lpad || has_rpad))
    {
        return;
    }

    // check for update time
    const int SENSOR_UPDATE_INTERVAL_MS = 100;
    static Uint64 last_sensor_update = -1;
    Uint64 now = SDL_GetTicks();
    if (now < last_sensor_update + SENSOR_UPDATE_INTERVAL_MS)
    {
        return;
    }
    last_sensor_update = now;

    // get data
    float accel_data[3] = {0};
    float gyro_data[3] = {0};
    if (has_accel)
    {
        SDL_GameControllerGetSensorData(gamepad, SDL_SENSOR_ACCEL, accel_data, SDL_arraysize(accel_data));
    }
    if (has_gyro)
    {
        SDL_GameControllerGetSensorData(gamepad, SDL_SENSOR_GYRO, gyro_data, SDL_arraysize(gyro_data));
    }

    float lpad_data[3] = {0};
    float rpad_data[3] = {0};
    if (has_lpad)
    {
        // SDL_GameControllerGetSensorData(gamepad, SDL_SENSOR_LPAD, lpad_data, SDL_arraysize(lpad_data));
    }
    if (has_rpad)
    {
        // SDL_GameControllerGetSensorData(gamepad, SDL_SENSOR_RPAD, rpad_data, SDL_arraysize(rpad_data));
    }

    // format message
    char text1[512] = {0};
    if (has_accel)
    {
        sprintf(text1,
                "Accel:(%.2f,%.2f,%.2f)\t",
                accel_data[0],
                accel_data[1],
                accel_data[2]);
    }

    char text2[512] = {0};
    if (has_gyro)
    {
        sprintf(text2, "Gyro:(%.2f,%.2f,%.2f)\t",
                gyro_data[0],
                gyro_data[1],
                gyro_data[2]);
    }

    char text3[512] = {0};
    if (has_lpad)
    {
        sprintf(text3, "Lpad:(%.2f,%.2f,%.2f)\t",
                lpad_data[0],
                lpad_data[1],
                lpad_data[2]);
    }

    char text4[512] = {0};
    if (has_rpad)
    {
        sprintf(text4, "Rpad:(%.2f,%.2f,%.2f)\t",
                rpad_data[0],
                rpad_data[1],
                rpad_data[2]);
    }
    char text[2048] = {0};
    sprintf(text, "%s%s%s%s\r\n",
            text1,
            text2,
            text3,
            text4);
    SDL_Log(text);
}

void showExtraButtons()
{
    if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_PADDLE1))
    {
        SDL_Log("PADDLE1\r\n");
    }
    if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_PADDLE2))
    {
        SDL_Log("PADDLE2\r\n");
    }
    if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_PADDLE3))
    {
        SDL_Log("PADDLE3\r\n");
    }
    if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_PADDLE4))
    {
        SDL_Log("PADDLE4\r\n");
    }
    if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_TOUCHPAD))
    {
        SDL_Log("LTOUCH\r\n");
    }
    /*if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_RTOUCHPAD)) {
        SDL_Log("RTOUCH\r\n");
    }*/
    if (SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller), SDL_CONTROLLER_BUTTON_MISC1))
    {
        SDL_Log("MISC1\r\n");
    }
}

void letsRumble()
{
    /* Update rumble based on trigger state */
    {
        Sint16 left = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        Sint16 right = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        Uint16 low_frequency_rumble = ConvertAxisToRumble(left);
        Uint16 high_frequency_rumble = ConvertAxisToRumble(right);
        SDL_GameControllerRumble(gamecontroller, low_frequency_rumble, high_frequency_rumble, 250);
    }

    /* Update trigger rumble based on thumbstick state */
    {
        Sint16 left = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_LEFTY);
        Sint16 right = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_RIGHTY);
        Uint16 left_rumble = ConvertAxisToRumble(~left);
        Uint16 right_rumble = ConvertAxisToRumble(~right);

        SDL_GameControllerRumbleTriggers(gamecontroller, left_rumble, right_rumble, 250);
    }
}
/// <summary>
/// return true when exit requested
/// </summary>
/// <returns></returns>
bool handlelGamecontrollerEvents()
{
    bool done = false;

    SDL_Event event;
    int i;

    /* Update to get the current event state */
    SDL_PumpEvents();

    /* Process all currently pending events */
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1)
    {
        switch (event.type)
        {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_Log("Game controller device %d added.\n", (int)SDL_JoystickGetDeviceInstanceID(event.cdevice.which));
            AddController(event.cdevice.which, SDL_TRUE);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            SDL_Log("Game controller device %d removed.\n", (int)event.cdevice.which);
            DelController(event.cdevice.which);
            break;

        case SDL_CONTROLLERTOUCHPADDOWN:
        case SDL_CONTROLLERTOUCHPADMOTION:
        case SDL_CONTROLLERTOUCHPADUP:
            SDL_Log("Controller %" SDL_PRIs32 " touchpad %" SDL_PRIs32 " finger %" SDL_PRIs32 " %s %.2f, %.2f, %.2f\n",
                    event.ctouchpad.which,
                    event.ctouchpad.touchpad,
                    event.ctouchpad.finger,
                    (event.type == SDL_CONTROLLERTOUCHPADDOWN ? "pressed at" : (event.type == SDL_CONTROLLERTOUCHPADUP ? "released at" : "moved to")),
                    event.ctouchpad.x,
                    event.ctouchpad.y,
                    event.ctouchpad.pressure);
            break;

// #define VERBOSE_SENSORS
#ifdef VERBOSE_SENSORS
        case SDL_CONTROLLERSENSORUPDATE:
            if (!(((SDL_SensorType)event.csensor.sensor == SDL_SENSOR_GYRO)) ||
                ((SDL_SensorType)event.csensor.sensor == SDL_SENSOR_GYRO_L) ||
                ((SDL_SensorType)event.csensor.sensor == SDL_SENSOR_GYRO_R))
            {
                break;
            }
            SDL_Log("Controller %" SDL_PRIs32 " sensor %s: %.2f, %.2f, %.2f (%" SDL_PRIu64 ")\n",
                    event.csensor.which,
                    GetSensorName((SDL_SensorType)event.csensor.sensor),
                    event.csensor.data[0],
                    event.csensor.data[1],
                    event.csensor.data[2],
                    event.csensor.timestamp_us);
            break;
#endif /* VERBOSE_SENSORS */

#define VERBOSE_AXES
#ifdef VERBOSE_AXES
        case SDL_CONTROLLERAXISMOTION:
            if (event.caxis.value <= (-SDL_JOYSTICK_AXIS_MAX / 2) || event.caxis.value >= (SDL_JOYSTICK_AXIS_MAX / 2))
            {
                SetController(event.caxis.which);
            }
            /* SDL_Log("Controller %" SDL_PRIs32 " axis %s changed to %d\n", event.caxis.which, SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis), event.caxis.value);*/
            break;
#endif /* VERBOSE_AXES */

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            if (event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                SetController(event.cbutton.which);
            }
            /*SDL_Log("Controller %" SDL_PRIs32 " button %s %s\n", event.cbutton.which, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button),
                    event.cbutton.state ? "pressed" : "released");*/

            /* Cycle PS5 trigger effects when the microphone button is pressed */
            if (event.type == SDL_CONTROLLERBUTTONDOWN &&
                event.cbutton.button == SDL_CONTROLLER_BUTTON_MISC1 &&
                SDL_GameControllerGetType(gamecontroller) == SDL_CONTROLLER_TYPE_PS5)
            {
                CyclePS5TriggerEffect();
            }
            break;

        case SDL_JOYBATTERYUPDATED:
            SDL_Log("Controller %" SDL_PRIs32 " battery state changed to %s\n", event.jbattery.which, power_level_strings[event.jbattery.level + 1]);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (virtual_joystick)
            {
                VirtualControllerMouseDown(event.button.x, event.button.y);
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (virtual_joystick)
            {
                VirtualControllerMouseUp(event.button.x, event.button.y);
            }
            break;

        case SDL_MOUSEMOTION:
            if (virtual_joystick)
            {
                VirtualControllerMouseMotion(event.motion.x, event.motion.y);
            }
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)
            {
                if (gamecontroller)
                {
                    int player_index = (event.key.keysym.sym - SDLK_0);

                    SDL_GameControllerSetPlayerIndex(gamecontroller, player_index);
                }
                break;
            }
            if (event.key.keysym.sym == SDLK_a)
            {
                OpenVirtualController();
                break;
            }
            if (event.key.keysym.sym == SDLK_d)
            {
                CloseVirtualController();
                break;
            }
            if (event.key.keysym.sym != SDLK_ESCAPE)
            {
                break;
            }
            SDL_FALLTHROUGH;
        case SDL_QUIT:
            done = true;
            break;
        default:
            break;
        }
    }
    return done;
}

int demoGameController()
{
    if (!initializeGamecontroller())
    {
        return 1;
    }

    /* Print information about the mappings */
    // if (argv[1] && SDL_strcmp(argv[1], "--mappings") == 0) {
    /*SDL_Log("Supported mappings:\n");
    for (i = 0; i < SDL_GameControllerNumMappings(); ++i) {
        char *mapping = SDL_GameControllerMappingForIndex(i);
        if (mapping) {
            SDL_Log("\t%s\n", mapping);
            SDL_free(mapping);
        }
    }
    SDL_Log("\n");*/
    //}

    /* Print information about the controller */
    /*  for (i = 0; i < SDL_NumJoysticks(); ++i) {
          const char *name;
          const char *path;
          const char *description;

          SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i),
                                    guid, sizeof(guid));

          if (SDL_IsGameController(i)) {
              controller_count++;
              name = SDL_GameControllerNameForIndex(i);
              path = SDL_GameControllerPathForIndex(i);
              switch (SDL_GameControllerTypeForIndex(i)) {
              case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
                  description = "Amazon Luna Controller";
                  break;
              case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
                  description = "Google Stadia Controller";
                  break;
              case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
              case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
              case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
                  description = "Nintendo Switch Joy-Con";
                  break;
              case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
                  description = "Nintendo Switch Pro Controller";
                  break;
              case SDL_CONTROLLER_TYPE_PS3:
                  description = "PS3 Controller";
                  break;
              case SDL_CONTROLLER_TYPE_PS4:
                  description = "PS4 Controller";
                  break;
              case SDL_CONTROLLER_TYPE_PS5:
                  description = "PS5 Controller";
                  break;
              case SDL_CONTROLLER_TYPE_XBOX360:
                  description = "XBox 360 Controller";
                  break;
              case SDL_CONTROLLER_TYPE_XBOXONE:
                  description = "XBox One Controller";
                  break;
              case SDL_CONTROLLER_TYPE_VIRTUAL:
                  description = "Virtual Game Controller";
                  break;
              default:
                  description = "Game Controller";
                  break;
              }
              AddController(i, SDL_FALSE);
          } else {
              name = SDL_JoystickNameForIndex(i);
              path = SDL_JoystickPathForIndex(i);
              description = "Joystick";
          }
          SDL_Log("%s %d: %s%s%s (guid %s, VID 0x%.4x, PID 0x%.4x, player index = %d)\n",
                  description, i, name ? name : "Unknown", path ? ", " : "", path ? path : "", guid,
                  SDL_JoystickGetDeviceVendor(i),
                  SDL_JoystickGetDeviceProduct(i),
                  SDL_JoystickGetDevicePlayerIndex(i));
      }
      SDL_Log("There are %d game controller(s) attached (%d joystick(s))\n", controller_count, SDL_NumJoysticks());

      if (controller_index < num_controllers) {
          gamecontroller = gamecontrollers[controller_index];
      } else {
          gamecontroller = NULL;
      }*/

    /* Loop, getting controller events! */
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(loop, NULL, 0, 1);
#else
    bool done = false;
    while (!(done = handlelGamecontrollerEvents()))
    {
        showExtraButtons();

        showSensorData(gamecontroller);

        SDL_bool showing = ShowingFront();

        if (gamecontroller)
        {
            if (trigger_effect == 0)
            {
                letsRumble();
            }
        }
        SDL_Delay(16);
    }
#endif

    closeGamecontroller();

    return 0;
}

bool joystickGetButton(int button)
{
    return (bool)SDL_JoystickGetButton(SDL_GameControllerGetJoystick(gamecontroller),
                                       button);
}

bool gameControllerGetButton(int button)
{
    return (bool)SDL_GameControllerGetButton(gamecontroller,
                                             (SDL_GameControllerButton)button);
}

bool getAnalogInputs(float channels[])
{

    bool joyConnected = gamecontroller != nullptr;
    if (!joyConnected)
    {
        return joyConnected;
    }
    channels[SDL_CONTROLLER_AXIS_LEFTX] = (float)SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_LEFTX) / (float)0x7fff;
    channels[SDL_CONTROLLER_AXIS_LEFTY] = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_LEFTY) / (float)0x7fff;
    channels[SDL_CONTROLLER_AXIS_RIGHTX] = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_RIGHTX) / (float)0x7fff;
    channels[SDL_CONTROLLER_AXIS_RIGHTY] = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_RIGHTY) / (float)0x7fff;
    channels[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / (float)0x7fff;
    channels[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / (float)0x7fff;
    return joyConnected;
}
void closeGamecontroller()
{
    /* Reset trigger state */
    if (trigger_effect != 0)
    {
        trigger_effect = -1;
        CyclePS5TriggerEffect();
    }

    CloseVirtualController();
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
}

bool initializeGamecontroller()
{
    SDL_version ver;
    SDL_GetVersion(&ver);
    printf("SDL version %d.%d.%d", ver.major, ver.minor, ver.patch);

    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ROG_CHAKRAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_LINUX_JOYSTICK_DEADZONES, "1");

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    /* Initialize SDL (Note: video is required to start event loop) */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_SENSOR | /* SDL_INIT_JOYSTICK |*/ SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER /*SDL_INIT_EVERYTHING*/) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
    return true;
}

//#else
//
//int main(int argc, char *argv[])
//{
//    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL compiled without Joystick support.\n");
//    return 1;
//}
//
//#endif

/* vi: set ts=4 sw=4 expandtab: */
