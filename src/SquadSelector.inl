// SPDX-License-Identifier: GPL-3.0-only
    // Compact, value-only selector for Kenshi's active, nonempty squads. The
    // engine snapshot follows the exact raw order used by vanilla TAB.

    MyGUI::Widget* g_squadSelectorRoot = NULL;
    MyGUI::TextBox* g_squadSelectorLabel = NULL;
    MyGUI::Widget* g_squadSelectorViewport = NULL;
    MyGUI::Widget* g_squadSelectorCanvas = NULL;
    MyGUI::ScrollBar* g_squadSelectorScroll = NULL;
    std::vector<MyGUI::Button*> g_squadSelectorButtons;
    int g_squadSelectorOffset = 0;
    int g_squadSelectorMaximumOffset = 0;
    bool g_squadSelectorChangingScroll = false;

    void OnSquadSelectorClicked(MyGUI::Widget* widget);
    void OnSquadSelectorScroll(MyGUI::ScrollBar*, size_t position);
    void OnSquadSelectorMouseWheel(MyGUI::Widget*, int relative);

    bool SameSquadSelectorEntries(
        const std::vector<SquadSelectorEntry>& left,
        const std::vector<SquadSelectorEntry>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.size(); ++index)
        {
            if (!SameHandleIdentity(left[index].identity, right[index].identity) ||
                left[index].name != right[index].name ||
                left[index].memberCount != right[index].memberCount)
            {
                return false;
            }
        }
        return true;
    }

    void ApplySquadSelectorOffset()
    {
        if (g_squadSelectorCanvas != NULL)
        {
            g_squadSelectorCanvas->setPosition(-g_squadSelectorOffset, 0);
        }
    }

    void UpdateSquadSelectorScrollRange()
    {
        if (g_squadSelectorViewport == NULL ||
            g_squadSelectorCanvas == NULL)
        {
            return;
        }
        const int viewportWidth = g_squadSelectorViewport->getWidth();
        const int entryCount = static_cast<int>(g_squadSelectorEntries.size());
        const int requestedWidth = entryCount == 0 ? 0 :
            entryCount * (SQUAD_SELECTOR_BUTTON_WIDTH +
                SQUAD_SELECTOR_BUTTON_GAP) - SQUAD_SELECTOR_BUTTON_GAP;
        const int contentWidth = std::max(viewportWidth, requestedWidth);
        g_squadSelectorCanvas->setSize(
            contentWidth, SQUAD_SELECTOR_BUTTON_HEIGHT);
        g_squadSelectorMaximumOffset = std::max(
            0, contentWidth - viewportWidth);
        g_squadSelectorOffset = ClampInt(
            g_squadSelectorOffset, 0, g_squadSelectorMaximumOffset);

        g_squadSelectorChangingScroll = true;
        if (g_squadSelectorScroll != NULL)
        {
            g_squadSelectorScroll->setScrollRange(
                static_cast<size_t>(g_squadSelectorMaximumOffset + 1));
            g_squadSelectorScroll->setScrollPage(
                static_cast<size_t>(std::max(1, viewportWidth)));
            g_squadSelectorScroll->setScrollViewPage(
                static_cast<size_t>(
                    SQUAD_SELECTOR_BUTTON_WIDTH +
                    SQUAD_SELECTOR_BUTTON_GAP));
            g_squadSelectorScroll->setScrollPosition(
                static_cast<size_t>(g_squadSelectorOffset));
            g_squadSelectorScroll->setEnabled(
                g_squadSelectorMaximumOffset > 0);
        }
        g_squadSelectorChangingScroll = false;
        ApplySquadSelectorOffset();
    }

    void FitSquadSelectorCaption(
        MyGUI::Button* button,
        const std::string& caption)
    {
        if (button == NULL)
        {
            return;
        }
        button->setCaption(caption.c_str());
        int fontHeight = 14;
        button->setFontHeight(fontHeight);
        while (fontHeight > 10 &&
            button->getTextSize().width > button->getWidth() - 14)
        {
            --fontHeight;
            button->setFontHeight(fontHeight);
        }
    }

    void BindSquadSelectorMouseWheelTree(MyGUI::Widget* widget)
    {
        if (widget == NULL)
        {
            return;
        }
        if (widget->isType<MyGUI::ScrollBar>())
        {
            MyGUI::ScrollBar* scrollbar =
                widget->castType<MyGUI::ScrollBar>(false);
            if (scrollbar != NULL)
            {
                scrollbar->setScrollWheelPage(0);
            }
        }
        if (!widget->isUserString("KJM_SquadSelectorWheelBound"))
        {
            widget->eventMouseWheel +=
                MyGUI::newDelegate(OnSquadSelectorMouseWheel);
            widget->setUserString("KJM_SquadSelectorWheelBound", "1");
        }
        const size_t childCount = widget->getChildCount();
        for (size_t index = 0; index < childCount; ++index)
        {
            BindSquadSelectorMouseWheelTree(widget->getChildAt(index));
        }
    }

    void UpdateSquadSelectorSelection()
    {
        const size_t count = std::min(
            g_squadSelectorEntries.size(),
            g_squadSelectorButtons.size());
        for (size_t index = 0; index < count; ++index)
        {
            MyGUI::Button* button = g_squadSelectorButtons[index];
            if (button == NULL)
            {
                continue;
            }
            const bool selected = SameHandleIdentity(
                g_squadSelectorEntries[index].identity,
                g_squad.identity);
            button->setStateSelected(selected);
            button->setTextColour(selected ?
                MyGUI::Colour(0.68f, 1.0f, 0.70f) :
                MyGUI::Colour(1.0f, 1.0f, 1.0f));
        }
    }

    void RevealCurrentSquadSelectorButton()
    {
        if (g_squadSelectorViewport == NULL ||
            g_squadSelectorMaximumOffset <= 0)
        {
            return;
        }

        for (size_t index = 0;
             index < g_squadSelectorEntries.size(); ++index)
        {
            if (!SameHandleIdentity(
                    g_squadSelectorEntries[index].identity,
                    g_squad.identity))
            {
                continue;
            }

            const int left = static_cast<int>(index) *
                (SQUAD_SELECTOR_BUTTON_WIDTH +
                 SQUAD_SELECTOR_BUTTON_GAP);
            const int right = left + SQUAD_SELECTOR_BUTTON_WIDTH;
            const int viewportWidth = g_squadSelectorViewport->getWidth();
            int newOffset = g_squadSelectorOffset;
            if (left < newOffset)
            {
                newOffset = left;
            }
            else if (right > newOffset + viewportWidth)
            {
                newOffset = right - viewportWidth;
            }
            newOffset = ClampInt(
                newOffset, 0, g_squadSelectorMaximumOffset);
            if (newOffset == g_squadSelectorOffset)
            {
                return;
            }

            g_squadSelectorOffset = newOffset;
            g_squadSelectorChangingScroll = true;
            if (g_squadSelectorScroll != NULL)
            {
                g_squadSelectorScroll->setScrollPosition(
                    static_cast<size_t>(g_squadSelectorOffset));
            }
            g_squadSelectorChangingScroll = false;
            ApplySquadSelectorOffset();
            return;
        }
    }

    void DestroySquadSelectorButtons()
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            for (size_t index = 0;
                 index < g_squadSelectorButtons.size(); ++index)
            {
                if (g_squadSelectorButtons[index] != NULL)
                {
                    gui->destroyWidget(g_squadSelectorButtons[index]);
                }
            }
        }
        g_squadSelectorButtons.clear();
    }

    void RebuildSquadSelectorButtons()
    {
        DestroySquadSelectorButtons();
        if (g_squadSelectorCanvas == NULL)
        {
            return;
        }
        g_squadSelectorButtons.reserve(g_squadSelectorEntries.size());
        for (size_t index = 0;
             index < g_squadSelectorEntries.size(); ++index)
        {
            const SquadSelectorEntry& entry = g_squadSelectorEntries[index];
            MyGUI::Button* button =
                g_squadSelectorCanvas->createWidget<MyGUI::Button>(
                    "Kenshi_Button1",
                    MyGUI::IntCoord(
                        static_cast<int>(index) *
                            (SQUAD_SELECTOR_BUTTON_WIDTH +
                             SQUAD_SELECTOR_BUTTON_GAP),
                        0,
                        SQUAD_SELECTOR_BUTTON_WIDTH,
                        SQUAD_SELECTOR_BUTTON_HEIGHT),
                    MyGUI::Align::Left | MyGUI::Align::Top,
                    "KJM_SquadSelectorButton");
            FitSquadSelectorCaption(
                button,
                entry.name.empty() ? "Unnamed squad" : entry.name);
            button->setTextAlign(MyGUI::Align::Center);
            button->setUserString(
                "KJM_SquadSelectorIndex", IntegerString(index));
            button->eventMouseButtonClick +=
                MyGUI::newDelegate(OnSquadSelectorClicked);
            BindSquadSelectorMouseWheelTree(button);
            g_squadSelectorButtons.push_back(button);
        }
        UpdateSquadSelectorScrollRange();
        UpdateSquadSelectorSelection();
        RevealCurrentSquadSelectorButton();
    }

    bool RefreshSquadSelectorRoster(bool force)
    {
        std::vector<SquadSelectorEntry> fresh;
        bool incomplete = false;
        if (!BuildSquadSelectorSnapshot(&fresh, &incomplete))
        {
            if (g_squadSelectorEntries.empty() &&
                g_squadSelectorLabel != NULL)
            {
                g_squadSelectorLabel->setCaption("SQUADS ?");
            }
            return false;
        }

        const bool changed = !SameSquadSelectorEntries(
            g_squadSelectorEntries, fresh);
        if (changed)
        {
            g_squadSelectorEntries.swap(fresh);
            g_squadSelectorOffset = 0;
        }
        g_squadSelectorIncomplete = incomplete;
        if (g_squadSelectorLabel != NULL)
        {
            g_squadSelectorLabel->setCaption(
                incomplete ? "SQUADS *" : "SQUADS");
        }
        if (changed || force ||
            g_squadSelectorButtons.size() !=
                g_squadSelectorEntries.size())
        {
            RebuildSquadSelectorButtons();
        }
        else
        {
            UpdateSquadSelectorSelection();
        }
        return true;
    }

    void CreateSquadSelectorView(
        MyGUI::Widget* parent,
        const MyGUI::IntCoord& coord)
    {
        if (parent == NULL)
        {
            return;
        }
        g_squadSelectorRoot = parent->createWidget<MyGUI::Widget>(
            "PanelEmpty",
            coord,
            MyGUI::Align::Left | MyGUI::Align::Bottom |
                MyGUI::Align::HStretch,
            "KJM_SquadSelectorRoot");
        g_squadSelectorRoot->setUserString(
            "KJM_SquadSelectorWheelRoot", "1");

        g_squadSelectorLabel =
            g_squadSelectorRoot->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(
                    0, 0,
                    SQUAD_SELECTOR_LABEL_WIDTH,
                    SQUAD_SELECTOR_BUTTON_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_SquadSelectorLabel");
        g_squadSelectorLabel->setCaption("SQUADS");
        g_squadSelectorLabel->setFontHeight(16);
        g_squadSelectorLabel->setTextAlign(MyGUI::Align::Center);

        const int selectorLeft = SQUAD_SELECTOR_LABEL_WIDTH + 4;
        const int selectorWidth = std::max(1, coord.width - selectorLeft);
        g_squadSelectorViewport =
            g_squadSelectorRoot->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(
                    selectorLeft, 0,
                    selectorWidth,
                    SQUAD_SELECTOR_BUTTON_HEIGHT),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_SquadSelectorViewport");
        g_squadSelectorCanvas =
            g_squadSelectorViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(
                    0, 0,
                    selectorWidth,
                    SQUAD_SELECTOR_BUTTON_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_SquadSelectorCanvas");
        g_squadSelectorScroll =
            g_squadSelectorRoot->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarH",
                MyGUI::IntCoord(
                    selectorLeft,
                    SQUAD_SELECTOR_BUTTON_HEIGHT + 4,
                    selectorWidth,
                    SQUAD_SELECTOR_SCROLL_HEIGHT),
                MyGUI::Align::Bottom | MyGUI::Align::HStretch,
                "KJM_SquadSelectorScroll");
        g_squadSelectorScroll->eventScrollChangePosition +=
            MyGUI::newDelegate(OnSquadSelectorScroll);
        BindSquadSelectorMouseWheelTree(g_squadSelectorRoot);
        UpdateSquadSelectorScrollRange();
    }

    void ResetSquadSelectorView()
    {
        g_squadSelectorRoot = NULL;
        g_squadSelectorLabel = NULL;
        g_squadSelectorViewport = NULL;
        g_squadSelectorCanvas = NULL;
        g_squadSelectorScroll = NULL;
        g_squadSelectorButtons.clear();
        g_squadSelectorOffset = 0;
        g_squadSelectorMaximumOffset = 0;
        g_squadSelectorChangingScroll = false;
    }

    void OnSquadSelectorClicked(MyGUI::Widget* widget)
    {
        if (g_window == NULL || g_stationTabActive ||
            g_modal.kind != MODAL_NONE || IsStationDetailOpen() ||
            g_drag.armed || g_pendingAction.type != ACTION_NONE)
        {
            return;
        }
        int index = -1;
        if (!GetWidgetIndex(
                widget, "KJM_SquadSelectorIndex", &index) ||
            index < 0 ||
            index >= static_cast<int>(g_squadSelectorEntries.size()))
        {
            return;
        }
        g_pendingSquadSelectionIdentity =
            g_squadSelectorEntries[static_cast<size_t>(index)].identity;
        g_squadSelectorSelectionPending = true;
    }

    void OnSquadSelectorScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_squadSelectorChangingScroll)
        {
            return;
        }
        g_squadSelectorOffset = ClampInt(
            static_cast<int>(position),
            0,
            g_squadSelectorMaximumOffset);
        ApplySquadSelectorOffset();
    }

    void OnSquadSelectorMouseWheel(MyGUI::Widget* widget, int relative)
    {
        if (relative == 0 || g_window == NULL || g_stationTabActive ||
            g_modal.kind != MODAL_NONE || IsStationDetailOpen())
        {
            return;
        }
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        const bool shift =
            (input != NULL && input->isShiftPressed()) ||
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (!shift)
        {
            OnMouseWheel(widget, relative);
            return;
        }
        if (g_squadSelectorMaximumOffset == 0)
        {
            return;
        }
        g_squadSelectorOffset = ClampInt(
            g_squadSelectorOffset - relative * 80,
            0,
            g_squadSelectorMaximumOffset);
        g_squadSelectorChangingScroll = true;
        if (g_squadSelectorScroll != NULL)
        {
            g_squadSelectorScroll->setScrollPosition(
                static_cast<size_t>(g_squadSelectorOffset));
        }
        g_squadSelectorChangingScroll = false;
        ApplySquadSelectorOffset();
    }

    void CancelPendingSquadSelectorSelection()
    {
        g_squadSelectorSelectionPending = false;
        ResetHandleIdentity(&g_pendingSquadSelectionIdentity);
    }

    void PublishUnavailableSquadSelection(
        const HandleIdentity& fallbackIdentity)
    {
        // The engine can complete a squad change even when the following job
        // snapshot read fails. Never leave the previous squad's editable
        // controls on screen in that state.
        HandleIdentity currentIdentity = fallbackIdentity;
        hand currentHandle;
        std::string currentName;
        RootObjectContainer* currentActive = NULL;
        if (TryGetCurrentSquad(
                g_playerInterface,
                &currentHandle,
                &currentName,
                &currentActive))
        {
            CaptureHandleIdentity(currentHandle, &currentIdentity);
        }
        if (currentName.empty())
        {
            for (size_t index = 0;
                 index < g_squadSelectorEntries.size(); ++index)
            {
                if (SameHandleIdentity(
                        g_squadSelectorEntries[index].identity,
                        currentIdentity))
                {
                    currentName = g_squadSelectorEntries[index].name;
                    break;
                }
            }
        }

        if (g_squad.live)
        {
            StoreCurrentSquadCache();
        }
        CancelDrag();
        g_selectedJobs.clear();
        ResetHandleIdentity(&g_selectionAnchorMember);
        g_selectionAnchorSlot = -1;
        g_horizontalOffset = 0;
        g_verticalOffset = 0;

        SquadSnapshot unavailable;
        unavailable.identity = currentIdentity;
        unavailable.handle = currentHandle;
        unavailable.name = currentName;
        unavailable.live = false;
        unavailable.unavailable = true;
        g_squad = unavailable;
        RebuildJobHighlightCache();
        RebuildSquadWidgets();
        UpdateSquadHeading();
        ApplyCardSelectionStates();
    }

    bool ProcessPendingSquadSelectorSelection()
    {
        if (!g_squadSelectorSelectionPending)
        {
            return true;
        }
        const HandleIdentity target = g_pendingSquadSelectionIdentity;
        CancelPendingSquadSelectorSelection();
        if (g_window == NULL || g_stationTabActive ||
            g_modal.kind != MODAL_NONE || IsStationDetailOpen() ||
            g_drag.armed || g_pendingAction.type != ACTION_NONE)
        {
            SetStatus("Squad selection was cancelled because another manager action is active.");
            return false;
        }
        if (!TrySelectSquadByIdentity(target))
        {
            SetStatus("That squad is no longer available. The current squad was not changed.");
            RefreshSquadSelectorRoster(false);
            return false;
        }
        if (!RefreshSquadView(true))
        {
            PublishUnavailableSquadSelection(target);
            SetStatus("Kenshi selected the squad, but its jobs are temporarily unavailable.");
        }
        else
        {
            SetStatus("");
        }
        RefreshSquadSelectorRoster(false);
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        if (input != NULL && g_window != NULL)
        {
            input->setKeyFocusWidget(g_window);
        }
        return true;
    }
