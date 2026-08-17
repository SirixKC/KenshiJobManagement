// SPDX-License-Identifier: GPL-3.0-only
    void OnCloseClicked(MyGUI::Widget*)
    {
        g_closeRequested = true;
    }

    void OnWindowButtonPressed(MyGUI::Window*, const std::string& name)
    {
        if (name == "close")
        {
            g_closeRequested = true;
        }
    }

    // Keep the player-station and assignment projection fail-closed. Ownership
    // records are copied as scalar POD, and exact queue targets are resolved
    // from loaded player queues; no borrowed or live engine pointer is cached.
    __declspec(noinline) bool TryBeginStationScanGuarded()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = BeginStationScan(ou, g_playerInterface, &g_stationScan);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted)
        {
            ErrorLog("[KenshiJobManagement] Structured exception while building the station view.");
            g_stationScan.started = true;
            g_stationScan.complete = true;
            g_stationScan.rosterIncomplete = true;
            return false;
        }
        return result;
    }

    __declspec(noinline) bool TryStepStationScanGuarded()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = StepStationScan(ou, g_playerInterface, &g_stationScan);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted)
        {
            ErrorLog("[KenshiJobManagement] Structured exception while resolving a station candidate.");
            g_stationScan.complete = true;
            g_stationScan.rosterIncomplete = true;
            return false;
        }
        return result;
    }

    bool CompleteStationScanImmediately()
    {
        if (!g_stationScan.started)
        {
            return false;
        }
        const size_t candidateCount =
            StationScanCandidateCount(g_stationScan);
        const size_t stationCountBefore = g_stationScan.stations.size();
        const DWORD startTick = GetTickCount();
        size_t steps = 0;
        bool succeeded = true;
        while (!g_stationScan.complete && steps <= candidateCount)
        {
            const size_t nextTargetBefore = g_stationScan.nextTarget;
            if (!TryStepStationScanGuarded())
            {
                succeeded = false;
                break;
            }
            ++steps;
            if (!g_stationScan.complete &&
                g_stationScan.nextTarget == nextTargetBefore)
            {
                succeeded = false;
                break;
            }
        }
        if (!g_stationScan.complete)
        {
            g_stationScan.complete = true;
            g_stationScan.ownershipResolutionIncomplete = true;
            AddStationScanError(
                &g_stationScan,
                "The player-station scan stopped before every candidate was read.");
            succeeded = false;
        }
        if (!succeeded)
        {
            // A guarded candidate fault can leave fewer than one presentation
            // batch of normalized snapshots pending. Do not expose complete
            // state with unpublished values whose final join was skipped.
            g_stationScan.pendingStations.clear();
        }
        else if (!g_stationScan.pendingStations.empty())
        {
            PublishPendingStationResults(&g_stationScan);
        }
        for (size_t stationIndex = stationCountBefore;
             stationIndex < g_stationScan.stations.size(); ++stationIndex)
        {
            SyncJobStationArtworkFromSnapshot(
                g_stationScan.stations[stationIndex]);
        }
        if (steps > 0)
        {
            std::ostringstream completed;
            completed << "[KenshiJobManagement] Player-station scan "
                      << (succeeded ? "completed" : "stopped")
                      << " after " << steps << " candidate(s) in "
                      << (GetTickCount() - startTick) << " ms.";
            DebugLog(completed.str().c_str());
        }
        return succeeded && g_stationScan.complete;
    }

    __declspec(noinline) bool CallCompleteStationScanImmediatelySafely()
    {
        try
        {
            return CompleteStationScanImmediately();
        }
        catch (...)
        {
            return false;
        }
    }

    void RecordSynchronousStationScanFailure()
    {
        g_stationScan.complete = true;
        g_stationScan.ownershipResolutionIncomplete = true;
        g_stationScan.pendingStations.clear();
        try
        {
            AddStationScanError(
                &g_stationScan,
                "The synchronous player-station scan was interrupted safely.");
        }
        catch (...)
        {
        }
        ErrorLog("[KenshiJobManagement] The synchronous player-station scan was interrupted safely.");
    }

    __declspec(noinline) bool TryCompleteStationScanImmediately()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallCompleteStationScanImmediatelySafely();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || !result)
        {
            RecordSynchronousStationScanFailure();
            return false;
        }
        return true;
    }

    __declspec(noinline) bool TryRefreshStationAssignmentsGuarded()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = RefreshStationAssignments(
                g_playerInterface, &g_stationScan);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted)
        {
            ErrorLog("[KenshiJobManagement] Structured exception while refreshing station assignments.");
            g_stationScan.rosterIncomplete = true;
            return false;
        }
        return result;
    }

    __declspec(noinline) bool CallSwitchStationViewSafely(bool visible)
    {
        try
        {
            SwitchStationView(visible);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TrySwitchStationViewGuarded(bool visible)
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallSwitchStationViewSafely(visible);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || !result)
        {
            ErrorLog("[KenshiJobManagement] The Stations view failed safely while changing tabs.");
            return false;
        }
        return true;
    }

    __declspec(noinline) bool CallTickStationViewSafely(
        bool snapshotChanged,
        bool scanProgressChanged)
    {
        try
        {
            TickStationView(
                &g_stationScan, snapshotChanged, scanProgressChanged);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TryTickStationViewGuarded(
        bool snapshotChanged,
        bool scanProgressChanged)
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallTickStationViewSafely(
                snapshotChanged, scanProgressChanged);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || !result)
        {
            ErrorLog("[KenshiJobManagement] The Stations view failed safely while drawing results.");
            return false;
        }
        return true;
    }

    __declspec(noinline) bool CallRefreshStationTransferredRowsSafely(
        const HandleIdentity& source,
        const HandleIdentity& destination)
    {
        try
        {
            return RefreshStationTransferredMemberRows(
                source, destination);
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TryRefreshStationTransferredRowsGuarded(
        const HandleIdentity& source,
        const HandleIdentity& destination)
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallRefreshStationTransferredRowsSafely(
                source, destination);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || !result)
        {
            ErrorLog(
                "[KenshiJobManagement] Selective Stations row refresh failed; redrawing visible widgets.");
            return false;
        }
        return true;
    }

    void SetManagerTab(bool stations)
    {
        if (g_window == NULL)
        {
            return;
        }
        if (stations && g_drag.armed)
        {
            CancelDrag();
        }
        if (g_tooltip != NULL)
        {
            g_tooltip->setVisible(false);
        }
        g_stationTabActive = stations;
        if (stations)
        {
            SetStatus("");
        }
        if (g_squadTabRoot != NULL)
        {
            g_squadTabRoot->setVisible(!stations);
        }
        if (g_squadTabButton != NULL)
        {
            g_squadTabButton->setStateSelected(!stations);
        }
        if (g_stationTabButton != NULL)
        {
            g_stationTabButton->setStateSelected(stations);
        }

        if (stations && !g_stationIconLocationRegistered)
        {
            // Load only this plug-in's nine packaged PNG files.  The Stations
            // view keeps its short text labels as a fail-safe if Ogre rejects
            // the location or an image.
            TryEnsureStationIconResourcesGuarded();
        }

        if (stations && !g_stationScan.started)
        {
            DebugLog("[KenshiJobManagement] Building Stations from loaded squad job targets.");
            if (TryBeginStationScanGuarded())
            {
                std::ostringstream scanReady;
                scanReady << "[KenshiJobManagement] Stations roster, ownership records, and assigned targets captured: "
                          << StationScanCandidateCount(g_stationScan)
                          << " candidate(s).";
                DebugLog(scanReady.str().c_str());
                if (!TryCompleteStationScanImmediately())
                {
                    DebugLog("[KenshiJobManagement] Stations opened with an incomplete player-station scan.");
                }
            }
            else
            {
                DebugLog("[KenshiJobManagement] Stations player-station view failed safely during startup.");
            }
            g_stationAssignmentsDirty = false;
        }
        else if (stations && g_stationAssignmentsDirty)
        {
            // Squad-tab edits do not cause a hidden scan pause. Refresh the
            // copied roster/candidates and finish the board only when the
            // player explicitly opens Stations again.
            if (TryRefreshStationAssignmentsGuarded())
            {
                g_stationAssignmentsDirty = false;
                TryCompleteStationScanImmediately();
            }
        }
        if (!TrySwitchStationViewGuarded(stations))
        {
            // Closing the manager is safer than leaving a partially-created
            // widget tree alive.  Kenshi itself remains paused until normal
            // window cleanup runs on the next update.
            g_closeRequested = true;
        }
    }

    void OnSquadTabClicked(MyGUI::Widget*)
    {
        SetManagerTab(false);
    }

    void OnStationTabClicked(MyGUI::Widget*)
    {
        SetManagerTab(true);
    }

    void ResetWindowPointers()
    {
        g_window = NULL;
        g_squadTabRoot = NULL;
        g_squadTabButton = NULL;
        g_stationTabButton = NULL;
        g_memberViewport = NULL;
        g_memberCanvas = NULL;
        g_jobViewport = NULL;
        g_jobCanvas = NULL;
        g_priorityViewport = NULL;
        g_priorityCanvas = NULL;
        g_squadText = NULL;
        g_statusText = NULL;
        g_emptyText = NULL;
        g_removeButton = NULL;
        g_optionsButton = NULL;
        g_closeButton = NULL;
        g_verticalScroll = NULL;
        g_horizontalScroll = NULL;
        g_insertionLine = NULL;
        g_tooltip = NULL;
        g_tooltipText = NULL;
    }

    void DestroyJobWindow(bool duringReset = false)
    {
        CancelDrag();
        if (g_modal.kind != MODAL_NONE)
        {
            CloseModalNow();
        }

        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            try
            {
                DestroyStationView();
                if (g_tooltip != NULL)
                {
                    gui->destroyWidget(g_tooltip);
                    g_tooltip = NULL;
                    g_tooltipText = NULL;
                }
                if (g_window != NULL)
                {
                    if (g_windowModalAdded)
                    {
                        MyGUI::InputManager* input =
                            MyGUI::InputManager::getInstancePtr();
                        if (input != NULL)
                        {
                            input->removeWidgetModal(g_window);
                        }
                    }
                    gui->destroyWidget(g_window);
                }
            }
            catch (...)
            {
                ErrorLog("[KenshiJobManagement] MyGUI threw while closing the squad manager.");
            }
        }

        const bool keepResume = g_closeForResume || duringReset;
        RestoreGamePauseState(keepResume);

        ResetWindowPointers();
        g_windowModalAdded = false;
        g_memberWidgets.clear();
        g_priorityLabels.clear();
        g_jobHighlightKeys.clear();
        g_jobHighlightCacheValid = false;
        g_hoveredJobHighlightGroup = -1;
        g_selectedJobs.clear();
        g_squadCaches.clear();
        g_squad = SquadSnapshot();
        g_drag = DragState();
        g_modal = ModalState();
        g_pendingAction = PendingAction();
        g_stationOptionButtons.clear();
        g_stationScan = StationScanState();
        ClearJobStationCategoryCache();
        ResetHandleIdentity(&g_selectionAnchorMember);
        g_selectionAnchorSlot = -1;
        g_horizontalOffset = 0;
        g_verticalOffset = 0;
        g_maxHorizontalOffset = 0;
        g_maxVerticalOffset = 0;
        g_closeRequested = false;
        g_closeForResume = false;
        g_modalCloseRequested = false;
        g_stationFilterRefreshRequested = false;
        g_stationAssignmentsDirty = false;
        g_stationProjectionRefreshRequested = false;
        ResetHandleIdentity(&g_stationProjectionRefreshSource);
        ResetHandleIdentity(&g_stationProjectionRefreshDestination);
        g_lastStationAssignmentRefreshAttempt = 0;
        g_stationTabActive = false;
        g_settingsWriteFailed = false;
        g_escapeWasDown = false;
        g_tabWasDown = false;
        g_lastDragTick = 0;
    }

    void CreateJobWindow()
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
        {
            ErrorLog("[KenshiJobManagement] MyGUI was unavailable when opening the squad manager.");
            return;
        }
        if (g_window != NULL)
        {
            return;
        }

        LoadStationCategorySettings();
        if (!TryCaptureAndPauseGame())
        {
            ErrorLog("[KenshiJobManagement] Could not acquire Kenshi's native pause state.");
            return;
        }

        try
        {
            const MyGUI::IntSize screen =
                MyGUI::RenderManager::getInstance().getViewSize();
            g_window = gui->createWidget<MyGUI::Window>(
                "Kenshi_WindowCX",
                MyGUI::IntCoord(0, 0, screen.width, screen.height),
                MyGUI::Align::Stretch,
                "Popup",
                "KJM_SquadWindow");
            g_window->setMovable(false);
            g_window->setAutoAlpha(false);
            g_window->setCaption("Kenshi Job Management 0.1.0-alpha");
            g_window->eventWindowButtonPressed +=
                MyGUI::newDelegate(OnWindowButtonPressed);

            MyGUI::Widget* client = g_window->getClientWidget();
            // Kenshi_WindowCX places Client inside an opaque native window
            // body. Hide that body and draw a predictable custom backdrop.
            MyGUI::Widget* nativeBody = client != NULL ? client->getParent() : NULL;
            if (nativeBody != NULL && nativeBody != g_window)
            {
                client->setInheritsAlpha(false);
                nativeBody->setAlpha(0.0f);
            }
            const MyGUI::IntSize size = client->getSize();
            MyGUI::Widget* background = client->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(0, 0, size.width, size.height),
                MyGUI::Align::Stretch,
                "KJM_Background");
            background->setColour(MyGUI::Colour(0.105f, 0.085f, 0.065f));
            // Keep the manager fully opaque. The game world showing through
            // reduced contrast for portraits, station art, and small labels.
            background->setAlpha(1.0f);
            background->setNeedMouseFocus(true);

            g_squadTabRoot = client->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(0, 0, size.width, size.height),
                MyGUI::Align::Stretch,
                "KJM_SquadTabContent");

            g_memberWidth = ClampInt(size.width / 4, 260, 340);
            const int jobLeft = PAD + g_memberWidth + PAD;
            const int bodyTop = PAD + TOP_HEIGHT + HEADER_HEIGHT;
            g_jobWidth = std::max(
                CARD_WIDTH,
                size.width - jobLeft - SCROLL_SIZE - PAD);
            g_bodyHeight = std::max(
                ROW_HEIGHT,
                size.height - bodyTop - ACTION_HEIGHT - SCROLL_SIZE - 3 * PAD);

            g_squadText = g_squadTabRoot->createWidget<MyGUI::TextBox>(
                // Standard is 20pt; Large is 24pt, which gives the squad
                // name and member count the requested four-point increase.
                "Kenshi_TextboxStandardText_Large",
                MyGUI::IntCoord(
                    PAD + 300, PAD,
                    std::max(160, size.width - 2 * PAD - 300),
                    TOP_HEIGHT - 6),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_SquadHeading");
            g_squadText->setCaption("Current squad");
            g_squadText->setTextAlign(MyGUI::Align::Center);
            g_squadText->setNeedMouseFocus(false);

            MyGUI::TextBox* memberHeader = g_squadTabRoot->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(PAD, PAD + TOP_HEIGHT, g_memberWidth, HEADER_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_MemberHeader");
            memberHeader->setCaption("SQUAD MEMBER  |  TOP STATS");
            memberHeader->setTextAlign(MyGUI::Align::Center);
            memberHeader->setNeedMouseFocus(false);

            g_priorityViewport = g_squadTabRoot->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(jobLeft, PAD + TOP_HEIGHT, g_jobWidth, HEADER_HEIGHT),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_PriorityViewport");
            g_priorityCanvas = g_priorityViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(0, 0, g_jobWidth, HEADER_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_PriorityCanvas");
            g_priorityViewport->setNeedMouseFocus(false);

            g_memberViewport = g_squadTabRoot->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(PAD, bodyTop, g_memberWidth, g_bodyHeight),
                MyGUI::Align::Left | MyGUI::Align::VStretch,
                "KJM_MemberViewport");
            g_memberCanvas = g_memberViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(0, 0, g_memberWidth, g_bodyHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_MemberCanvas");
            g_memberViewport->eventMouseWheel += MyGUI::newDelegate(OnMouseWheel);
            g_memberCanvas->eventMouseButtonPressed += MyGUI::newDelegate(OnEmptyPressed);

            g_jobViewport = g_squadTabRoot->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(jobLeft, bodyTop, g_jobWidth, g_bodyHeight),
                MyGUI::Align::Stretch,
                "KJM_JobViewport");
            g_jobViewport->eventMouseWheel += MyGUI::newDelegate(OnMouseWheel);
            g_jobCanvas = g_jobViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(0, 0, g_jobWidth, g_bodyHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_JobCanvas");
            g_jobCanvas->eventMouseButtonPressed += MyGUI::newDelegate(OnEmptyPressed);
            g_jobCanvas->eventMouseWheel += MyGUI::newDelegate(OnMouseWheel);

            g_emptyText = g_squadTabRoot->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(jobLeft + 20, bodyTop + 40, g_jobWidth - 40, 80),
                MyGUI::Align::Center,
                "KJM_EmptySquad");
            g_emptyText->setTextAlign(MyGUI::Align::Center);
            g_emptyText->setNeedMouseFocus(false);
            g_emptyText->setVisible(false);

            g_verticalScroll = g_squadTabRoot->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarV",
                MyGUI::IntCoord(
                    jobLeft + g_jobWidth,
                    bodyTop,
                    SCROLL_SIZE,
                    g_bodyHeight),
                MyGUI::Align::Right | MyGUI::Align::VStretch,
                "KJM_VScroll");
            g_verticalScroll->eventScrollChangePosition +=
                MyGUI::newDelegate(OnVerticalScroll);

            g_horizontalScroll = g_squadTabRoot->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarH",
                MyGUI::IntCoord(
                    jobLeft,
                    bodyTop + g_bodyHeight,
                    g_jobWidth,
                    SCROLL_SIZE),
                MyGUI::Align::Bottom | MyGUI::Align::HStretch,
                "KJM_HScroll");
            g_horizontalScroll->eventScrollChangePosition +=
                MyGUI::newDelegate(OnHorizontalScroll);

            const int actionTop = bodyTop + g_bodyHeight + SCROLL_SIZE + PAD;
            g_removeButton = g_squadTabRoot->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                MyGUI::IntCoord(PAD, actionTop, 220, 40),
                MyGUI::Align::Left | MyGUI::Align::Bottom,
                "KJM_RemoveSelected");
            g_removeButton->setCaption("Remove Selected (0)");
            g_removeButton->setEnabled(false);
            g_removeButton->eventMouseButtonClick += MyGUI::newDelegate(OnRemoveClicked);

            g_optionsButton = client->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                MyGUI::IntCoord(PAD + 232, actionTop, 120, 40),
                MyGUI::Align::Left | MyGUI::Align::Bottom,
                "KJM_Options");
            g_optionsButton->setCaption("Options");
            g_optionsButton->eventMouseButtonClick += MyGUI::newDelegate(OnOptionsClicked);

            g_closeButton = client->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                MyGUI::IntCoord(size.width - PAD - 120, actionTop, 120, 40),
                MyGUI::Align::Right | MyGUI::Align::Bottom,
                "KJM_Close");
            g_closeButton->setCaption("Close");
            g_closeButton->eventMouseButtonClick += MyGUI::newDelegate(OnCloseClicked);

            // Keep the status channel for errors and mutation results, but do
            // not show routine squad-order or drag instructions here.
            g_statusText = client->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(
                    PAD + 364,
                    actionTop,
                    std::max(120, size.width - 2 * PAD - 364 - 132),
                    40),
                MyGUI::Align::Bottom | MyGUI::Align::HStretch,
                "KJM_Status");
            g_statusText->setCaption("");
            g_statusText->setTextAlign(MyGUI::Align::Center);
            g_statusText->setNeedMouseFocus(false);

            g_insertionLine = g_squadTabRoot->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(0, 0, 4, ROW_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_InsertionLine");
            g_insertionLine->setColour(MyGUI::Colour(0.35f, 0.95f, 0.45f));
            g_insertionLine->setNeedMouseFocus(false);
            g_insertionLine->setVisible(false);

            CreateStationView(
                client,
                MyGUI::IntCoord(
                    PAD,
                    PAD + TOP_HEIGHT,
                    size.width - 2 * PAD,
                    std::max(
                        STATION_MEMBER_ROW_HEIGHT + STATION_HEADER_HEIGHT + 60,
                        actionTop - (PAD + TOP_HEIGHT) - PAD)));
            SetStationBoardSnapshot(&g_stationScan);

            g_squadTabButton = client->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                MyGUI::IntCoord(PAD, PAD, 138, 36),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_SquadTabButton");
            g_squadTabButton->setCaption("Squad Jobs");
            g_squadTabButton->eventMouseButtonClick +=
                MyGUI::newDelegate(OnSquadTabClicked);
            g_stationTabButton = client->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                MyGUI::IntCoord(PAD + 146, PAD, 138, 36),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationTabButton");
            g_stationTabButton->setCaption("Stations");
            g_stationTabButton->eventMouseButtonClick +=
                MyGUI::newDelegate(OnStationTabClicked);

            MyGUI::InputManager::getInstance().addWidgetModal(g_window);
            g_windowModalAdded = true;
            MyGUI::InputManager::getInstance().setKeyFocusWidget(g_window);

            g_lastRefreshTick = GetTickCount();
            g_tabWasDown = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
            g_escapeWasDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
            // Squad cards now reuse the station-category artwork. Load the
            // same small packaged icon set before the first card build; the
            // guarded helper leaves the text-only fallback intact if Ogre
            // cannot load it.
            TryEnsureStationIconResourcesGuarded();
            RefreshSquadView(true);
            SetManagerTab(false);
        }
        catch (...)
        {
            ErrorLog("[KenshiJobManagement] MyGUI threw while creating the squad manager.");
            DestroyJobWindow(false);
        }
    }

    void ToggleJobWindow()
    {
        if (g_window != NULL)
        {
            g_closeRequested = true;
        }
        else
        {
            CreateJobWindow();
        }
    }

    void TickHotkey()
    {
        const bool controlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool jDown = (GetAsyncKeyState('J') & 0x8000) != 0;
        const bool hotkeyDown = controlDown && jDown;
        const DWORD now = GetTickCount();
        if (g_lastResetTick != 0 &&
            !HasElapsed(now, g_lastResetTick, RESET_HOTKEY_COOLDOWN_MS))
        {
            g_hotkeyWasDown = hotkeyDown;
            return;
        }
        if (hotkeyDown && !g_hotkeyWasDown)
        {
            ToggleJobWindow();
        }
        g_hotkeyWasDown = hotkeyDown;
    }

    void TickWindow()
    {
        if (g_window == NULL)
        {
            return;
        }

        bool paused = true;
        float speed = g_managerPausedSpeed;
        if (g_pauseCaptured &&
            TryReadGamePauseAndSpeed(&paused, &speed) &&
            (!paused || std::fabs(speed - g_managerPausedSpeed) > 0.001f))
        {
            g_closeForResume = true;
            g_closeRequested = true;
        }
        if (g_closeRequested)
        {
            DestroyJobWindow(false);
            return;
        }

        const bool escapeDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        if (escapeDown && !g_escapeWasDown)
        {
            if (IsStationAssignmentDragArmed())
            {
                CancelStationAssignmentDrag();
            }
            else if (IsStationHeaderDragArmed())
            {
                CancelStationHeaderDrag();
            }
            else if (IsStationDetailOpen())
            {
                CloseStationDetail();
                RefreshStationView();
            }
            else if (g_drag.armed)
            {
                CancelDrag();
            }
            else if (g_modal.kind != MODAL_NONE)
            {
                g_modalCloseRequested = true;
            }
            else
            {
                g_closeRequested = true;
            }
        }
        g_escapeWasDown = escapeDown;

        const bool tabDown = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
        if (tabDown && !g_tabWasDown && g_modal.kind == MODAL_NONE &&
            !IsStationDetailOpen())
        {
            TryCycleCurrentSquad();
            RefreshSquadView(true);
        }
        g_tabWasDown = tabDown;

        if (g_modalCloseRequested)
        {
            CloseModalNow();
        }
        ProcessPendingAction();
        // Squad-card artwork is hidden while Stations is active. Do not pair
        // its separate live-building lookup with every scanner lookup; pending
        // artwork resumes when Squad Jobs is shown or the station pass ends.
        if (!g_stationTabActive || !g_stationScan.started ||
            g_stationScan.complete)
        {
            TickJobStationCategoryCache();
        }

        const DWORD now = GetTickCount();
        TickDragAutoScroll(now);
        if (HasElapsed(now, g_lastRefreshTick, REFRESH_INTERVAL_MS))
        {
            g_lastRefreshTick = now;
            RefreshSquadView(false);
        }

        bool stationViewChanged = false;
        bool stationProgressChanged = false;
        const bool stationInteractionDragging =
            IsStationInteractionDragArmed();
        if (!stationInteractionDragging &&
            g_stationProjectionRefreshRequested)
        {
            g_stationProjectionRefreshRequested = false;
            const bool refreshed = TryRefreshStationTransferredRowsGuarded(
                g_stationProjectionRefreshSource,
                g_stationProjectionRefreshDestination);
            ResetHandleIdentity(&g_stationProjectionRefreshSource);
            ResetHandleIdentity(&g_stationProjectionRefreshDestination);
            if (!refreshed)
            {
                // The projection is already patched. Rebuild only the current
                // virtual view; do not discard it or start another queue scan.
                stationViewChanged = true;
            }
        }
        if (!stationInteractionDragging && g_stationTabActive &&
            g_stationScan.started &&
            g_stationAssignmentsDirty &&
            (g_lastStationAssignmentRefreshAttempt == 0 ||
             HasElapsed(
                 now, g_lastStationAssignmentRefreshAttempt,
                 REFRESH_INTERVAL_MS)))
        {
            g_lastStationAssignmentRefreshAttempt = now;
            if (TryRefreshStationAssignmentsGuarded())
            {
                g_stationAssignmentsDirty = false;
                if (!TryCompleteStationScanImmediately())
                {
                    stationProgressChanged = true;
                }
                stationViewChanged = true;
            }
            else
            {
                // The guarded refresh records its incomplete/error state.
                // Update only the banner while the dirty request remains
                // queued for a later throttled retry.
                stationProgressChanged = true;
            }
        }
        if (!stationInteractionDragging && g_stationFilterRefreshRequested)
        {
            g_stationFilterRefreshRequested = false;
            stationViewChanged = true;
            if (g_settingsWriteFailed)
            {
                SetStatus(
                    "Station-category options were applied, but settings.ini could not be saved.");
                g_settingsWriteFailed = false;
            }
        }
        if (!TryTickStationViewGuarded(
                stationViewChanged, stationProgressChanged))
        {
            g_closeRequested = true;
        }
    }
