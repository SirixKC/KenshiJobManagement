// SPDX-License-Identifier: GPL-3.0-only
// Kenshi Job Management, 0.1.0-alpha squad queue editor.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/AI/AITaskSystem.h>
// KenshiLib 0.4.0 declares BuildingDesignation independently in Building.h
// and Platoon.h. Rename the Building.h copy locally so the generated headers
// can coexist in this single translation unit. This plug-in does not use that
// designation type.
#define BuildingDesignation KJM_BuildingHeaderDesignation
#define BD_NONE KJM_BUILDING_BD_NONE
#define BD_SHOP KJM_BUILDING_BD_SHOP
#define BD_BARRACKS KJM_BUILDING_BD_BARRACKS
#define BD_BAR KJM_BUILDING_BD_BAR
#define BD_HOSPITAL KJM_BUILDING_BD_HOSPITAL
#define BD_ARMOURY KJM_BUILDING_BD_ARMOURY
#define BD_TREASURE KJM_BUILDING_BD_TREASURE
#define BD_PRISON KJM_BUILDING_BD_PRISON
#define BD_HQ KJM_BUILDING_BD_HQ
#define BD_RESIDENTIAL KJM_BUILDING_BD_RESIDENTIAL
#define BD_SLAVE_STORAGE KJM_BUILDING_BD_SLAVE_STORAGE
#define BD_RESIDENTIAL_SMALL KJM_BUILDING_BD_RESIDENTIAL_SMALL
#include <kenshi/Building/Building.h>
#undef BuildingDesignation
#undef BD_NONE
#undef BD_SHOP
#undef BD_BARRACKS
#undef BD_BAR
#undef BD_HOSPITAL
#undef BD_ARMOURY
#undef BD_TREASURE
#undef BD_PRISON
#undef BD_HQ
#undef BD_RESIDENTIAL
#undef BD_SLAVE_STORAGE
#undef BD_RESIDENTIAL_SMALL
#include <kenshi/CharStats.h>
#include <kenshi/Character.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/InputHandler.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/Platoon.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObject.h>
#include <kenshi/RootObjectBase.h>
#include <kenshi/Town.h>
#include <kenshi/gui/OrdersPanel.h>
#include <kenshi/gui/PortraitManager.h>
#include <kenshi/gui/SquadManagementScreen.h>
#include <kenshi/util/hand.h>

#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ScrollBar.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_TextIterator.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_Window.h>

// KenshiLib's generated UseableStuff header depends on InventoryGUI and must
// follow the normal MyGUI declarations in this translation unit.
#include <kenshi/gui/InventoryGUI.h>
#include <kenshi/Building/UseableStuff.h>

#include <ogre/OgreResourceGroupManager.h>
#include <ogre/OgreTextureManager.h>

#include <algorithm>
#include <cmath>
#ifdef KJM_SCANNER_PROBE
#include <stddef.h>
#endif
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    const char* const PLUGIN_NAME = "Kenshi Job Management";
    const char* const PLUGIN_VERSION = "0.1.0-alpha";
    const DWORD REFRESH_INTERVAL_MS = 1000;
    const DWORD RESET_HOTKEY_COOLDOWN_MS = 1000;
    const int MAX_SAFE_JOB_ROWS = 64;
    const int PAD = 12;
    const int TOP_HEIGHT = 46;
    const int HEADER_HEIGHT = 38;
    const int ACTION_HEIGHT = 62;
    const int SCROLL_SIZE = 20;
    // Keep the member rows compact enough to show a full squad without
    // wasting vertical space.  Card dimensions are intentionally modest:
    // the original tick-button skin was being stretched over a very large
    // 220x112 surface and its checkbox texture read like a distorted icon.
    // Member rows need room for three readable, vertically stacked skills.
    // Job cards keep their compact 88px height and are centered in the row.
    const int ROW_HEIGHT = 116;
    const int ROW_GAP = 6;
    const int ROW_STRIDE = ROW_HEIGHT + ROW_GAP;
    const int CARD_HEIGHT = 88;
    const int CARD_TOP = (ROW_HEIGHT - CARD_HEIGHT) / 2;
    // Use the spare height in each member row for a larger portrait.  Keep
    // the same geometry in the Squad and Stations rosters so the two tabs
    // remain visually consistent.
    const int MEMBER_PORTRAIT_SIZE = 80;
    const int MEMBER_PORTRAIT_INSET = 2;
    const int MEMBER_TEXT_LEFT = 94;
    // Keep the priority columns narrow around the actual three-line caption.
    // The previous 196px card left conspicuous empty space on both sides of
    // ordinary work and target names.  A 168px card still leaves room for
    // the fitter below to wrap or reduce unusually long labels.
    const int CARD_WIDTH = 168;
    const int CARD_GAP = 4;
    const int CARD_STRIDE = CARD_WIDTH + CARD_GAP;
    const int DRAG_THRESHOLD = 5;
    const int DRAG_EDGE = 32;

    typedef void (*PlayerInterfaceUpdateFunction)(PlayerInterface*);
    typedef void (*PlayerInterfaceClearAndResetFunction)(PlayerInterface*);
    typedef void (*PlayerInterfaceCycleSquadFunction)(PlayerInterface*);

    PlayerInterfaceUpdateFunction g_playerInterfaceUpdateOriginal = NULL;
    PlayerInterfaceClearAndResetFunction g_playerInterfaceClearAndResetOriginal = NULL;
    PlayerInterfaceCycleSquadFunction g_playerInterfaceCycleSquadOriginal = NULL;

    volatile LONG g_started = 0;
    PlayerInterface* g_playerInterface = NULL;
    bool g_enabled = false;

    struct HandleIdentity
    {
        bool valid;
        itemType type;
        unsigned int container;
        unsigned int containerSerial;
        unsigned int index;
        unsigned int serial;

        HandleIdentity() :
            valid(false), type(static_cast<itemType>(0)), container(0),
            containerSerial(0), index(0), serial(0)
        {
        }
    };

    struct SkillValue
    {
        StatsEnumerated stat;
        std::string name;
        float sortValue;
        int value;

        SkillValue() : stat(STAT_NONE), sortValue(0.0f), value(0) {}
    };

    struct JobRowSnapshot
    {
        TaskType taskType;
        std::string jobLabel;
        std::string targetLabel;
        bool hasTarget;
        bool targetAvailable;
        ULONG_PTR taskToken;
        HandleIdentity target;

        JobRowSnapshot() :
            taskType(static_cast<TaskType>(0)), hasTarget(false),
            targetAvailable(false), taskToken(0)
        {
        }
    };

    struct MemberSnapshot
    {
        HandleIdentity identity;
        hand handle;
        std::string name;
        std::string condition;
        bool loaded;
        bool queueAvailable;
        bool jobsEnabled;
        bool truncated;
        unsigned int revision;
        std::vector<SkillValue> skills;
        std::vector<JobRowSnapshot> jobs;

        MemberSnapshot() :
            loaded(false), queueAvailable(false), jobsEnabled(false),
            truncated(false), revision(1)
        {
        }
    };

    struct SquadSnapshot
    {
        HandleIdentity identity;
        hand handle;
        std::string name;
        bool live;
        bool unavailable;
        std::vector<MemberSnapshot> members;

        SquadSnapshot() : live(false), unavailable(false) {}
    };

    struct SquadCache
    {
        HandleIdentity identity;
        std::string name;
        std::vector<MemberSnapshot> members;
        int horizontalOffset;
        int verticalOffset;

        SquadCache() : horizontalOffset(0), verticalOffset(0) {}
    };

    struct SelectedJob
    {
        HandleIdentity member;
        JobRowSnapshot job;
        int lastSlot;

        SelectedJob() : lastSlot(-1) {}
    };

    struct CardWidgets
    {
        MyGUI::Button* card;
        MyGUI::Widget* unavailableMarker;
        MyGUI::Widget* selectionMarker;
        MyGUI::Widget* highlightTop;
        MyGUI::Widget* highlightBottom;
        MyGUI::Widget* highlightLeft;
        MyGUI::Widget* highlightRight;
        MyGUI::ImageBox* categoryIcon;
        MyGUI::Widget* categoryOverlay;
        MyGUI::TextBox* job;
        MyGUI::TextBox* arrow;
        MyGUI::TextBox* target;
        int memberIndex;
        int slot;
        int highlightGroup;

        CardWidgets() :
            card(NULL), unavailableMarker(NULL), selectionMarker(NULL),
            highlightTop(NULL), highlightBottom(NULL), highlightLeft(NULL),
            highlightRight(NULL), categoryIcon(NULL), categoryOverlay(NULL),
            job(NULL), arrow(NULL), target(NULL),
            memberIndex(-1), slot(-1), highlightGroup(-1)
        {
        }
    };

    struct JobHighlightKey
    {
        TaskType taskType;
        bool hasTarget;
        HandleIdentity target;

        JobHighlightKey() :
            taskType(static_cast<TaskType>(0)), hasTarget(false)
        {
        }
    };

    struct MemberWidgets
    {
        MyGUI::Widget* memberRoot;
        MyGUI::Widget* jobsRoot;
        MyGUI::Button* portraitBorder;
        MyGUI::ImageBox* portraitBackground;
        MyGUI::ImageBox* portrait;
        MyGUI::ImageBox* portraitBackOverlay;
        MyGUI::ImageBox* portraitFrontOverlay;
        MyGUI::TextBox* name;
        MyGUI::TextBox* condition;
        MyGUI::TextBox* skills;
        MyGUI::Button* jobsToggle;
        MyGUI::Button* clearButton;
        MyGUI::TextBox* emptyJobs;
        std::vector<CardWidgets> cards;
        bool portraitBound;
        unsigned int appliedRevision;

        MemberWidgets() :
            memberRoot(NULL), jobsRoot(NULL), portraitBorder(NULL),
            portraitBackground(NULL), portrait(NULL), portraitBackOverlay(NULL),
            portraitFrontOverlay(NULL), name(NULL), condition(NULL), skills(NULL),
            jobsToggle(NULL), clearButton(NULL), emptyJobs(NULL),
            portraitBound(false), appliedRevision(0)
        {
        }
    };

    enum DragKind
    {
        DRAG_NONE,
        DRAG_REORDER,
        DRAG_REMOVE_ONLY
    };

    struct DragState
    {
        DragKind kind;
        bool armed;
        bool active;
        MyGUI::IntPoint pressPoint;
        int memberIndex;
        int slot;
        int insertionGap;
        bool deferredPlainClick;
        bool sourceWasSelected;
        std::vector<JobRowSnapshot> startSequence;

        DragState() :
            kind(DRAG_NONE), armed(false), active(false),
            pressPoint(0, 0), memberIndex(-1), slot(-1), insertionGap(-1),
            deferredPlainClick(false), sourceWasSelected(false)
        {
        }
    };

    enum ModalKind
    {
        MODAL_NONE,
        MODAL_CLEAR,
        MODAL_OPTIONS
    };

    struct ModalState
    {
        ModalKind kind;
        MyGUI::Widget* shade;
        MyGUI::Window* window;
        HandleIdentity member;
        std::vector<JobRowSnapshot> reviewedQueue;

        ModalState() : kind(MODAL_NONE), shade(NULL), window(NULL) {}
    };

    enum PendingActionType
    {
        ACTION_NONE,
        ACTION_TOGGLE_JOBS,
        ACTION_REORDER,
        ACTION_REMOVE_SELECTED,
        ACTION_CLEAR_MEMBER,
        ACTION_TRANSFER_STATION_JOB
    };

    struct PendingAction
    {
        PendingActionType type;
        HandleIdentity member;
        HandleIdentity destinationMember;
        HandleIdentity stationTarget;
        JobRowSnapshot job;
        std::vector<JobRowSnapshot> sequence;
        std::vector<JobRowSnapshot> destinationSequence;
        int targetSlot;

        PendingAction() : type(ACTION_NONE), targetSlot(-1) {}
    };

    struct SkillDefinition
    {
        StatsEnumerated stat;
        const char* fallbackName;
        const char* category;
        bool defaultEnabled;
    };

    struct OptionCategoryButton
    {
        MyGUI::Button* button;
        std::string category;

        OptionCategoryButton() : button(NULL) {}
    };

    // Full-screen UI state. g_window is retained as the lifecycle sentinel.
    MyGUI::Window* g_window = NULL;
    MyGUI::Widget* g_squadTabRoot = NULL;
    MyGUI::Button* g_squadTabButton = NULL;
    MyGUI::Button* g_stationTabButton = NULL;
    MyGUI::Widget* g_memberViewport = NULL;
    MyGUI::Widget* g_memberCanvas = NULL;
    MyGUI::Widget* g_jobViewport = NULL;
    MyGUI::Widget* g_jobCanvas = NULL;
    MyGUI::Widget* g_priorityViewport = NULL;
    MyGUI::Widget* g_priorityCanvas = NULL;
    MyGUI::TextBox* g_squadText = NULL;
    MyGUI::TextBox* g_statusText = NULL;
    MyGUI::TextBox* g_emptyText = NULL;
    MyGUI::Button* g_removeButton = NULL;
    MyGUI::Button* g_optionsButton = NULL;
    MyGUI::Button* g_closeButton = NULL;
    MyGUI::ScrollBar* g_verticalScroll = NULL;
    MyGUI::ScrollBar* g_horizontalScroll = NULL;
    MyGUI::Widget* g_insertionLine = NULL;
    MyGUI::Widget* g_tooltip = NULL;
    MyGUI::TextBox* g_tooltipText = NULL;

    int g_memberWidth = 280;
    int g_bodyHeight = 0;
    int g_jobWidth = 0;
    int g_horizontalOffset = 0;
    int g_verticalOffset = 0;
    int g_maxHorizontalOffset = 0;
    int g_maxVerticalOffset = 0;
    bool g_changingScroll = false;

    SquadSnapshot g_squad;
    std::vector<SquadCache> g_squadCaches;
    std::vector<MemberWidgets> g_memberWidgets;
    std::vector<MyGUI::TextBox*> g_priorityLabels;
    std::vector<JobHighlightKey> g_jobHighlightKeys;
    std::vector<SelectedJob> g_selectedJobs;
    HandleIdentity g_selectionAnchorMember;
    int g_selectionAnchorSlot = -1;
    int g_hoveredJobHighlightGroup = -1;
    bool g_jobHighlightCacheValid = false;
    DragState g_drag;
    ModalState g_modal;
    PendingAction g_pendingAction;
    std::vector<MyGUI::Button*> g_optionStatButtons;
    std::vector<OptionCategoryButton> g_optionCategoryButtons;

    bool g_skillEnabled[STAT_END];
    bool g_settingsLoaded = false;
    std::string g_settingsPath;

    DWORD g_lastRefreshTick = 0;
    DWORD g_lastResetTick = 0;
    DWORD g_lastDragTick = 0;
    bool g_hotkeyWasDown = false;
    bool g_escapeWasDown = false;
    bool g_tabWasDown = false;
    bool g_closeRequested = false;
    bool g_modalCloseRequested = false;
    bool g_skillRefreshRequested = false;
    bool g_settingsWriteFailed = false;
    bool g_stationFilterRefreshRequested = false;
    bool g_stationAssignmentsDirty = false;
    bool g_stationProjectionRefreshRequested = false;
    HandleIdentity g_stationProjectionRefreshSource;
    HandleIdentity g_stationProjectionRefreshDestination;
    DWORD g_lastStationAssignmentRefreshAttempt = 0;
    bool g_stationTabActive = false;
    bool g_closeForResume = false;
    bool g_windowModalAdded = false;
    bool g_pauseCaptured = false;
    bool g_wasPaused = false;
    float g_previousSpeed = 1.0f;
    float g_managerPausedSpeed = 0.0f;

    // Cross-file event declarations.
    void OnCardPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnCardDrag(MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnCardReleased(MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnCardMouseSetFocus(MyGUI::Widget*, MyGUI::Widget*);
    void OnCardMouseLostFocus(MyGUI::Widget*, MyGUI::Widget*);
    void OnCardToolTip(MyGUI::Widget*, const MyGUI::ToolTipInfo&);
    void OnEmptyPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnMouseWheel(MyGUI::Widget*, int);
    void OnJobsToggleClicked(MyGUI::Widget*);
    void OnClearClicked(MyGUI::Widget*);
    void OnRemoveClicked(MyGUI::Widget*);
    void OnOptionsClicked(MyGUI::Widget*);
    void OnCloseClicked(MyGUI::Widget*);
    void OnWindowButtonPressed(MyGUI::Window*, const std::string&);
    void OnVerticalScroll(MyGUI::ScrollBar*, size_t);
    void OnHorizontalScroll(MyGUI::ScrollBar*, size_t);
    void OnClearYes(MyGUI::Widget*);
    void OnClearNo(MyGUI::Widget*);
    void OnOptionStatClicked(MyGUI::Widget*);
    void OnOptionCategoryClicked(MyGUI::Widget*);
    void OnOptionSelectAll(MyGUI::Widget*);
    void OnOptionClearAll(MyGUI::Widget*);
    void OnOptionReset(MyGUI::Widget*);
    void OnOptionClose(MyGUI::Widget*);
    void ClearJobHoverHighlight();
    void CancelDrag();

#include "RuntimeAccess.inl"
#include "StationScanner.inl"
#include "StationSettings.inl"
#include "StationAssets.inl"

#ifdef KJM_SCANNER_PROBE
#include "../diagnostics/OwnershipProbe.inl"
#endif

    StationScanState g_stationScan;

#include "JobView.inl"
#include "JobActions.inl"
#include "StationView.inl"
#include "JobWindow.inl"
#include "Hooks.inl"
}

#ifdef KJM_SCANNER_PROBE
extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestStage1()
{
    OwnershipProbeRequestStage(1);
}

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestInspect()
{
    OwnershipProbeRequestStage(2);
}

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestReadHandles()
{
    OwnershipProbeRequestStage(3);
}

extern "C" __declspec(dllexport) LONG KJM_ScannerProbe_GetState()
{
    return OwnershipProbeGetState();
}
#endif

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

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&PlayerInterface::cycleSquad),
            &PlayerInterfaceCycleSquadHook,
            &g_playerInterfaceCycleSquadOriginal))
    {
        ErrorLog("[KenshiJobManagement] Failed to hook PlayerInterface::cycleSquad.");
        return;
    }

    g_enabled = true;

#ifdef KJM_SCANNER_PROBE
    DebugLog(
        "[KJM OwnershipProbe] diagnostic build active; "
        "each Ctrl+Shift+F10 press advances one stage");
#endif

    std::ostringstream message;
    message << "[KenshiJobManagement] " << PLUGIN_NAME << " "
            << PLUGIN_VERSION << " loaded. Press Ctrl+J to open the squad manager.";
    DebugLog(message.str().c_str());
}
