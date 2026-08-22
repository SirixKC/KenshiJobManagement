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
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/MainBarGUI.h>
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
#include <stddef.h>
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
    // Keep failed station-assignment recovery bounded while the Stations tab
    // is active. The paused Squad Jobs board refreshes only at explicit UI or
    // mutation boundaries.
    const DWORD REFRESH_INTERVAL_MS = 1000;
    const DWORD RESET_HOTKEY_COOLDOWN_MS = 1000;
    const DWORD RECENT_MESSAGE_DEDUP_WINDOW_MS = 50;
    const int MAX_SAFE_JOB_ROWS = 64;
    const int PAD = 12;
    const int TOP_HEIGHT = 46;
    const int HEADER_HEIGHT = 38;
    // The status frame now keeps three timestamped message lines. Reserve
    // enough bottom-bar height for that frame and the compact action buttons.
    const int ACTION_HEIGHT = 82;
    const int SCROLL_SIZE = 20;
    const int SQUAD_SELECTOR_HEIGHT = 54;
    const int SQUAD_SELECTOR_LABEL_WIDTH = 76;
    const int SQUAD_SELECTOR_BUTTON_WIDTH = 150;
    const int SQUAD_SELECTOR_BUTTON_HEIGHT = 34;
    const int SQUAD_SELECTOR_BUTTON_GAP = 4;
    const int SQUAD_SELECTOR_SCROLL_HEIGHT = 16;
    // Keep the member rows compact enough to show a full squad without
    // wasting vertical space.  Card dimensions are intentionally modest:
    // the original tick-button skin was being stretched over a very large
    // 220x112 surface and its checkbox texture read like a distorted icon.
    // Member rows need room for three readable, vertically stacked skills.
    // The first skill is two points larger, so keep two extra pixels between
    // the skill block and the bottom controls.
    // Job cards keep their compact 88px height and are centered in the row.
    const int ROW_HEIGHT = 122;
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

    // Value-only data for the squad selector. Never store Platoon or
    // ActivePlatoon pointers in UI state because both are borrowed engine
    // objects whose lifetime can change while the manager is open.
    struct SquadSelectorEntry
    {
        HandleIdentity identity;
        std::string name;
        int memberCount;

        SquadSelectorEntry() : memberCount(0) {}
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
        // TaskData::associated is the native semantic root for wrapper and
        // companion rows. Keep it as a copied scalar so presentation can use
        // the same protected-family contract as destructive cleanup.
        TaskType associatedTaskType;
        std::string jobLabel;
        std::string targetLabel;
        // `fixedTarget` is Kenshi's authoritative perma-job scope.  A
        // Tasker can still carry an incidental subject for global jobs, so
        // the presence of a subject is not enough to make a card target
        // scoped or invalid.
        bool fixedTarget;
        bool hasTarget;
        // `targetAvailable` remains the display-name result used by older
        // station projections.  `targetResolvable` is the separate object
        // lookup result used by the Squad Jobs card warning state.
        bool targetAvailable;
        bool targetResolvable;
        ULONG_PTR taskToken;
        HandleIdentity target;

        JobRowSnapshot() :
            taskType(static_cast<TaskType>(0)),
            associatedTaskType(NULL_TASK), fixedTarget(false),
            hasTarget(false), targetAvailable(false),
            targetResolvable(false), taskToken(0)
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
        bool incomplete;
        std::vector<MemberSnapshot> members;

        SquadSnapshot() : live(false), unavailable(false), incomplete(false) {}
    };

    // The Squad Jobs view can show every active player platoon at once. Keep
    // that board separate from g_squad: g_squad remains Kenshi's current
    // platoon and continues to drive existing actions until the view is
    // migrated to identity-based bindings.
    struct AllSquadsSnapshot
    {
        std::vector<SquadSnapshot> squads;
        bool incomplete;
        unsigned int revision;

        AllSquadsSnapshot() : incomplete(false), revision(1) {}
    };

    // Collapse state is value-only and keyed by the squad handle identity.
    // It can survive a board rebuild without retaining a borrowed Platoon or
    // ActivePlatoon pointer.
    struct SquadCollapseState
    {
        HandleIdentity identity;
        bool collapsed;
        bool autoExpandedByNativeTab;

        SquadCollapseState() :
            collapsed(true), autoExpandedByNativeTab(false) {}
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
        MyGUI::Widget* recipientMarker;
        MyGUI::Button* portraitBorder;
        MyGUI::ImageBox* portraitBackground;
        MyGUI::ImageBox* portrait;
        MyGUI::ImageBox* portraitBackOverlay;
        MyGUI::ImageBox* portraitFrontOverlay;
        MyGUI::TextBox* name;
        MyGUI::TextBox* condition;
        MyGUI::TextBox* highestSkill;
        MyGUI::TextBox* skills;
        MyGUI::Button* jobsToggle;
        MyGUI::Button* clearButton;
        MyGUI::TextBox* emptyJobs;
        std::vector<CardWidgets> cards;
        bool portraitBound;
        unsigned int appliedRevision;

        MemberWidgets() :
            memberRoot(NULL), jobsRoot(NULL), recipientMarker(NULL),
            portraitBorder(NULL),
            portraitBackground(NULL), portrait(NULL), portraitBackOverlay(NULL),
            portraitFrontOverlay(NULL), name(NULL), condition(NULL),
            highestSkill(NULL), skills(NULL),
            jobsToggle(NULL), clearButton(NULL), emptyJobs(NULL),
            portraitBound(false), appliedRevision(0)
        {
        }
    };

    enum DragKind
    {
        DRAG_NONE,
        DRAG_REORDER,
        DRAG_MULTI_MOVE,
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
        bool deferredControlToggle;
        std::vector<JobRowSnapshot> startSequence;

        DragState() :
            kind(DRAG_NONE), armed(false), active(false),
            pressPoint(0, 0), memberIndex(-1), slot(-1), insertionGap(-1),
            deferredPlainClick(false), deferredControlToggle(false)
        {
        }
    };

    enum ModalKind
    {
        MODAL_NONE,
        MODAL_CLEAR,
        MODAL_OPTIONS,
        MODAL_MESSAGE_LOG
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
        ACTION_TRANSFER_STATION_JOB,
        ACTION_ASSIGN_STATION,
        ACTION_REMOVE_STATION_BUNDLE
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
    MyGUI::Widget* g_statusFrame = NULL;
    MyGUI::ScrollView* g_messageLogScroll = NULL;
    const size_t MAX_RECENT_STATUS_MESSAGES = 10;

    struct RecentStatusMessage
    {
        std::string timestamp;
        std::string text;

        RecentStatusMessage() {}

        RecentStatusMessage(
            const std::string& timestampValue,
            const std::string& textValue) :
            timestamp(timestampValue), text(textValue)
        {
        }
    };

    std::vector<RecentStatusMessage> g_recentStatusMessages;
    DWORD g_lastRecentStatusMessageTick = 0;
    MyGUI::Widget* g_background = NULL;
    MyGUI::TextBox* g_emptyText = NULL;
    MyGUI::Button* g_removeButton = NULL;
    MyGUI::Button* g_removeInvalidJobsButton = NULL;
    MyGUI::Button* g_optionsButton = NULL;
    MyGUI::Button* g_closeButton = NULL;
    MyGUI::ScrollBar* g_verticalScroll = NULL;
    MyGUI::ScrollBar* g_horizontalScroll = NULL;
    MyGUI::Widget* g_insertionLine = NULL;
    // Four thin, mouse-transparent edges make the current drag destination
    // unambiguous across both the member and job halves of the board.
    MyGUI::Widget* g_dragDestinationOutlineTop = NULL;
    MyGUI::Widget* g_dragDestinationOutlineBottom = NULL;
    MyGUI::Widget* g_dragDestinationOutlineLeft = NULL;
    MyGUI::Widget* g_dragDestinationOutlineRight = NULL;
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
    AllSquadsSnapshot g_allSquads;
    std::vector<SquadCollapseState> g_squadCollapseStates;
    std::vector<SquadCache> g_squadCaches;
    std::vector<SquadSelectorEntry> g_squadSelectorEntries;
    bool g_squadSelectorIncomplete = false;
    bool g_squadSelectorSelectionPending = false;
    HandleIdentity g_pendingSquadSelectionIdentity;
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
    MyGUI::ScrollView* g_optionsScroll = NULL;
    MyGUI::Button* g_darkUiOptionButton = NULL;

    std::string g_settingsPath;

    DWORD g_lastResetTick = 0;
    DWORD g_lastDragTick = 0;
    bool g_hotkeyWasDown = false;
    bool g_copyHotkeyWasDown = false;
    bool g_pasteHotkeyWasDown = false;
    bool g_escapeWasDown = false;
    bool g_squadCycleObserved = false;
    bool g_closeRequested = false;
    bool g_modalCloseRequested = false;
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
    bool g_hudManagerButtonRequest = false;
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
    void BindSquadMouseWheelTree(MyGUI::Widget*);
    void OnJobsToggleClicked(MyGUI::Widget*);
    void OnClearClicked(MyGUI::Widget*);
    void OnRemoveClicked(MyGUI::Widget*);
    // Implemented by the job-action layer. The window owns only presentation
    // and forwards this button click through the named hook.
    void OnRemoveInvalidJobsClicked(MyGUI::Widget*);
    void OnOptionsClicked(MyGUI::Widget*);
    void OnCloseClicked(MyGUI::Widget*);
    void OnWindowButtonPressed(MyGUI::Window*, const std::string&);
    void OnVerticalScroll(MyGUI::ScrollBar*, size_t);
    void OnHorizontalScroll(MyGUI::ScrollBar*, size_t);
    void OnClearYes(MyGUI::Widget*);
    void OnClearNo(MyGUI::Widget*);
    void OnOptionReset(MyGUI::Widget*);
    void OnOptionClose(MyGUI::Widget*);
    void OnHudManagerButtonClicked(MyGUI::Widget*);
    void ClearJobHoverHighlight();
    void CancelDrag();
    bool IsStationDetailOpen();
    void CloseStationDetail();
    void SetStationDetailStatus(const std::string& text);
    void MarkStationDetailChange(
        const HandleIdentity& station,
        const HandleIdentity& member);
    void RestoreHudManagerButton();
    void TickHudManagerButton();
    void ProcessHudManagerButtonRequest();
    void RefreshStatusMessageFrame();
    void ShowToast(const std::string& message);

#include "RuntimeAccess.inl"
#include "ThemePalette.inl"
#include "SquadPriority.inl"
#include "GeneralJobTransfer.inl"
#include "JobBatchActions.inl"
#include "StationScanner.inl"
#include "StationSettings.inl"
#include "StationAssets.inl"

#ifdef KJM_SCANNER_PROBE
#include "../diagnostics/OwnershipProbe.inl"
#endif

    StationScanState g_stationScan;

#include "JobView.inl"
#include "JobActions.inl"
#include "SquadSelector.inl"
#include "StationView.inl"
#include "JobWindow.inl"
#include "HudButton.inl"
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

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestConstructHand()
{
    OwnershipProbeRequestStage(4);
}

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestValidateHand()
{
    OwnershipProbeRequestStage(5);
}

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestGetBuilding()
{
    OwnershipProbeRequestStage(6);
}

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestVerifyBuilding()
{
    OwnershipProbeRequestStage(7);
}

extern "C" __declspec(dllexport) void KJM_ScannerProbe_RequestCheckOwnership()
{
    OwnershipProbeRequestStage(8);
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
        "borrowed-list and isolated Building stages advance with Ctrl+Shift+F10; "
        "the probe retains no engine pointer and performs no memory release");
#endif

    std::ostringstream message;
    message << "[KenshiJobManagement] " << PLUGIN_NAME << " "
            << PLUGIN_VERSION << " loaded. Press Ctrl+J to open the squad manager.";
    DebugLog(message.str().c_str());
}
