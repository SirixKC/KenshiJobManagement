// SPDX-License-Identifier: GPL-3.0-only
    bool GetWidgetIndex(MyGUI::Widget* widget, const char* key, int* valueOut)
    {
        if (widget == NULL || valueOut == NULL || !widget->isUserString(key))
        {
            return false;
        }
        *valueOut = std::atoi(widget->getUserString(key).c_str());
        return true;
    }

    bool GetCardBinding(MyGUI::Widget* widget, int* memberIndexOut, int* slotOut)
    {
        return GetWidgetIndex(widget, "KJM_Member", memberIndexOut) &&
            GetWidgetIndex(widget, "KJM_Slot", slotOut) &&
            *memberIndexOut >= 0 && *slotOut >= 0 &&
            *memberIndexOut < static_cast<int>(g_squad.members.size()) &&
            *slotOut < static_cast<int>(g_squad.members[*memberIndexOut].jobs.size());
    }

    void ClearJobHoverHighlight()
    {
        g_hoveredJobHighlightGroup = -1;
        ApplyCardHoverHighlights();
    }

    void OnCardMouseSetFocus(MyGUI::Widget* widget, MyGUI::Widget*)
    {
        if (g_modal.kind != MODAL_NONE || g_drag.active)
        {
            ClearJobHoverHighlight();
            return;
        }

        int memberIndex = -1;
        int slot = -1;
        if (!GetCardBinding(widget, &memberIndex, &slot) ||
            memberIndex >= static_cast<int>(g_memberWidgets.size()) ||
            slot >= static_cast<int>(g_memberWidgets[memberIndex].cards.size()))
        {
            ClearJobHoverHighlight();
            return;
        }

        const int group =
            g_memberWidgets[memberIndex].cards[slot].highlightGroup;
        if (group < 0)
        {
            ClearJobHoverHighlight();
            return;
        }
        g_hoveredJobHighlightGroup = group;
        ApplyCardHoverHighlights();
    }

    void OnCardMouseLostFocus(MyGUI::Widget*, MyGUI::Widget*)
    {
        ClearJobHoverHighlight();
    }

    int FindSelectedJobIndex(
        const HandleIdentity& member,
        const JobRowSnapshot& job,
        int slot)
    {
        for (size_t index = 0; index < g_selectedJobs.size(); ++index)
        {
            if (SameHandleIdentity(g_selectedJobs[index].member, member) &&
                SameJob(g_selectedJobs[index].job, job) &&
                (job.taskToken != 0 || g_selectedJobs[index].lastSlot == slot))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void AddSelectedJob(int memberIndex, int slot)
    {
        if (memberIndex < 0 || slot < 0 ||
            memberIndex >= static_cast<int>(g_squad.members.size()) ||
            slot >= static_cast<int>(g_squad.members[memberIndex].jobs.size()))
        {
            return;
        }
        const MemberSnapshot& member = g_squad.members[memberIndex];
        const JobRowSnapshot& job = member.jobs[slot];
        if (FindSelectedJobIndex(member.identity, job, slot) >= 0)
        {
            return;
        }
        SelectedJob selected;
        selected.member = member.identity;
        selected.job = job;
        selected.lastSlot = slot;
        g_selectedJobs.push_back(selected);
    }

    void SelectOnlyJob(int memberIndex, int slot)
    {
        g_selectedJobs.clear();
        AddSelectedJob(memberIndex, slot);
        if (memberIndex >= 0 && memberIndex < static_cast<int>(g_squad.members.size()))
        {
            g_selectionAnchorMember = g_squad.members[memberIndex].identity;
            g_selectionAnchorSlot = slot;
        }
        ApplyCardSelectionStates();
    }

    void ApplyCardClickSelection(int memberIndex, int slot)
    {
        const MemberSnapshot& member = g_squad.members[memberIndex];
        const JobRowSnapshot& job = member.jobs[slot];
        MyGUI::InputManager& input = MyGUI::InputManager::getInstance();
        const bool control = input.isControlPressed();
        const bool shift = input.isShiftPressed();

        if (shift && SameHandleIdentity(g_selectionAnchorMember, member.identity) &&
            g_selectionAnchorSlot >= 0)
        {
            if (!control)
            {
                g_selectedJobs.clear();
            }
            const int low = std::min(g_selectionAnchorSlot, slot);
            const int high = std::max(g_selectionAnchorSlot, slot);
            for (int rangeSlot = low; rangeSlot <= high; ++rangeSlot)
            {
                if (rangeSlot < static_cast<int>(member.jobs.size()))
                {
                    AddSelectedJob(memberIndex, rangeSlot);
                }
            }
        }
        else if (control)
        {
            const int selectedIndex =
                FindSelectedJobIndex(member.identity, job, slot);
            if (selectedIndex >= 0)
            {
                g_selectedJobs.erase(g_selectedJobs.begin() + selectedIndex);
            }
            else
            {
                AddSelectedJob(memberIndex, slot);
            }
            g_selectionAnchorMember = member.identity;
            g_selectionAnchorSlot = slot;
        }
        else
        {
            SelectOnlyJob(memberIndex, slot);
            return;
        }

        ApplyCardSelectionStates();
    }

    void ClearJobSelection()
    {
        g_selectedJobs.clear();
        ResetHandleIdentity(&g_selectionAnchorMember);
        g_selectionAnchorSlot = -1;
        ApplyCardSelectionStates();
    }

    void OnEmptyPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button == MyGUI::MouseButton::Left && g_modal.kind == MODAL_NONE)
        {
            ClearJobSelection();
        }
    }

    bool PointInside(const MyGUI::IntCoord& coord, const MyGUI::IntPoint& point)
    {
        return point.left >= coord.left && point.left < coord.right() &&
            point.top >= coord.top && point.top < coord.bottom();
    }

    void ResetRemoveDropStyle()
    {
        if (g_removeButton != NULL)
        {
            g_removeButton->setColour(MyGUI::Colour::White);
            g_removeButton->setStateSelected(false);
        }
        UpdateRemoveButton();
    }

    void CancelDrag()
    {
        g_drag = DragState();
        g_lastDragTick = 0;
        ClearJobHoverHighlight();
        if (g_insertionLine != NULL)
        {
            g_insertionLine->setVisible(false);
        }
        ResetRemoveDropStyle();
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        if (input != NULL)
        {
            try
            {
                input->resetMouseCaptureWidget();
            }
            catch (...)
            {
            }
        }
    }

    bool MouseIsInSourceRow(const MyGUI::IntPoint& mouse)
    {
        if (g_jobViewport == NULL || g_drag.memberIndex < 0)
        {
            return false;
        }
        const MyGUI::IntCoord view = g_jobViewport->getAbsoluteCoord();
        const int rowTop = view.top + g_drag.memberIndex * ROW_STRIDE - g_verticalOffset;
        return mouse.left >= view.left && mouse.left < view.right() &&
            mouse.top >= view.top && mouse.top < view.bottom() &&
            mouse.top >= rowTop && mouse.top < rowTop + ROW_HEIGHT;
    }

    void UpdateDragInsertion(const MyGUI::IntPoint& mouse)
    {
        if (!g_drag.active || g_drag.kind != DRAG_REORDER ||
            g_jobViewport == NULL || g_insertionLine == NULL ||
            g_drag.memberIndex < 0 ||
            g_drag.memberIndex >= static_cast<int>(g_squad.members.size()))
        {
            if (g_insertionLine != NULL)
            {
                g_insertionLine->setVisible(false);
            }
            return;
        }

        if (!MouseIsInSourceRow(mouse))
        {
            g_drag.insertionGap = -1;
            g_insertionLine->setVisible(false);
            return;
        }

        const MyGUI::IntCoord view = g_jobViewport->getAbsoluteCoord();
        const int contentX = mouse.left - view.left + g_horizontalOffset;
        const int queueCount =
            static_cast<int>(g_squad.members[g_drag.memberIndex].jobs.size());
        int gap = (contentX + CARD_STRIDE / 2) / CARD_STRIDE;
        gap = ClampInt(gap, 0, queueCount);
        g_drag.insertionGap = gap;

        MyGUI::Widget* client = g_window != NULL ? g_window->getClientWidget() : NULL;
        if (client == NULL)
        {
            return;
        }
        const MyGUI::IntCoord clientCoord = client->getAbsoluteCoord();
        const int lineX = view.left - clientCoord.left + gap * CARD_STRIDE -
            g_horizontalOffset - 2;
        const int lineY = view.top - clientCoord.top +
            g_drag.memberIndex * ROW_STRIDE - g_verticalOffset;
        g_insertionLine->setCoord(lineX, lineY, 4, ROW_HEIGHT);
        g_insertionLine->setVisible(true);
    }

    void StartDragVisuals()
    {
        if (g_removeButton != NULL)
        {
            g_removeButton->setColour(MyGUI::Colour(1.0f, 0.34f, 0.30f));
            g_removeButton->setStateSelected(true);
            std::ostringstream caption;
            caption << "DROP TO REMOVE (" << g_selectedJobs.size() << ")";
            g_removeButton->setCaption(caption.str().c_str());
        }
    }

    void OnCardPressed(
        MyGUI::Widget* widget,
        int,
        int,
        MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left || g_modal.kind != MODAL_NONE)
        {
            return;
        }

        int memberIndex = -1;
        int slot = -1;
        if (!GetCardBinding(widget, &memberIndex, &slot) ||
            !g_squad.members[memberIndex].queueAvailable)
        {
            return;
        }

        const MemberSnapshot& member = g_squad.members[memberIndex];
        const JobRowSnapshot& job = member.jobs[slot];
        MyGUI::InputManager& input = MyGUI::InputManager::getInstance();
        const bool plain = !input.isControlPressed() && !input.isShiftPressed();
        const bool alreadySelected =
            FindSelectedJobIndex(member.identity, job, slot) >= 0;

        g_drag = DragState();
        g_drag.armed = true;
        g_drag.pressPoint = input.getMousePosition();
        g_drag.memberIndex = memberIndex;
        g_drag.slot = slot;
        g_drag.startSequence = member.jobs;
        g_drag.deferredPlainClick =
            plain && alreadySelected && g_selectedJobs.size() > 1;
        g_drag.sourceWasSelected = alreadySelected;

        if (!g_drag.deferredPlainClick)
        {
            ApplyCardClickSelection(memberIndex, slot);
        }
    }

    void OnCardDrag(
        MyGUI::Widget*,
        int,
        int,
        MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left || !g_drag.armed)
        {
            return;
        }
        if (g_drag.memberIndex < 0 ||
            g_drag.memberIndex >= static_cast<int>(g_squad.members.size()) ||
            g_drag.slot < 0 ||
            g_drag.slot >= static_cast<int>(
                g_squad.members[g_drag.memberIndex].jobs.size()) ||
            !SameQueue(
                g_squad.members[g_drag.memberIndex].jobs,
                g_drag.startSequence))
        {
            CancelDrag();
            SetStatus("The queue changed during the drag. Review it and try again.");
            return;
        }
        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        if (!g_drag.active)
        {
            const int dx = std::abs(mouse.left - g_drag.pressPoint.left);
            const int dy = std::abs(mouse.top - g_drag.pressPoint.top);
            if (dx < DRAG_THRESHOLD && dy < DRAG_THRESHOLD)
            {
                return;
            }
            g_drag.active = true;
            g_drag.deferredPlainClick = false;
            ClearJobHoverHighlight();
            const MemberSnapshot& sourceMember =
                g_squad.members[g_drag.memberIndex];
            if (!g_drag.sourceWasSelected ||
                FindSelectedJobIndex(
                    sourceMember.identity,
                    sourceMember.jobs[g_drag.slot],
                    g_drag.slot) < 0)
            {
                SelectOnlyJob(g_drag.memberIndex, g_drag.slot);
            }
            g_drag.kind = g_selectedJobs.size() > 1
                ? DRAG_REMOVE_ONLY
                : DRAG_REORDER;
            StartDragVisuals();
        }
        UpdateDragInsertion(mouse);
    }

    void QueueRemoveSelectedAction()
    {
        if (g_selectedJobs.empty() || g_pendingAction.type != ACTION_NONE)
        {
            return;
        }
        g_pendingAction = PendingAction();
        g_pendingAction.type = ACTION_REMOVE_SELECTED;
    }

    void OnCardReleased(
        MyGUI::Widget*,
        int,
        int,
        MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left || !g_drag.armed)
        {
            return;
        }

        if (g_drag.memberIndex < 0 ||
            g_drag.memberIndex >= static_cast<int>(g_squad.members.size()) ||
            g_drag.slot < 0 ||
            g_drag.slot >= static_cast<int>(
                g_squad.members[g_drag.memberIndex].jobs.size()) ||
            !SameQueue(
                g_squad.members[g_drag.memberIndex].jobs,
                g_drag.startSequence))
        {
            CancelDrag();
            SetStatus("The queue changed during the drag. Review it and try again.");
            return;
        }

        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        if (!g_drag.active)
        {
            if (g_drag.deferredPlainClick)
            {
                SelectOnlyJob(g_drag.memberIndex, g_drag.slot);
            }
            CancelDrag();
            return;
        }

        if (g_removeButton != NULL &&
            PointInside(g_removeButton->getAbsoluteCoord(), mouse))
        {
            QueueRemoveSelectedAction();
            CancelDrag();
            return;
        }

        if (g_drag.kind == DRAG_REORDER && g_drag.insertionGap >= 0 &&
            MouseIsInSourceRow(mouse))
        {
            int target = g_drag.insertionGap;
            if (g_drag.insertionGap > g_drag.slot)
            {
                --target;
            }
            const int count = static_cast<int>(g_drag.startSequence.size());
            target = ClampInt(target, 0, std::max(0, count - 1));
            if (target != g_drag.slot && g_pendingAction.type == ACTION_NONE)
            {
                g_pendingAction = PendingAction();
                g_pendingAction.type = ACTION_REORDER;
                g_pendingAction.member =
                    g_squad.members[g_drag.memberIndex].identity;
                g_pendingAction.job =
                    g_squad.members[g_drag.memberIndex].jobs[g_drag.slot];
                g_pendingAction.sequence = g_drag.startSequence;
                g_pendingAction.targetSlot = target;
            }
        }
        else if (g_drag.kind == DRAG_REMOVE_ONLY)
        {
            SetStatus("Multiple selected jobs can only be dropped on Remove Selected.");
        }

        CancelDrag();
    }

    std::string WrapToolTipCaption(
        const std::string& source,
        size_t maximumCharacters,
        size_t* lineCountOut)
    {
        std::string wrapped;
        size_t lineCount = 0;
        size_t sourcePosition = 0;
        while (sourcePosition <= source.size())
        {
            const size_t newline = source.find('\n', sourcePosition);
            const size_t sourceEnd = newline == std::string::npos ?
                source.size() : newline;
            std::string line = source.substr(
                sourcePosition, sourceEnd - sourcePosition);
            while (line.size() > maximumCharacters)
            {
                size_t split = line.rfind(' ', maximumCharacters);
                if (split == std::string::npos || split == 0)
                {
                    split = maximumCharacters;
                }
                wrapped.append(line.substr(0, split));
                wrapped.push_back('\n');
                ++lineCount;
                size_t next = split;
                while (next < line.size() && line[next] == ' ')
                {
                    ++next;
                }
                line = line.substr(next);
            }
            wrapped.append(line);
            ++lineCount;
            if (newline == std::string::npos)
            {
                break;
            }
            wrapped.push_back('\n');
            sourcePosition = newline + 1;
        }
        if (lineCountOut != NULL)
        {
            *lineCountOut = lineCount;
        }
        return wrapped;
    }

    void OnCardToolTip(MyGUI::Widget* widget, const MyGUI::ToolTipInfo& info)
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL || widget == NULL)
        {
            return;
        }
        if (info.type == MyGUI::ToolTipInfo::Hide)
        {
            if (g_tooltip != NULL)
            {
                g_tooltip->setVisible(false);
            }
            return;
        }
        if (g_tooltip == NULL)
        {
            g_tooltip = gui->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(0, 0, 520, 104),
                MyGUI::Align::Default,
                "ToolTip",
                "KJM_ToolTip");
            g_tooltip->setColour(MyGUI::Colour(0.10f, 0.08f, 0.06f));
            g_tooltip->setNeedMouseFocus(false);
            g_tooltipText = g_tooltip->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(8, 6, 504, 92),
                MyGUI::Align::Stretch,
                "KJM_ToolTipText");
            g_tooltipText->setNeedMouseFocus(false);
        }
        const MyGUI::IntSize view =
            MyGUI::RenderManager::getInstance().getViewSize();
        const int tooltipWidth = ClampInt(520, 240, std::max(240, view.width - 20));
        size_t lineCount = 1;
        if (widget->isUserString("KJM_ToolTip") && g_tooltipText != NULL)
        {
            const size_t maximumCharacters = static_cast<size_t>(std::max(
                28, (tooltipWidth - 16) / 7));
            const std::string caption = WrapToolTipCaption(
                widget->getUserString("KJM_ToolTip"),
                maximumCharacters,
                &lineCount);
            g_tooltipText->setCaption(caption);
        }
        const int tooltipHeight = ClampInt(
            12 + static_cast<int>(lineCount) * 16,
            44,
            std::max(44, view.height - 20));
        g_tooltip->setSize(tooltipWidth, tooltipHeight);
        if (g_tooltipText != NULL)
        {
            g_tooltipText->setCoord(
                8, 6, tooltipWidth - 16, tooltipHeight - 12);
        }
        const int x = ClampInt(
            info.point.left + 16, 0, std::max(0, view.width - tooltipWidth));
        const int y = ClampInt(
            info.point.top + 20, 0, std::max(0, view.height - tooltipHeight));
        g_tooltip->setPosition(x, y);
        g_tooltip->setVisible(true);
    }

    void OnVerticalScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_changingScroll)
        {
            return;
        }
        ClearJobHoverHighlight();
        g_verticalOffset = ClampInt(
            static_cast<int>(position), 0, g_maxVerticalOffset);
        ApplyScrollOffsets();
        StoreCurrentSquadScrollOffsets();
    }

    void OnHorizontalScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_changingScroll)
        {
            return;
        }
        ClearJobHoverHighlight();
        g_horizontalOffset = ClampInt(
            static_cast<int>(position), 0, g_maxHorizontalOffset);
        ApplyScrollOffsets();
        StoreCurrentSquadScrollOffsets();
        if (g_drag.active)
        {
            UpdateDragInsertion(MyGUI::InputManager::getInstance().getMousePosition());
        }
    }

    void OnMouseWheel(MyGUI::Widget*, int relative)
    {
        if (relative == 0 || g_window == NULL || g_stationTabActive ||
            g_modal.kind != MODAL_NONE || IsStationDetailOpen())
        {
            return;
        }

        ClearJobHoverHighlight();

        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        const bool shift =
            (input != NULL && input->isShiftPressed()) ||
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shift)
        {
            g_horizontalOffset = ClampInt(
                g_horizontalOffset - relative * 40,
                0,
                g_maxHorizontalOffset);
            g_changingScroll = true;
            if (g_horizontalScroll != NULL)
            {
                g_horizontalScroll->setScrollPosition(
                    static_cast<size_t>(g_horizontalOffset));
            }
            g_changingScroll = false;
            ApplyScrollOffsets();
            StoreCurrentSquadScrollOffsets();
            if (g_drag.active && input != NULL)
            {
                UpdateDragInsertion(input->getMousePosition());
            }
            return;
        }

        g_verticalOffset = ClampInt(
            g_verticalOffset - relative * 40,
            0,
            g_maxVerticalOffset);
        g_changingScroll = true;
        if (g_verticalScroll != NULL)
        {
            g_verticalScroll->setScrollPosition(
                static_cast<size_t>(g_verticalOffset));
        }
        g_changingScroll = false;
        ApplyScrollOffsets();
        StoreCurrentSquadScrollOffsets();
    }

    void BindSquadMouseWheelTree(MyGUI::Widget* widget)
    {
        if (widget == NULL)
        {
            return;
        }

        // The bottom squad strip keeps plain wheel input routed to the member
        // rows, but owns Shift+wheel for its independent horizontal overflow.
        // Its dedicated binder handles that subtree exactly once.
        if (widget->isUserString("KJM_SquadSelectorWheelRoot"))
        {
            return;
        }

        // ScrollBar skins contain their own button/track widgets. Those
        // children forward wheel input to the native scrollbar, which would
        // otherwise move its native axis in addition to our single routed
        // Squad Jobs update. Disable the native wheel step, then bind both
        // the scrollbar and its picked skin children to our routed handler.
        if (widget->isType<MyGUI::ScrollBar>())
        {
            MyGUI::ScrollBar* scrollbar =
                widget->castType<MyGUI::ScrollBar>(false);
            if (scrollbar != NULL)
            {
                scrollbar->setScrollWheelPage(0);
            }
        }

        // MyGUI 3.2 sends wheel input only to the picked widget. It does not
        // bubble the event to a parent. Bind every current Squad Jobs child
        // once so buttons, card contents, blank panels, and scrollbars all
        // use the same vertical/Shift-horizontal routing.
        if (!widget->isUserString("KJM_SquadWheelBound"))
        {
            widget->eventMouseWheel += MyGUI::newDelegate(OnMouseWheel);
            widget->setUserString("KJM_SquadWheelBound", "1");
        }

        const size_t childCount = widget->getChildCount();
        for (size_t index = 0; index < childCount; ++index)
        {
            BindSquadMouseWheelTree(widget->getChildAt(index));
        }
    }

    void OnOptionsMouseWheel(MyGUI::Widget*, int relative)
    {
        if (g_modal.kind != MODAL_OPTIONS || g_optionsScroll == NULL ||
            relative == 0)
        {
            return;
        }

        try
        {
            const MyGUI::IntSize canvas = g_optionsScroll->getCanvasSize();
            const MyGUI::IntCoord view = g_optionsScroll->getViewCoord();
            const int maxOffset = std::max(0, canvas.height - view.height);
            // ScrollView exposes the canvas position, which is zero at the
            // top and negative while the content moves upward.  Keep the
            // arithmetic in a positive scroll-distance coordinate, then
            // convert it back for setViewOffset().
            const MyGUI::IntPoint current = g_optionsScroll->getViewOffset();
            const int currentDistance = ClampInt(
                -current.top, 0, maxOffset);
            const int nextDistance = ClampInt(
                currentDistance - relative * 40, 0, maxOffset);
            if (nextDistance != currentDistance)
            {
                g_optionsScroll->setViewOffset(
                    MyGUI::IntPoint(current.left, -nextDistance));
            }
        }
        catch (...)
        {
            // Modal input must remain fail-closed if MyGUI is shutting down.
        }
    }

    void CloseModalNow()
    {
        g_optionsScroll = NULL;
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            try
            {
                if (g_modal.window != NULL)
                {
                    MyGUI::InputManager* input =
                        MyGUI::InputManager::getInstancePtr();
                    if (input != NULL)
                    {
                        input->removeWidgetModal(g_modal.window);
                    }
                    gui->destroyWidget(g_modal.window);
                }
                if (g_modal.shade != NULL)
                {
                    gui->destroyWidget(g_modal.shade);
                }
                MyGUI::InputManager* input =
                    MyGUI::InputManager::getInstancePtr();
                if (input != NULL && g_window != NULL)
                {
                    input->setKeyFocusWidget(g_window);
                }
            }
            catch (...)
            {
                ErrorLog("[KenshiJobManagement] MyGUI threw while closing a modal.");
            }
        }
        g_modal = ModalState();
        g_stationOptionButtons.clear();
        g_modalCloseRequested = false;
    }

    bool BeginModal(ModalKind kind, int width, int height, const char* caption)
    {
        if (g_modal.kind != MODAL_NONE || g_window == NULL)
        {
            return false;
        }
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
        {
            return false;
        }
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        width = std::min(width, std::max(320, view.width - 40));
        height = std::min(height, std::max(180, view.height - 40));
        g_modal.shade = gui->createWidget<MyGUI::Widget>(
            "WhiteSkin",
            MyGUI::IntCoord(0, 0, view.width, view.height),
            MyGUI::Align::Stretch,
            "Popup",
            "KJM_ModalShade");
        g_modal.shade->setColour(MyGUI::Colour(0.0f, 0.0f, 0.0f, 0.72f));
        g_modal.shade->setNeedMouseFocus(true);

        g_modal.window = gui->createWidget<MyGUI::Window>(
            "Kenshi_WindowCX",
            MyGUI::IntCoord(
                (view.width - width) / 2,
                (view.height - height) / 2,
                width,
                height),
            MyGUI::Align::Center,
            "Popup",
            "KJM_ModalWindow");
        g_modal.window->setMovable(false);
        g_modal.window->setCaption(caption);
        g_modal.kind = kind;
        MyGUI::InputManager::getInstance().addWidgetModal(g_modal.window);
        return true;
    }

    void OpenClearModal(int memberIndex)
    {
        if (memberIndex < 0 || memberIndex >= static_cast<int>(g_squad.members.size()))
        {
            return;
        }
        const MemberSnapshot& member = g_squad.members[memberIndex];
        if (!member.queueAvailable || member.jobs.empty() ||
            !BeginModal(MODAL_CLEAR, 520, 210, "Confirm Clear Queue"))
        {
            return;
        }

        g_modal.member = member.identity;
        g_modal.reviewedQueue = member.jobs;
        MyGUI::Widget* client = g_modal.window->getClientWidget();
        MyGUI::TextBox* prompt = client->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(18, 18, 484, 78),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_ClearPrompt");
        std::ostringstream message;
        message << "Are you sure you wish to remove all jobs?\n\nRemove all "
                << member.jobs.size() << " permanent jobs from " << member.name << "?";
        prompt->setCaption(message.str().c_str());
        prompt->setTextAlign(MyGUI::Align::Center);
        prompt->setNeedMouseFocus(false);

        MyGUI::Button* yes = client->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(88, 115, 150, 38),
            MyGUI::Align::Left | MyGUI::Align::Bottom,
            "KJM_ClearYes");
        yes->setCaption("Yes");
        yes->eventMouseButtonClick += MyGUI::newDelegate(OnClearYes);
        MyGUI::Button* no = client->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(282, 115, 150, 38),
            MyGUI::Align::Right | MyGUI::Align::Bottom,
            "KJM_ClearNo");
        no->setCaption("No");
        no->eventMouseButtonClick += MyGUI::newDelegate(OnClearNo);
        MyGUI::InputManager::getInstance().setKeyFocusWidget(no);
    }

    void OnClearYes(MyGUI::Widget*)
    {
        if (g_modal.kind != MODAL_CLEAR || g_pendingAction.type != ACTION_NONE)
        {
            return;
        }
        g_pendingAction = PendingAction();
        g_pendingAction.type = ACTION_CLEAR_MEMBER;
        g_pendingAction.member = g_modal.member;
        g_pendingAction.sequence = g_modal.reviewedQueue;
        g_modalCloseRequested = true;
    }

    void OnClearNo(MyGUI::Widget*)
    {
        g_modalCloseRequested = true;
    }

    void RefreshOptionButtons()
    {
        for (size_t index = 0; index < g_stationOptionButtons.size(); ++index)
        {
            g_stationOptionButtons[index].button->setStateSelected(
                IsStationCategoryEnabled(
                g_stationOptionButtons[index].category));
        }
    }

    void OnStationCategoryOptionClicked(MyGUI::Widget* widget)
    {
        for (size_t index = 0; index < g_stationOptionButtons.size(); ++index)
        {
            StationCategoryOptionButton& binding =
                g_stationOptionButtons[index];
            if (binding.button != widget)
            {
                continue;
            }
            const int category = static_cast<int>(binding.category);
            g_stationCategoryEnabled[category] =
                !g_stationCategoryEnabled[category];
            if (!SaveStationCategorySettings())
            {
                g_settingsWriteFailed = true;
            }
            RefreshOptionButtons();
            g_stationFilterRefreshRequested = true;
            return;
        }
    }

    void OnOptionReset(MyGUI::Widget*)
    {
        ResetDefaultStationCategories();
        if (!SaveStationCategorySettings())
        {
            g_settingsWriteFailed = true;
        }
        g_stationFilterRefreshRequested = true;
        RefreshOptionButtons();
    }

    void OnOptionClose(MyGUI::Widget*)
    {
        g_modalCloseRequested = true;
    }

    void OpenOptionsModal()
    {
        if (!BeginModal(MODAL_OPTIONS, 900, 460, "Job Manager Options"))
        {
            return;
        }
        MyGUI::Widget* client = g_modal.window->getClientWidget();
        const MyGUI::IntSize clientSize = client->getSize();
        MyGUI::TextBox* help = client->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(16, 8, clientSize.width - 32, 44),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_OptionsHelp");
        help->setCaption(
            "Station category filters apply only to Stations.\nSquad Jobs always shows every permanent job and selects each member's top three from all supported base stats. Changes save globally now.");
        help->setTextAlign(MyGUI::Align::Center);
        help->setNeedMouseFocus(false);

        MyGUI::ScrollView* scroll = client->createWidget<MyGUI::ScrollView>(
            "Kenshi_ScrollView",
            MyGUI::IntCoord(
                16,
                56,
                clientSize.width - 32,
                std::max(180, clientSize.height - 122)),
            MyGUI::Align::Stretch,
            "KJM_OptionsScroll");
        scroll->setVisibleVScroll(true);
        scroll->setVisibleHScroll(false);
        scroll->eventMouseWheel += MyGUI::newDelegate(OnOptionsMouseWheel);
        g_optionsScroll = scroll;

        const int canvasWidth = std::max(240, clientSize.width - 60);
        const int columnGap = 16;
        const int columnWidth = std::max(
            100, (canvasWidth - 16 - columnGap) / 2);
        const int leftColumn = 8;
        const int rightColumn = leftColumn + columnWidth + columnGap;

        MyGUI::TextBox* stationSection = scroll->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(leftColumn, 8, canvasWidth, 30),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationFilterSection");
        stationSection->setCaption("STATION CATEGORY FILTERS");
        stationSection->setTextColour(MyGUI::Colour(1.0f, 0.84f, 0.45f));
        stationSection->setNeedMouseFocus(false);

        for (size_t index = 0;
             index < STATION_CATEGORY_DEFINITION_COUNT; ++index)
        {
            const StationCategoryDefinition& definition =
                STATION_CATEGORY_DEFINITIONS[index];
            const size_t column = index / 5;
            const size_t row = index % 5;
            const int x = column == 0 ? leftColumn : rightColumn;
            const int y = 46 + static_cast<int>(row) * 40;
            MyGUI::Button* category = scroll->createWidget<MyGUI::Button>(
                "Kenshi_TickButton1",
                MyGUI::IntCoord(
                    x, y, columnWidth, 34),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationCategoryOption");
            category->setCaption(definition.name);
            category->eventMouseButtonClick +=
                MyGUI::newDelegate(OnStationCategoryOptionClicked);
            category->eventMouseWheel +=
                MyGUI::newDelegate(OnOptionsMouseWheel);
            StationCategoryOptionButton binding;
            binding.button = category;
            binding.category = definition.category;
            g_stationOptionButtons.push_back(binding);
        }
        scroll->setCanvasSize(
            canvasWidth, 46 + 5 * 40 + 8);

        MyGUI::Button* reset = client->createWidget<MyGUI::Button>(
            "Kenshi_Button1", MyGUI::IntCoord(16, clientSize.height - 50, 160, 38),
            MyGUI::Align::Left | MyGUI::Align::Bottom, "KJM_OptionReset");
        reset->setCaption("Reset Default");
        reset->eventMouseButtonClick += MyGUI::newDelegate(OnOptionReset);
        MyGUI::Button* close = client->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(clientSize.width - 156, clientSize.height - 50, 140, 38),
            MyGUI::Align::Right | MyGUI::Align::Bottom, "KJM_OptionClose");
        close->setCaption("Close");
        close->eventMouseButtonClick += MyGUI::newDelegate(OnOptionClose);
        RefreshOptionButtons();
        MyGUI::InputManager::getInstance().setKeyFocusWidget(close);
    }

    void OnJobsToggleClicked(MyGUI::Widget* widget)
    {
        int memberIndex = -1;
        if (!GetWidgetIndex(widget, "KJM_Member", &memberIndex) ||
            memberIndex < 0 || memberIndex >= static_cast<int>(g_squad.members.size()) ||
            !g_squad.members[memberIndex].queueAvailable ||
            g_pendingAction.type != ACTION_NONE)
        {
            return;
        }
        g_pendingAction = PendingAction();
        g_pendingAction.type = ACTION_TOGGLE_JOBS;
        g_pendingAction.member = g_squad.members[memberIndex].identity;
    }

    void OnClearClicked(MyGUI::Widget* widget)
    {
        int memberIndex = -1;
        if (GetWidgetIndex(widget, "KJM_Member", &memberIndex))
        {
            OpenClearModal(memberIndex);
        }
    }

    void OnRemoveClicked(MyGUI::Widget*)
    {
        QueueRemoveSelectedAction();
    }

    void OnOptionsClicked(MyGUI::Widget*)
    {
        OpenOptionsModal();
    }

    struct RemovalCandidate
    {
        SelectedJob selected;
        int memberIndex;
        int slot;
    };

    bool RemovalCandidateLess(
        const RemovalCandidate& left,
        const RemovalCandidate& right)
    {
        if (left.memberIndex != right.memberIndex)
        {
            return left.memberIndex < right.memberIndex;
        }
        return left.slot > right.slot;
    }

    std::string DescribeRemovalCandidate(const RemovalCandidate& candidate)
    {
        std::ostringstream text;
        if (candidate.memberIndex >= 0 &&
            candidate.memberIndex < static_cast<int>(g_squad.members.size()))
        {
            text << g_squad.members[candidate.memberIndex].name;
        }
        else
        {
            text << "Unavailable member";
        }
        text << " / " << candidate.selected.job.jobLabel;
        if (candidate.slot >= 0)
        {
            text << " / priority " << (candidate.slot + 1);
        }
        return text.str();
    }

    void EraseSelectedJob(const SelectedJob& completed)
    {
        const int index = FindSelectedJobIndex(
            completed.member,
            completed.job,
            completed.lastSlot);
        if (index >= 0)
        {
            g_selectedJobs.erase(g_selectedJobs.begin() + index);
        }
    }

    void ProcessRemoveSelected()
    {
        const HandleIdentity batchSquad = g_squad.identity;
        std::vector<RemovalCandidate> candidates;
        for (size_t index = 0; index < g_selectedJobs.size(); ++index)
        {
            const int memberIndex = FindMemberIndex(g_selectedJobs[index].member);
            int slot = -1;
            if (memberIndex >= 0)
            {
                slot = FindJobSlot(
                    g_squad.members[memberIndex],
                    g_selectedJobs[index].job);
                if (g_selectedJobs[index].job.taskToken == 0)
                {
                    slot = g_selectedJobs[index].lastSlot;
                }
                else if (slot < 0)
                {
                    slot = g_selectedJobs[index].lastSlot;
                }
            }
            RemovalCandidate candidate;
            candidate.selected = g_selectedJobs[index];
            candidate.memberIndex = memberIndex;
            candidate.slot = slot;
            candidates.push_back(candidate);
        }
        std::sort(candidates.begin(), candidates.end(), RemovalCandidateLess);

        size_t completedCount = 0;
        bool interrupted = false;
        std::string failedDetail;
        for (size_t index = 0; index < candidates.size(); ++index)
        {
            RemovalCandidate& candidate = candidates[index];
            if (candidate.memberIndex < 0 || candidate.slot < 0)
            {
                interrupted = true;
                failedDetail = DescribeRemovalCandidate(candidate);
                break;
            }
            MemberSnapshot fresh;
            if (!TryRefreshMemberByIdentity(candidate.selected.member, &fresh))
            {
                interrupted = true;
                failedDetail = DescribeRemovalCandidate(candidate);
                break;
            }

            int liveSlot = FindJobSlot(fresh, candidate.selected.job);
            if (candidate.selected.job.taskToken == 0)
            {
                liveSlot = candidate.slot;
                if (liveSlot < 0 || liveSlot >= static_cast<int>(fresh.jobs.size()) ||
                    !SameJob(fresh.jobs[liveSlot], candidate.selected.job))
                {
                    liveSlot = -1;
                }
            }
            if (liveSlot < 0 || !TryRemovePermajob(fresh.handle, liveSlot))
            {
                interrupted = true;
                failedDetail = DescribeRemovalCandidate(candidate);
                break;
            }
            EraseSelectedJob(candidate.selected);
            ++completedCount;
        }

        const std::vector<SelectedJob> remaining = g_selectedJobs;
        if (completedCount > 0)
        {
            TryNotifyVanillaSelectionUI();
        }
        RefreshSquadView(true);
        if (interrupted && SameHandleIdentity(batchSquad, g_squad.identity))
        {
            // Keep the failed and not-yet-attempted rows selected, including
            // logical selections that became temporarily unavailable.
            for (size_t index = 0; index < remaining.size(); ++index)
            {
                if (FindSelectedJobIndex(
                        remaining[index].member,
                        remaining[index].job,
                        remaining[index].lastSlot) < 0)
                {
                    g_selectedJobs.push_back(remaining[index]);
                }
            }
            ApplyCardSelectionStates();
        }
        std::ostringstream status;
        if (interrupted)
        {
            status << "Deletion queue interrupted after " << completedCount
                   << " removal" << (completedCount == 1 ? "" : "s")
                   << ". Failed: " << failedDetail
                   << ". " << remaining.size() << " job"
                   << (remaining.size() == 1 ? " remains" : "s remain")
                   << " selected. Review and try again.";
        }
        else
        {
            status << "Removed " << completedCount << " permanent job"
                   << (completedCount == 1 ? "" : "s") << ".";
        }
        SetStatus(status.str());
    }

    bool SameStationJobKindAndTarget(
        const JobRowSnapshot& job,
        TaskType taskType,
        const HandleIdentity& target)
    {
        return job.taskType == taskType && job.hasTarget &&
            SameHandleIdentity(job.target, target);
    }

    bool HasStationJobKindAndTarget(
        const std::vector<JobRowSnapshot>& jobs,
        TaskType taskType,
        const HandleIdentity& target)
    {
        for (size_t index = 0; index < jobs.size(); ++index)
        {
            if (SameStationJobKindAndTarget(jobs[index], taskType, target))
            {
                return true;
            }
        }
        return false;
    }

    bool TryBuildLiveMemberByIdentity(
        const HandleIdentity& identity,
        MemberSnapshot* memberOut)
    {
        if (memberOut == NULL || !identity.valid ||
            identity.type != CHARACTER)
        {
            return false;
        }
        const hand member = RestoreHandleIdentity(identity);
        MemberSnapshot fresh;
        if (!BuildMemberSnapshot(member, &fresh) ||
            !fresh.loaded || !fresh.queueAvailable || fresh.truncated ||
            !SameHandleIdentity(fresh.identity, identity))
        {
            return false;
        }
        *memberOut = fresh;
        return true;
    }

    bool TryAddStationPermanentJob(
        const hand& member,
        TaskType taskType,
        const HandleIdentity& targetIdentity)
    {
        const hand target = RestoreHandleIdentity(targetIdentity);
        Building* building = NULL;
        if (targetIdentity.type != BUILDING ||
            !TryResolveStationBuilding(target, &building) || building == NULL)
        {
            return false;
        }
        bool playerManaged = false;
        if (!TryIsPlayerManagedStation(building, true, &playerManaged) ||
            !playerManaged)
        {
            return false;
        }

        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            const Ogre::Vector3 position = building->getPosition();
            // These flags reproduce Kenshi's permanent shift-add path. The
            // destination queue is appended without clearing existing jobs.
            character->addJob(taskType, building, true, true, position);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool SameQueueWithAppendedStationJob(
        const std::vector<JobRowSnapshot>& before,
        const std::vector<JobRowSnapshot>& after,
        TaskType taskType,
        const HandleIdentity& target)
    {
        if (after.size() != before.size() + 1)
        {
            return false;
        }
        for (size_t index = 0; index < before.size(); ++index)
        {
            if (!SameJob(before[index], after[index]) ||
                before[index].jobLabel != after[index].jobLabel ||
                before[index].targetLabel != after[index].targetLabel ||
                before[index].hasTarget != after[index].hasTarget ||
                before[index].targetAvailable != after[index].targetAvailable)
            {
                return false;
            }
        }
        return SameStationJobKindAndTarget(after.back(), taskType, target);
    }

    bool SameQueueWithExactJobRemoved(
        const std::vector<JobRowSnapshot>& before,
        int removedSlot,
        const std::vector<JobRowSnapshot>& after)
    {
        if (removedSlot < 0 ||
            removedSlot >= static_cast<int>(before.size()) ||
            after.size() + 1 != before.size())
        {
            return false;
        }
        for (size_t index = 0; index < after.size(); ++index)
        {
            const size_t beforeIndex = index < static_cast<size_t>(removedSlot) ?
                index : index + 1;
            if (!SameJob(after[index], before[beforeIndex]) ||
                after[index].jobLabel != before[beforeIndex].jobLabel ||
                after[index].targetLabel != before[beforeIndex].targetLabel ||
                after[index].hasTarget != before[beforeIndex].hasTarget ||
                after[index].targetAvailable !=
                    before[beforeIndex].targetAvailable)
            {
                return false;
            }
        }
        return true;
    }

    void SetStationActionStatus(const std::string& status)
    {
        SetStatus(status);
        SetStationDetailStatus(status);
    }

    bool TryCaptureProjectedStationMemberQueue(
        const HandleIdentity& station,
        const HandleIdentity& member,
        std::vector<JobRowSnapshot>* queueOut)
    {
        if (queueOut == NULL || !station.valid || station.type != BUILDING ||
            !member.valid || member.type != CHARACTER ||
            !g_stationScan.started)
        {
            return false;
        }
        queueOut->clear();

        bool stationFound = false;
        for (size_t stationIndex = 0;
             stationIndex < g_stationScan.stations.size(); ++stationIndex)
        {
            if (SameHandleIdentity(
                    g_stationScan.stations[stationIndex].identity, station))
            {
                if (stationFound)
                {
                    return false;
                }
                stationFound = true;
            }
        }
        if (!stationFound)
        {
            return false;
        }

        const StationMemberSnapshot* projectedMember = NULL;
        for (size_t squadIndex = 0;
             squadIndex < g_stationScan.squads.size(); ++squadIndex)
        {
            const StationSquadSnapshot& squad =
                g_stationScan.squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const StationMemberSnapshot& candidate =
                    squad.members[memberIndex];
                if (!SameHandleIdentity(candidate.identity, member))
                {
                    continue;
                }
                if (projectedMember != NULL)
                {
                    return false;
                }
                projectedMember = &candidate;
            }
        }
        if (projectedMember == NULL || !projectedMember->loaded ||
            !projectedMember->queueAvailable || projectedMember->truncated ||
            projectedMember->permanentJobCount < 0 ||
            static_cast<size_t>(projectedMember->permanentJobCount) !=
                projectedMember->jobs.size() ||
            projectedMember->jobs.size() >
                static_cast<size_t>(MAX_SAFE_JOB_ROWS))
        {
            return false;
        }

        queueOut->reserve(projectedMember->jobs.size());
        for (size_t index = 0; index < projectedMember->jobs.size(); ++index)
        {
            queueOut->push_back(projectedMember->jobs[index].exactJob);
        }
        return true;
    }

    bool TryGetAssignedNaturalAddAuthorization(
        const HandleIdentity& station,
        bool* allowAssignedNaturalExceptionOut)
    {
        if (allowAssignedNaturalExceptionOut == NULL ||
            !station.valid || station.type != BUILDING ||
            !g_stationScan.started)
        {
            return false;
        }
        *allowAssignedNaturalExceptionOut = false;

        const StationTargetSnapshot* matchedStation = NULL;
        for (size_t stationIndex = 0;
             stationIndex < g_stationScan.stations.size(); ++stationIndex)
        {
            const StationTargetSnapshot& candidate =
                g_stationScan.stations[stationIndex];
            if (!SameHandleIdentity(candidate.identity, station))
            {
                continue;
            }
            if (matchedStation != NULL)
            {
                return false;
            }
            matchedStation = &candidate;
        }
        if (matchedStation == NULL)
        {
            return false;
        }

        // A non-player natural node is actionable only while a readable
        // loaded-player queue still proves that Kenshi already assigned it.
        // Player-owned buildings do not need this exception; the guarded
        // native leaf checks their ownership directly.
        *allowAssignedNaturalExceptionOut =
            matchedStation->naturalResourceException &&
            !matchedStation->assignments.empty();
        return true;
    }

    void RequestAddStationAssignment(
        const HandleIdentity& station,
        const HandleIdentity& member)
    {
        if (g_pendingAction.type != ACTION_NONE)
        {
            SetStationActionStatus(
                "Another job change is pending. Try again after it finishes.");
            return;
        }

        std::vector<JobRowSnapshot> capturedQueue;
        if (!TryCaptureProjectedStationMemberQueue(
                station, member, &capturedQueue))
        {
            SetStationActionStatus(
                "The member queue or station changed. No jobs were changed.");
            return;
        }

        g_pendingAction = PendingAction();
        g_pendingAction.type = ACTION_ASSIGN_STATION;
        g_pendingAction.member = member;
        g_pendingAction.stationTarget = station;
        g_pendingAction.sequence = capturedQueue;
        SetStationActionStatus("Validating the station assignment...");
    }

    void RequestRemoveStationAssignment(
        const HandleIdentity& station,
        const HandleIdentity& member)
    {
        if (g_pendingAction.type != ACTION_NONE)
        {
            SetStationActionStatus(
                "Another job change is pending. Try again after it finishes.");
            return;
        }

        std::vector<JobRowSnapshot> capturedQueue;
        if (!TryCaptureProjectedStationMemberQueue(
                station, member, &capturedQueue))
        {
            SetStationActionStatus(
                "The member queue or station changed. No jobs were changed.");
            return;
        }

        g_pendingAction = PendingAction();
        g_pendingAction.type = ACTION_REMOVE_STATION_BUNDLE;
        g_pendingAction.member = member;
        g_pendingAction.stationTarget = station;
        g_pendingAction.sequence = capturedQueue;
        SetStationActionStatus("Validating all jobs for this station...");
    }

    bool SameStationQueuePrefix(
        const std::vector<JobRowSnapshot>& before,
        const std::vector<JobRowSnapshot>& after)
    {
        if (after.size() < before.size())
        {
            return false;
        }
        std::vector<JobRowSnapshot> afterPrefix(
            after.begin(), after.begin() + before.size());
        return SameQueue(before, afterPrefix);
    }

    bool IsExpectedStationAssignmentSuffix(
        const std::vector<JobRowSnapshot>& before,
        const std::vector<JobRowSnapshot>& after,
        TaskType normalizedTask,
        bool automaticBundle,
        const HandleIdentity& station)
    {
        if (!SameStationQueuePrefix(before, after))
        {
            return false;
        }
        const size_t suffixCount = after.size() - before.size();
        if (automaticBundle)
        {
            if (suffixCount != 2)
            {
                return false;
            }
            const JobRowSnapshot& first = after[before.size()];
            const JobRowSnapshot& second = after[before.size() + 1];
            return SameStationJobKindAndTarget(
                    first, normalizedTask, station) &&
                SameStationJobKindAndTarget(
                    second, OPERATE_STORAGE, station);
        }
        if (suffixCount != 1)
        {
            return false;
        }
        return SameStationJobKindAndTarget(
            after[before.size()], normalizedTask, station);
    }

    bool TryPatchStationActionProjection(
        const MemberSnapshot& memberAfter,
        const HandleIdentity& station,
        bool* naturalBecameUnassignedOut)
    {
        if (naturalBecameUnassignedOut == NULL)
        {
            return false;
        }
        *naturalBecameUnassignedOut = false;
        if (!g_stationScan.started || !memberAfter.identity.valid ||
            !station.valid)
        {
            return false;
        }
        StationMemberSnapshot* projectedMember = NULL;
        for (size_t squadIndex = 0;
             squadIndex < g_stationScan.squads.size(); ++squadIndex)
        {
            StationSquadSnapshot& squad = g_stationScan.squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                StationMemberSnapshot& candidate = squad.members[memberIndex];
                if (SameHandleIdentity(
                        candidate.identity, memberAfter.identity))
                {
                    if (projectedMember != NULL)
                    {
                        return false;
                    }
                    projectedMember = &candidate;
                }
            }
        }
        StationTargetSnapshot* projectedStation = NULL;
        for (size_t stationIndex = 0;
             stationIndex < g_stationScan.stations.size(); ++stationIndex)
        {
            StationTargetSnapshot& candidate =
                g_stationScan.stations[stationIndex];
            if (SameHandleIdentity(candidate.identity, station))
            {
                if (projectedStation != NULL)
                {
                    return false;
                }
                projectedStation = &candidate;
            }
        }
        if (projectedMember == NULL || projectedStation == NULL)
        {
            return false;
        }

        CopyMemberQueueIntoStationProjection(memberAfter, projectedMember);
        JoinStationAssignments(g_stationScan.squads, projectedStation);
        *naturalBecameUnassignedOut =
            projectedStation->naturalResourceException &&
            projectedStation->assignments.empty();
        return true;
    }

    void FinishStationMemberAction(
        const MemberSnapshot& memberAfter,
        const HandleIdentity& station,
        const std::string& status,
        bool fullyVerified)
    {
        bool naturalBecameUnassigned = false;
        const bool projectionPatched = fullyVerified &&
            TryPatchStationActionProjection(
                memberAfter, station, &naturalBecameUnassigned);
        TryNotifyVanillaSelectionUI();
        RefreshSquadView(true);
        if (projectionPatched && !naturalBecameUnassigned)
        {
            // RefreshSquadView observes the verified queue change and marks
            // the station join dirty. The value projection above is already
            // exact, so clear that flag after the Squad refresh.
            g_stationAssignmentsDirty = false;
            // The Stations view integration treats equal source/destination
            // identities as one affected row for add/remove actions.
            g_stationProjectionRefreshSource = memberAfter.identity;
            g_stationProjectionRefreshDestination = memberAfter.identity;
            g_stationProjectionRefreshRequested = true;
        }
        else
        {
            // Partial, unexpected, or unreadable results must rebuild from
            // fresh queues. An unassigned natural target also needs a full
            // target rescan so it disappears from the station inventory.
            g_stationAssignmentsDirty = g_stationScan.started;
        }
        if (fullyVerified)
        {
            MarkStationDetailChange(station, memberAfter.identity);
        }
        SetStationActionStatus(status);
    }

    void FinishUnverifiedStationMemberAction(
        const HandleIdentity& station,
        const HandleIdentity& member,
        const std::string& status)
    {
        (void)station;
        (void)member;
        TryNotifyVanillaSelectionUI();
        RefreshSquadView(true);
        g_stationAssignmentsDirty = g_stationScan.started;
        SetStationActionStatus(status);
    }

    void ProcessStationAssignmentRequest(const PendingAction& action)
    {
        MemberSnapshot before;
        if (!TryBuildLiveMemberByIdentity(action.member, &before) ||
            !SameQueue(before.jobs, action.sequence))
        {
            FinishUnverifiedStationMemberAction(
                action.stationTarget, action.member,
                "Station assignment stopped: 0 job changes were verified. The member queue changed or became unavailable.");
            return;
        }

        bool allowAssignedNaturalException = false;
        if (!TryGetAssignedNaturalAddAuthorization(
                action.stationTarget,
                &allowAssignedNaturalException))
        {
            FinishUnverifiedStationMemberAction(
                action.stationTarget, action.member,
                "Station assignment stopped: 0 job changes were verified. The station projection became unavailable.");
            return;
        }

        // Reacquire the exact full queue again after projection
        // authorization and immediately before the native add leaf. This
        // keeps the pre-call fingerprint boundary as narrow as possible.
        MemberSnapshot immediatelyBeforeAdd;
        if (!TryBuildLiveMemberByIdentity(
                action.member, &immediatelyBeforeAdd) ||
            !SameQueue(immediatelyBeforeAdd.jobs, before.jobs))
        {
            FinishUnverifiedStationMemberAction(
                action.stationTarget, action.member,
                "Station assignment stopped: 0 job changes were verified. The member queue changed immediately before assignment.");
            return;
        }

        TaskType normalizedTask = NULL_TASK;
        bool automaticBundle = false;
        const StationAddJobResult addResult =
            TryResolveAndAddStationJobOnce(
                action.member, action.stationTarget, before.jobs.size(),
                allowAssignedNaturalException,
                &normalizedTask, &automaticBundle);
        if (addResult == STATION_ADD_INVALID_TARGET)
        {
            SetStationActionStatus(
                "The member or exact station is unavailable or is no longer player-managed. No jobs were changed.");
            return;
        }
        if (addResult == STATION_ADD_UNSUPPORTED_TASK)
        {
            SetStationActionStatus(
                "This station does not expose a supported permanent job. No jobs were changed.");
            return;
        }
        if (addResult == STATION_ADD_QUEUE_FULL)
        {
            SetStationActionStatus(
                "The member queue is too full for this station job. No jobs were changed.");
            return;
        }

        MemberSnapshot after;
        if (!TryBuildLiveMemberByIdentity(action.member, &after))
        {
            FinishUnverifiedStationMemberAction(
                action.stationTarget, action.member,
                "The station job was sent, but 0 job changes could be verified because the queue became unavailable. Review the member queue.");
            return;
        }
        const bool expectedSuffix = IsExpectedStationAssignmentSuffix(
                before.jobs, after.jobs, normalizedTask,
                automaticBundle, action.stationTarget);
        if (!expectedSuffix)
        {
            FinishStationMemberAction(
                after, action.stationTarget,
                "Kenshi changed the queue in an unexpected way. 0 station job changes were verified. Review the member queue before another change.",
                false);
            return;
        }

        if (addResult == STATION_ADD_FAULTED)
        {
            if (SameQueue(before.jobs, after.jobs))
            {
                SetStationActionStatus(
                    "Kenshi rejected the station job. No jobs were changed.");
            }
            else
            {
                FinishStationMemberAction(
                    after, action.stationTarget,
                    "Kenshi reported an error after changing the queue. The exact station job change was verified.",
                    true);
            }
            return;
        }

        const size_t addedCount = after.jobs.size() - before.jobs.size();
        std::ostringstream status;
        status << "Assigned " << after.name << " to this station ("
               << addedCount << " permanent job"
               << (addedCount == 1 ? "" : "s") << ").";
        FinishStationMemberAction(
            after, action.stationTarget, status.str(), true);
    }

    void ProcessStationBundleRemovalRequest(const PendingAction& action)
    {
        MemberSnapshot current;
        if (!TryBuildLiveMemberByIdentity(action.member, &current) ||
            !SameQueue(current.jobs, action.sequence))
        {
            FinishUnverifiedStationMemberAction(
                action.stationTarget, action.member,
                "Station removal stopped after 0 verified jobs. The member queue changed or became unavailable.");
            return;
        }

        if (!TryValidateStationActionTargetIdentity(action.stationTarget))
        {
            SetStationActionStatus(
                "The station is unavailable or is no longer player-managed. No jobs were changed.");
            return;
        }

        std::vector<int> removalSlots;
        for (size_t index = 0; index < current.jobs.size(); ++index)
        {
            const JobRowSnapshot& row = current.jobs[index];
            if (IsStationDisplayJob(row.taskType) && row.hasTarget &&
                SameHandleIdentity(row.target, action.stationTarget))
            {
                removalSlots.push_back(static_cast<int>(index));
            }
        }

        size_t removedCount = 0;
        for (size_t removalIndex = removalSlots.size();
             removalIndex > 0; --removalIndex)
        {
            const int slot = removalSlots[removalIndex - 1];
            // Revalidate the live handle and player/natural ownership gate
            // immediately before every mutation in the station bundle.
            if (!TryValidateStationActionTargetIdentity(
                    action.stationTarget))
            {
                std::ostringstream status;
                status << "Removal stopped after " << removedCount
                       << " verified job"
                       << (removedCount == 1 ? "" : "s")
                       << ". The exact station is no longer player-managed.";
                FinishUnverifiedStationMemberAction(
                    action.stationTarget, action.member, status.str());
                return;
            }

            // Reacquire and compare the exact full queue after the target
            // validation and immediately before the native removal. This
            // catches external or engine-side changes between bundle rows.
            MemberSnapshot beforeRemove;
            if (!TryBuildLiveMemberByIdentity(
                    action.member, &beforeRemove) ||
                !SameQueue(beforeRemove.jobs, current.jobs) ||
                slot < 0 ||
                slot >= static_cast<int>(beforeRemove.jobs.size()) ||
                !IsStationDisplayJob(beforeRemove.jobs[slot].taskType) ||
                !beforeRemove.jobs[slot].hasTarget ||
                !SameHandleIdentity(
                    beforeRemove.jobs[slot].target,
                    action.stationTarget))
            {
                std::ostringstream status;
                status << "Removal stopped after " << removedCount
                       << " verified job"
                       << (removedCount == 1 ? "" : "s")
                       << ". The exact full queue changed or became unavailable.";
                FinishUnverifiedStationMemberAction(
                    action.stationTarget, action.member, status.str());
                return;
            }
            const bool removeCallSucceeded =
                TryRemovePermajob(beforeRemove.handle, slot);

            MemberSnapshot afterRemove;
            if (!TryBuildLiveMemberByIdentity(action.member, &afterRemove))
            {
                std::ostringstream status;
                status << "Removal stopped after " << removedCount
                       << " verified job"
                       << (removedCount == 1 ? "" : "s")
                       << ". The changed queue became unavailable. Review it before another change.";
                FinishUnverifiedStationMemberAction(
                    action.stationTarget, action.member, status.str());
                return;
            }

            const bool exactRemoval = SameQueueWithExactJobRemoved(
                beforeRemove.jobs, slot, afterRemove.jobs);
            current = afterRemove;
            if (exactRemoval)
            {
                ++removedCount;
            }
            if (!removeCallSucceeded || !exactRemoval)
            {
                std::ostringstream status;
                status << "Removal stopped after " << removedCount
                       << " verified job"
                       << (removedCount == 1 ? "" : "s") << ". ";
                if (!exactRemoval)
                {
                    status << "The queue changed in an unexpected way.";
                }
                else
                {
                    status << "Kenshi reported an error after the removal.";
                }
                status << " Review it before another change.";
                FinishStationMemberAction(
                    current, action.stationTarget, status.str(), false);
                return;
            }
        }

        std::ostringstream status;
        if (removedCount == 0)
        {
            status << current.name << " has no permanent jobs for this station.";
        }
        else
        {
            status << "Removed all " << removedCount
                   << " permanent station job"
                   << (removedCount == 1 ? "" : "s") << " from "
                   << current.name << ".";
        }
        FinishStationMemberAction(
            current, action.stationTarget, status.str(), true);
    }

    void FinishStationTransfer(const std::string& status)
    {
        g_stationAssignmentsDirty = g_stationScan.started;
        TryNotifyVanillaSelectionUI();
        RefreshSquadView(true);
        SetStatus(status);
    }

    void FinishSuccessfulStationTransfer(
        const std::string& status,
        const MemberSnapshot& sourceAfter,
        const MemberSnapshot& destinationAfter)
    {
        const bool projectionPatched =
            TryPatchStationTransferProjection(
                &g_stationScan, sourceAfter, destinationAfter);
        TryNotifyVanillaSelectionUI();
        RefreshSquadView(true);
        if (projectionPatched)
        {
            // Refresh only the visible station widgets from the patched
            // snapshot.  Do not discard columns, scan progress, filters,
            // collapsed squads, selection, or scroll positions.
            g_stationAssignmentsDirty = false;
            g_stationProjectionRefreshSource = sourceAfter.identity;
            g_stationProjectionRefreshDestination = destinationAfter.identity;
            g_stationProjectionRefreshRequested = true;
        }
        else
        {
            // If the in-memory projection no longer contains both members,
            // fall back to the normal fail-closed station projection rebuild.
            g_stationAssignmentsDirty = g_stationScan.started;
        }
        SetStatus(status);
    }

    void ProcessStationJobTransfer(const PendingAction& action)
    {
        if (!action.job.hasTarget || !action.job.target.valid ||
            !SameHandleIdentity(action.job.target, action.stationTarget) ||
            action.stationTarget.type != BUILDING)
        {
            SetStatus("The station target is unavailable. No jobs were changed.");
            return;
        }
        if (SameHandleIdentity(action.member, action.destinationMember))
        {
            SetStatus("Choose a different member. No jobs were changed.");
            return;
        }

        MemberSnapshot source;
        MemberSnapshot destination;
        if (!TryBuildLiveMemberByIdentity(action.member, &source) ||
            !SameQueue(source.jobs, action.sequence))
        {
            g_stationAssignmentsDirty = g_stationScan.started;
            SetStatus(
                "The source queue changed or became unavailable. No jobs were changed.");
            return;
        }
        if (!TryBuildLiveMemberByIdentity(
                action.destinationMember, &destination) ||
            !SameQueue(destination.jobs, action.destinationSequence))
        {
            g_stationAssignmentsDirty = g_stationScan.started;
            SetStatus(
                "The destination queue changed or became unavailable. No jobs were changed.");
            return;
        }
        if (destination.jobs.size() >=
            static_cast<size_t>(MAX_SAFE_JOB_ROWS))
        {
            g_stationAssignmentsDirty = g_stationScan.started;
            SetStatus(
                "The destination queue is at the 64-job safety limit. "
                "No jobs were changed.");
            return;
        }

        const int sourceSlot = FindJobSlot(source, action.job);
        if (sourceSlot < 0 ||
            HasStationJobKindAndTarget(
                destination.jobs, action.job.taskType, action.stationTarget))
        {
            g_stationAssignmentsDirty = g_stationScan.started;
            SetStatus(sourceSlot < 0 ?
                "The exact source job is no longer available. No jobs were changed." :
                "That member already has the same job for this station. No jobs were changed.");
            return;
        }

        if (!TryAddStationPermanentJob(
                destination.handle, action.job.taskType,
                action.stationTarget))
        {
            g_stationAssignmentsDirty = g_stationScan.started;
            SetStatus("Kenshi rejected the destination job. No jobs were changed.");
            return;
        }

        MemberSnapshot destinationAfter;
        if (!TryBuildLiveMemberByIdentity(
                action.destinationMember, &destinationAfter) ||
            !SameQueueWithAppendedStationJob(
                destination.jobs, destinationAfter.jobs,
                action.job.taskType, action.stationTarget))
        {
            FinishStationTransfer(
                "The destination could not be verified after adding the job. "
                "The source was not changed; review both queues.");
            return;
        }

        // Recheck the source independently after the destination mutation.
        // Never remove a row if its full queue fingerprint changed.
        MemberSnapshot sourceBeforeRemove;
        if (!TryBuildLiveMemberByIdentity(action.member, &sourceBeforeRemove) ||
            !SameQueue(sourceBeforeRemove.jobs, action.sequence))
        {
            FinishStationTransfer(
                "The destination job was added, but the source queue changed. "
                "The source was not removed; a duplicate may remain.");
            return;
        }
        const int verifiedSourceSlot =
            FindJobSlot(sourceBeforeRemove, action.job);
        if (verifiedSourceSlot < 0 ||
            !TryRemovePermajob(sourceBeforeRemove.handle, verifiedSourceSlot))
        {
            FinishStationTransfer(
                "The destination job was added, but Kenshi rejected the source removal. "
                "A duplicate remains; review both queues.");
            return;
        }

        MemberSnapshot sourceAfter;
        if (!TryBuildLiveMemberByIdentity(action.member, &sourceAfter) ||
            !SameQueueWithExactJobRemoved(
                sourceBeforeRemove.jobs, verifiedSourceSlot,
                sourceAfter.jobs))
        {
            FinishStationTransfer(
                "The destination job was added, but the source removal could not be verified. "
                "Review both queues before making another change.");
            return;
        }

        std::ostringstream status;
        // The exact job label is stable even if the station column is filtered
        // before the next snapshot refresh.
        status << "Moved " << StripLeadingPriorityPrefix(action.job.jobLabel)
               << " from " << source.name << " to " << destination.name << ".";
        FinishSuccessfulStationTransfer(
            status.str(), sourceAfter, destinationAfter);
    }

    void ProcessPendingAction()
    {
        if (g_pendingAction.type == ACTION_NONE)
        {
            return;
        }
        PendingAction action = g_pendingAction;
        g_pendingAction = PendingAction();

        if (action.type == ACTION_REMOVE_SELECTED)
        {
            ProcessRemoveSelected();
            return;
        }

        if (action.type == ACTION_TRANSFER_STATION_JOB)
        {
            ProcessStationJobTransfer(action);
            return;
        }

        if (action.type == ACTION_ASSIGN_STATION)
        {
            ProcessStationAssignmentRequest(action);
            return;
        }

        if (action.type == ACTION_REMOVE_STATION_BUNDLE)
        {
            ProcessStationBundleRemovalRequest(action);
            return;
        }

        MemberSnapshot fresh;
        if (!TryRefreshMemberByIdentity(action.member, &fresh))
        {
            RefreshSquadView(true);
            SetStatus("The member queue is unavailable. No jobs were changed.");
            return;
        }

        if (action.type == ACTION_TOGGLE_JOBS)
        {
            if (!TrySetJobsEnabled(fresh.handle, !fresh.jobsEnabled))
            {
                SetStatus("Kenshi rejected the Jobs toggle. No jobs were changed.");
                return;
            }
            TryNotifyVanillaSelectionUI();
            RefreshSquadView(true);
            SetStatus(fresh.jobsEnabled ? "Jobs disabled for this member." :
                                          "Jobs enabled for this member.");
            return;
        }

        if (action.type == ACTION_REORDER)
        {
            if (!SameQueue(fresh.jobs, action.sequence))
            {
                RefreshSquadView(true);
                SetStatus("The queue changed during the drag. Review it and try again.");
                return;
            }
            const int source = FindJobSlot(fresh, action.job);
            if (source < 0 || action.targetSlot < 0 ||
                action.targetSlot >= static_cast<int>(fresh.jobs.size()) ||
                !TryMovePermajob(fresh.handle, source, action.targetSlot))
            {
                RefreshSquadView(true);
                SetStatus("Kenshi rejected the queue reorder. No other jobs were changed.");
                return;
            }
            TryNotifyVanillaSelectionUI();
            RefreshSquadView(true);
            SetStatus("Permanent-job priority updated.");
            return;
        }

        if (action.type == ACTION_CLEAR_MEMBER)
        {
            if (!SameQueue(fresh.jobs, action.sequence))
            {
                RefreshSquadView(true);
                SetStatus("The queue changed after confirmation. Clear Queue was cancelled.");
                return;
            }
            if (!TryClearPermajobs(fresh.handle))
            {
                SetStatus("Kenshi rejected Clear Queue. No jobs were changed.");
                return;
            }
            for (size_t index = g_selectedJobs.size(); index > 0; --index)
            {
                if (SameHandleIdentity(
                        g_selectedJobs[index - 1].member,
                        action.member))
                {
                    g_selectedJobs.erase(g_selectedJobs.begin() + index - 1);
                }
            }
            TryNotifyVanillaSelectionUI();
            RefreshSquadView(true);
            SetStatus("All permanent jobs were removed from " + fresh.name + ".");
        }
    }

    void TickDragAutoScroll(DWORD now)
    {
        if (!g_drag.active || g_jobViewport == NULL ||
            !HasElapsed(now, g_lastDragTick, 16))
        {
            return;
        }
        const DWORD elapsed = g_lastDragTick == 0 ? 16 : now - g_lastDragTick;
        g_lastDragTick = now;
        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        const MyGUI::IntCoord view = g_jobViewport->getAbsoluteCoord();
        int delta = 0;
        if (mouse.left >= view.left && mouse.left < view.left + DRAG_EDGE)
        {
            delta = -static_cast<int>(600.0f * elapsed / 1000.0f);
        }
        else if (mouse.left < view.right() && mouse.left >= view.right() - DRAG_EDGE)
        {
            delta = static_cast<int>(600.0f * elapsed / 1000.0f);
        }
        if (delta == 0)
        {
            UpdateDragInsertion(mouse);
            return;
        }

        const int next = ClampInt(
            g_horizontalOffset + delta,
            0,
            g_maxHorizontalOffset);
        if (next != g_horizontalOffset)
        {
            g_horizontalOffset = next;
            g_changingScroll = true;
            if (g_horizontalScroll != NULL)
            {
                g_horizontalScroll->setScrollPosition(
                    static_cast<size_t>(g_horizontalOffset));
            }
            g_changingScroll = false;
            ApplyScrollOffsets();
            UpdateDragInsertion(mouse);
        }
    }
