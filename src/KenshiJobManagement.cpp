// SPDX-License-Identifier: GPL-3.0-only
// Kenshi Job Management, 0.1.0-alpha queue field test.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Character.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/util/hand.h>

#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ListBox.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Window.h>

#include <sstream>
#include <string>
#include <vector>

namespace
{
    const char* const PLUGIN_NAME = "Kenshi Job Management";
    const char* const PLUGIN_VERSION = "0.1.0-alpha";
    const DWORD REFRESH_INTERVAL_MS = 250;
    const DWORD CLEAR_CONFIRMATION_MS = 4000;
    const DWORD RESET_HOTKEY_COOLDOWN_MS = 1000;
    const int MAX_SAFE_JOB_ROWS = 512;

    typedef void (*PlayerInterfaceUpdateFunction)(PlayerInterface*);
    typedef void (*PlayerInterfaceClearAndResetFunction)(PlayerInterface*);

    PlayerInterfaceUpdateFunction g_playerInterfaceUpdateOriginal = NULL;
    PlayerInterfaceClearAndResetFunction g_playerInterfaceClearAndResetOriginal = NULL;

    volatile LONG g_started = 0;
    PlayerInterface* g_playerInterface = NULL;

    MyGUI::Window* g_window = NULL;
    MyGUI::TextBox* g_characterText = NULL;
    MyGUI::TextBox* g_statusText = NULL;
    MyGUI::ListBox* g_jobList = NULL;
    MyGUI::Button* g_jobsToggleButton = NULL;
    MyGUI::Button* g_moveUpButton = NULL;
    MyGUI::Button* g_moveDownButton = NULL;
    MyGUI::Button* g_removeButton = NULL;
    MyGUI::Button* g_clearButton = NULL;
    MyGUI::Button* g_refreshButton = NULL;
    MyGUI::Button* g_closeButton = NULL;

    struct HandleIdentity
    {
        bool valid;
        itemType type;
        unsigned int container;
        unsigned int containerSerial;
        unsigned int index;
        unsigned int serial;
    };

    HandleIdentity g_displayedCharacter = {
        false, static_cast<itemType>(0), 0, 0, 0, 0
    };
    DWORD g_lastRefreshTick = 0;
    DWORD g_lastResetTick = 0;
    bool g_hotkeyWasDown = false;
    bool g_clearArmed = false;
    DWORD g_clearArmedTick = 0;
    bool g_refreshInProgress = false;
    bool g_enabled = false;

    struct JobRowSnapshot
    {
        TaskType taskType;
        std::string name;
        const Tasker* taskData;
    };

    std::vector<JobRowSnapshot> g_rows;


#include "RuntimeAccess.inl"
#include "JobView.inl"
#include "JobActions.inl"
#include "JobWindow.inl"
#include "Hooks.inl"
}

__declspec(dllexport) void startPlugin()
{
    if (InterlockedCompareExchange(&g_started, 1, 0) != 0)
    {
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&PlayerInterface::updateUT),
            &PlayerInterfaceUpdateHook,
            &g_playerInterfaceUpdateOriginal))
    {
        ErrorLog("[KenshiJobManagement] Failed to hook PlayerInterface::updateUT.");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&PlayerInterface::clearAndReset),
            &PlayerInterfaceClearAndResetHook,
            &g_playerInterfaceClearAndResetOriginal))
    {
        ErrorLog("[KenshiJobManagement] Failed to hook PlayerInterface::clearAndReset.");
        return;
    }

    g_enabled = true;

    std::ostringstream message;
    message << "[KenshiJobManagement] "
            << PLUGIN_NAME
            << " "
            << PLUGIN_VERSION
            << " loaded. Press Ctrl+J to open the queue editor.";
    LogInfo(message.str());
}
