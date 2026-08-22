// SPDX-License-Identifier: GPL-3.0-only
#include "SquadGroups.inl"

    bool TryBindPortrait(const hand& member, MyGUI::ImageBox* image, bool force)
    {
        if (image == NULL)
        {
            return false;
        }
        __try
        {
            PortraitManager* manager = PortraitManager::getInstance();
            if (manager == NULL)
            {
                return false;
            }
            manager->setImageWidget(member, image, force);
            const MyGUI::IntSize imageSize = image->getImageSize();
            return imageSize.width > 0 && imageSize.height > 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    __declspec(noinline) bool CallPortraitVisuals(
        const hand& member,
        bool* selectedOut,
        std::string* backgroundOut,
        std::string* backOverlayOut,
        std::string* frontOverlayOut)
    {
        if (selectedOut == NULL || backgroundOut == NULL ||
            backOverlayOut == NULL || frontOverlayOut == NULL)
        {
            return false;
        }

        try
        {
            PortraitManager* manager = PortraitManager::getInstance();
            if (manager == NULL)
            {
                return false;
            }
            PortraitData* data = manager->getPortrait(member);
            if (data == NULL)
            {
                return false;
            }
            data->update();
            *selectedOut = data->isSelected();
            *backgroundOut = data->getBackgroundImageName();
            *backOverlayOut = data->getBackOverlayImageName();
            *frontOverlayOut = data->getFrontOverlayImageName();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool TryGetPortraitVisuals(
        const hand& member,
        bool* selectedOut,
        std::string* backgroundOut,
        std::string* backOverlayOut,
        std::string* frontOverlayOut)
    {
        if (selectedOut == NULL || backgroundOut == NULL ||
            backOverlayOut == NULL || frontOverlayOut == NULL)
        {
            return false;
        }
        __try
        {
            return CallPortraitVisuals(
                member,
                selectedOut,
                backgroundOut,
                backOverlayOut,
                frontOverlayOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void SetPortraitImage(
        MyGUI::ImageBox* image,
        const char* group,
        const std::string& item)
    {
        if (image == NULL)
        {
            return;
        }
        if (item.empty())
        {
            image->setVisible(false);
            return;
        }
        image->setVisible(true);
        image->setItemResource("Kenshi_PortraitImage");
        image->setItemGroup(group);
        image->setItemName(item);
    }

    void ApplyPortrait(MemberWidgets& widgets, const MemberSnapshot& member, bool force)
    {
        if (!member.loaded)
        {
            widgets.portraitBound = false;
            if (widgets.portrait != NULL)
            {
                widgets.portrait->setVisible(false);
            }
            if (widgets.portraitBackground != NULL)
            {
                widgets.portraitBackground->setVisible(false);
            }
            if (widgets.portraitBackOverlay != NULL)
            {
                widgets.portraitBackOverlay->setVisible(false);
            }
            if (widgets.portraitFrontOverlay != NULL)
            {
                widgets.portraitFrontOverlay->setVisible(false);
            }
            if (widgets.portraitBorder != NULL)
            {
                widgets.portraitBorder->setStateSelected(false);
            }
            return;
        }

        // Update PortraitData before binding.  Kenshi can create the portrait
        // record or atlas entry during this call; binding first can leave a
        // new ImageBox empty with no later retry for an otherwise unchanged
        // member.
        bool selected = false;
        std::string background;
        std::string backOverlay;
        std::string frontOverlay;
        if (TryGetPortraitVisuals(
                member.handle,
                &selected,
                &background,
                &backOverlay,
                &frontOverlay))
        {
            SetPortraitImage(widgets.portraitBackground, "Background", background);
            SetPortraitImage(widgets.portraitBackOverlay, "BackOverlay", backOverlay);
            SetPortraitImage(widgets.portraitFrontOverlay, "FrontOverlay", frontOverlay);
        }

        // Recipient selection belongs to this manager and is intentionally
        // separate from Kenshi's existing character selection.
        widgets.portraitBorder->setStateSelected(
            IsRecipientSelected(member.identity));
        if (widgets.recipientMarker != NULL)
        {
            widgets.recipientMarker->setVisible(
                IsRecipientSelected(member.identity));
        }

        if (widgets.portrait != NULL)
        {
            widgets.portrait->setVisible(true);
            if (force || !widgets.portraitBound)
            {
                // Native portrait cells normally bind without force.  If the
                // atlas was not ready, retry once with force and keep a cheap
                // once-per-refresh retry active until a non-empty image is
                // actually attached.
                widgets.portraitBound = TryBindPortrait(
                    member.handle, widgets.portrait, false);
                if (!widgets.portraitBound)
                {
                    widgets.portraitBound = TryBindPortrait(
                        member.handle, widgets.portrait, true);
                }
            }
        }
    }

    bool IsSelectedJob(
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
                return true;
            }
        }
        return false;
    }

    bool JobRowUsesFixedTarget(const JobRowSnapshot& job)
    {
        // Keep the visual rule aligned with Remove Invalid Jobs.  The
        // TaskData flag is authoritative for ordinary station work, but the
        // protected families remain targetless even if an imported save
        // contains an unexpected fixed-target bit on their TaskData.
        const TaskType root = job.associatedTaskType == NULL_TASK ?
            job.taskType : job.associatedTaskType;
        return job.fixedTarget &&
            !IsJobBatchInvalidRemovalProtectedTask(job.taskType) &&
            !IsJobBatchInvalidRemovalProtectedTask(root);
    }

    bool JobRowTargetUnavailable(const JobRowSnapshot& job)
    {
        return JobRowUsesFixedTarget(job) &&
            (!job.hasTarget || !job.targetResolvable);
    }

    MyGUI::Colour JobSelectionMarkerColour()
    {
        // Keep a single selected card green for the normal single-job state.
        // Once Ctrl-selection contains more than one card, use the same light
        // blue accent used by the manager's other selected/recent markers so
        // a multi-selection is visually distinct without changing the
        // unavailable warning layer or hover outline.
        return g_selectedJobs.size() > 1 ?
            MyGUI::Colour(0.48f, 0.82f, 1.0f) :
            MyGUI::Colour(0.45f, 0.90f, 0.55f);
    }

    void ApplyJobSelectionMarker(
        CardWidgets& card,
        bool selected)
    {
        if (card.selectionMarker == NULL)
        {
            return;
        }
        // Only selected markers need a colour write. Most cards are not part
        // of the selection, so avoid invalidating every card subtree on each
        // Ctrl-click in a large roster.
        if (selected)
        {
            card.selectionMarker->setColour(JobSelectionMarkerColour());
        }
        if (card.selectionMarker->getVisible() != selected)
        {
            card.selectionMarker->setVisible(selected);
        }
    }

    bool RepairSelection()
    {
        std::vector<SelectedJob> repaired;
        repaired.reserve(g_selectedJobs.size());
        for (size_t index = 0; index < g_selectedJobs.size(); ++index)
        {
            const MemberSnapshot* member = FindDisplayedMember(
                g_selectedJobs[index].member);
            if (member == NULL)
            {
                continue;
            }
            if (!member->queueAvailable)
            {
                continue;
            }
            int slot = -1;
            if (g_selectedJobs[index].job.taskToken == 0 &&
                g_selectedJobs[index].lastSlot >= 0 &&
                g_selectedJobs[index].lastSlot <
                    static_cast<int>(member->jobs.size()) &&
                SameJob(
                    member->jobs[g_selectedJobs[index].lastSlot],
                    g_selectedJobs[index].job))
            {
                slot = g_selectedJobs[index].lastSlot;
            }
            else
            {
                slot = FindJobSlot(
                    *member,
                    g_selectedJobs[index].job);
            }
            if (slot < 0)
            {
                continue;
            }
            SelectedJob selected = g_selectedJobs[index];
            selected.job = member->jobs[slot];
            selected.lastSlot = slot;
            repaired.push_back(selected);
        }
        bool changed = repaired.size() != g_selectedJobs.size();
        if (!changed)
        {
            for (size_t index = 0; index < repaired.size(); ++index)
            {
                if (!SameHandleIdentity(
                        repaired[index].member,
                        g_selectedJobs[index].member) ||
                    !SameJob(repaired[index].job, g_selectedJobs[index].job) ||
                    repaired[index].lastSlot != g_selectedJobs[index].lastSlot)
                {
                    changed = true;
                    break;
                }
            }
        }
        g_selectedJobs.swap(repaired);

        if (FindDisplayedMember(g_selectionAnchorMember) == NULL)
        {
            ResetHandleIdentity(&g_selectionAnchorMember);
            g_selectionAnchorSlot = -1;
        }
        return changed;
    }

    void UpdateRemoveButton()
    {
        if (g_removeButton == NULL)
        {
            return;
        }
        // Keep this caption short enough for the fixed action-bar button.
        // The selected count remains visible through the card selection and
        // status/tooltip channels, while the action name stays readable.
        g_removeButton->setCaption("Remove Job(s)");
        g_removeButton->setFontHeight(14);
        g_removeButton->setEnabled(JobCardSelectionMode());
    }

    void ApplyCardSelectionStates()
    {
        for (size_t memberIndex = 0; memberIndex < g_memberWidgets.size(); ++memberIndex)
        {
            MemberWidgets& widgets = g_memberWidgets[memberIndex];
            const MemberSnapshot* member = GetVisibleMember(memberIndex);
            if (member == NULL)
            {
                continue;
            }
            for (size_t slot = 0; slot < widgets.cards.size(); ++slot)
            {
                if (slot < member->jobs.size() && widgets.cards[slot].card != NULL)
                {
                    const bool selected = IsSelectedJob(
                        member->identity,
                        member->jobs[slot],
                        static_cast<int>(slot));
                    if (widgets.cards[slot].card->getStateSelected() != selected)
                    {
                        widgets.cards[slot].card->setStateSelected(selected);
                    }
                    ApplyJobSelectionMarker(widgets.cards[slot], selected);
                    if (widgets.cards[slot].unavailableMarker != NULL)
                    {
                        const bool targetUnavailable = JobRowTargetUnavailable(
                            member->jobs[slot]);
                        if (widgets.cards[slot].unavailableMarker->getVisible() !=
                            targetUnavailable)
                        {
                            widgets.cards[slot].unavailableMarker->setVisible(
                                targetUnavailable);
                        }
                    }
                }
            }
        }
        UpdateRemoveButton();
    }

    JobHighlightKey MakeJobHighlightKey(const JobRowSnapshot& job)
    {
        JobHighlightKey key;
        key.taskType = job.taskType;
        // Kenshi stores an incidental subject on some global queue rows. Only
        // TaskData::permaJob_FixedTarget makes that subject authoritative for
        // presentation and hover grouping.
        key.hasTarget = JobRowUsesFixedTarget(job) && job.hasTarget;
        if (key.hasTarget)
        {
            key.target = job.target;
        }
        return key;
    }

    bool SameJobHighlightKey(
        const JobHighlightKey& left,
        const JobHighlightKey& right)
    {
        return left.taskType == right.taskType &&
            left.hasTarget == right.hasTarget &&
            (!left.hasTarget || SameHandleIdentity(left.target, right.target));
    }

    bool SameJobHighlightQueue(
        const std::vector<JobRowSnapshot>& left,
        const std::vector<JobRowSnapshot>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.size(); ++index)
        {
            if (!SameJobHighlightKey(
                    MakeJobHighlightKey(left[index]),
                    MakeJobHighlightKey(right[index])))
            {
                return false;
            }
        }
        return true;
    }

    int FindJobHighlightGroup(const JobRowSnapshot& job)
    {
        if (!g_jobHighlightCacheValid)
        {
            return -1;
        }
        const JobHighlightKey key = MakeJobHighlightKey(job);
        for (size_t index = 0; index < g_jobHighlightKeys.size(); ++index)
        {
            if (SameJobHighlightKey(g_jobHighlightKeys[index], key))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void RebuildJobHighlightCache()
    {
        // This index is built from the already captured all-squads snapshot. Hover
        // callbacks only compare cached group IDs and never inspect Kenshi's
        // live job queues.
        g_jobHighlightKeys.clear();
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const MemberSnapshot& member = squad.members[memberIndex];
                for (size_t slot = 0; slot < member.jobs.size(); ++slot)
                {
                    const JobHighlightKey key = MakeJobHighlightKey(member.jobs[slot]);
                    bool found = false;
                    for (size_t keyIndex = 0;
                         keyIndex < g_jobHighlightKeys.size(); ++keyIndex)
                    {
                        if (SameJobHighlightKey(g_jobHighlightKeys[keyIndex], key))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        g_jobHighlightKeys.push_back(key);
                    }
                }
            }
        }
        g_jobHighlightCacheValid = true;
        g_hoveredJobHighlightGroup = -1;
    }

    void SetCardHighlightVisible(CardWidgets& card, bool visible)
    {
        if (card.highlightTop != NULL)
        {
            if (card.highlightTop->getVisible() != visible)
            {
                card.highlightTop->setVisible(visible);
            }
        }
        if (card.highlightBottom != NULL)
        {
            if (card.highlightBottom->getVisible() != visible)
            {
                card.highlightBottom->setVisible(visible);
            }
        }
        if (card.highlightLeft != NULL)
        {
            if (card.highlightLeft->getVisible() != visible)
            {
                card.highlightLeft->setVisible(visible);
            }
        }
        if (card.highlightRight != NULL)
        {
            if (card.highlightRight->getVisible() != visible)
            {
                card.highlightRight->setVisible(visible);
            }
        }
    }

    bool CardIntersectsSquadJobsViewport(
        const CardWidgets& card,
        const MyGUI::IntCoord& viewport)
    {
        if (card.card == NULL || card.memberIndex < 0 ||
            card.memberIndex >= static_cast<int>(g_visibleMemberBindings.size()) ||
            card.slot < 0)
        {
            return false;
        }
        // Card positions are deterministic children of the scrolled job
        // canvas. Use the copied row binding and the two scroll offsets instead
        // of asking MyGUI for an absolute rectangle for every matching card.
        const int cardLeft = viewport.left +
            card.slot * CARD_STRIDE - g_horizontalOffset;
        const int cardTop = viewport.top +
            g_visibleMemberBindings[card.memberIndex].rowTop + CARD_TOP -
            g_verticalOffset;
        return cardLeft < viewport.right() &&
            cardLeft + CARD_WIDTH > viewport.left &&
            cardTop < viewport.bottom() &&
            cardTop + CARD_HEIGHT > viewport.top;
    }

    void ApplyCardHoverHighlights(int previousGroup, int nextGroup)
    {
        if (previousGroup == nextGroup || g_jobViewport == NULL)
        {
            return;
        }

        // The hover outline is presentation-only.  Limit the native visibility
        // writes to the old/new groups and to cards that are actually inside
        // the two-dimensional Squad Jobs viewport.  Cards outside either
        // scroll axis are already hidden and will be rebuilt or re-hovered when
        // they become visible.
        const MyGUI::IntCoord viewport = g_jobViewport->getAbsoluteCoord();
        for (size_t memberIndex = 0;
             memberIndex < g_memberWidgets.size();
             ++memberIndex)
        {
            MemberWidgets& widgets = g_memberWidgets[memberIndex];
            for (size_t slot = 0; slot < widgets.cards.size(); ++slot)
            {
                CardWidgets& card = widgets.cards[slot];
                const bool hidePrevious = previousGroup >= 0 &&
                    card.highlightGroup == previousGroup;
                const bool showNext = nextGroup >= 0 &&
                    card.highlightGroup == nextGroup;
                if (!hidePrevious && !showNext)
                {
                    continue;
                }
                if (!CardIntersectsSquadJobsViewport(card, viewport))
                {
                    continue;
                }
                SetCardHighlightVisible(
                    card,
                    showNext);
            }
        }
    }

    void ApplyCachedJobHighlightGroups()
    {
        // A rebuilt key vector can renumber every later group even when only
        // one member changed. Refresh surviving cards so unchanged member
        // widgets never retain stale group IDs.
        const size_t memberCount = std::min(
            g_memberWidgets.size(), g_visibleMemberBindings.size());
        for (size_t memberIndex = 0; memberIndex < memberCount; ++memberIndex)
        {
            MemberWidgets& widgets = g_memberWidgets[memberIndex];
            const MemberSnapshot* member = GetVisibleMember(memberIndex);
            if (member == NULL)
            {
                continue;
            }
            const size_t cardCount = std::min(
                widgets.cards.size(), member->jobs.size());
            for (size_t slot = 0; slot < cardCount; ++slot)
            {
                widgets.cards[slot].highlightGroup =
                    FindJobHighlightGroup(member->jobs[slot]);
            }
        }
    }

    std::string StripLeadingPriorityPrefix(const std::string& label)
    {
        // OrderData::text contains MyGUI inline colour tags.  Those tags take
        // precedence over TextBox::setTextColour, which left the work-type
        // line gray even after the widget itself was forced to white.  Strip
        // presentation markup before displaying the engine label; the queue
        // position is already represented by the card's column.
        std::string plainLabel = label;
        try
        {
            plainLabel = MyGUI::TextIterator::getOnlyText(
                MyGUI::UString(label)).asUTF8();
        }
        catch (...)
        {
            plainLabel = label;
        }

        size_t cursor = 0;
        while (cursor < plainLabel.size() &&
            (plainLabel[cursor] == ' ' || plainLabel[cursor] == '\t'))
        {
            ++cursor;
        }
        const size_t numberStart = cursor;
        while (cursor < plainLabel.size() &&
            plainLabel[cursor] >= '0' && plainLabel[cursor] <= '9')
        {
            ++cursor;
        }
        if (cursor == numberStart || cursor >= plainLabel.size())
        {
            return plainLabel;
        }

        // OrderData commonly formats its display text as "3: Find and
        // rescue".  Ignore whitespace between the number and colon so the
        // raw engine label remains safe to display when the spacing varies.
        size_t separator = cursor;
        while (separator < plainLabel.size() &&
            (plainLabel[separator] == ' ' || plainLabel[separator] == '\t'))
        {
            ++separator;
        }
        if (separator >= plainLabel.size() || plainLabel[separator] != ':')
        {
            return plainLabel;
        }

        ++separator;
        while (separator < plainLabel.size() &&
            (plainLabel[separator] == ' ' || plainLabel[separator] == '\t'))
        {
            ++separator;
        }
        if (separator >= plainLabel.size())
        {
            return plainLabel;
        }
        return plainLabel.substr(separator);
    }

    std::string WrapCardJobCaption(
        const std::string& label,
        size_t maxCharacters)
    {
        if (label.empty() || maxCharacters == 0)
        {
            return label;
        }

        std::string wrapped;
        std::string line;
        size_t cursor = 0;
        while (cursor < label.size())
        {
            while (cursor < label.size() &&
                (label[cursor] == ' ' || label[cursor] == '\t'))
            {
                ++cursor;
            }
            if (cursor >= label.size())
            {
                break;
            }

            size_t end = cursor;
            while (end < label.size() &&
                label[end] != ' ' && label[end] != '\t')
            {
                ++end;
            }
            std::string word = label.substr(cursor, end - cursor);
            if (!word.empty())
            {
                const size_t required = line.empty()
                    ? word.size()
                    : line.size() + 1 + word.size();
                if (!line.empty() && required > maxCharacters)
                {
                    if (!wrapped.empty())
                    {
                        wrapped += '\n';
                    }
                    wrapped += line;
                    line.clear();
                }
                if (line.empty())
                {
                    line = word;
                }
                else
                {
                    line += ' ';
                    line += word;
                }
            }
            cursor = end;
        }
        if (!line.empty())
        {
            if (!wrapped.empty())
            {
                wrapped += '\n';
            }
            wrapped += line;
        }
        return wrapped;
    }

    void SetFittedCardJobCaption(
        MyGUI::TextBox* text,
        const std::string& rawLabel)
    {
        if (text == NULL)
        {
            return;
        }

        std::string displayLabel = StripLeadingPriorityPrefix(rawLabel);
        // The target and arrow already identify the object below this line.
        // Keep the common machinery orders short on the Squad Jobs card while
        // leaving rawLabel untouched for tooltips, matching, and mutation.
        if (displayLabel.compare(0, 9, "Operating") == 0)
        {
            displayLabel = "Operating...";
        }
        text->setCaption(displayLabel.c_str());

        // TextBox exposes its measured caption size and font height.  Use
        // those MyGUI APIs instead of assuming a character width: Kenshi's
        // font is not monospaced, and long OrderData labels otherwise clip at
        // the fixed card edge.  Keep the normal size for short labels, then
        // scale only labels that need it so the card remains compact.
        const int maxWidth = CARD_WIDTH - 16;
        int fontHeight = text->getFontHeight();
        if (fontHeight <= 0)
        {
            return;
        }

        if (text->getTextSize().width > maxWidth && fontHeight > 16)
        {
            fontHeight = 16;
            text->setFontHeight(fontHeight);
        }

        if (text->getTextSize().width <= maxWidth)
        {
            return;
        }

        // Prefer a readable two-line caption at the 16pt size before using
        // the stock Small (14pt) size.  This keeps ordinary long work types
        // legible while reserving the smaller fallback for unusual labels.
        text->setCaption(
            WrapCardJobCaption(displayLabel, 25).c_str());
        if (text->getTextSize().width <= maxWidth &&
            text->getTextSize().height <= 38)
        {
            return;
        }

        if (fontHeight > 14)
        {
            fontHeight = 14;
            text->setFontHeight(fontHeight);
        }
        if (text->getTextSize().width <= maxWidth &&
            text->getTextSize().height <= 38)
        {
            return;
        }

        // Handle unusually long or unbroken labels after wrapping.  The
        // compact card has 38 pixels for this caption; a small fallback font
        // keeps every generated line inside that region without changing the
        // row height or moving the target/arrow layout.
        while (fontHeight > 10 &&
            (text->getTextSize().width > maxWidth ||
             text->getTextSize().height > 38))
        {
            --fontHeight;
            text->setFontHeight(fontHeight);
        }
    }

    struct JobStationCategoryCacheEntry
    {
        HandleIdentity target;
        StationCategory category;
        StationVisualSubtype visualSubtype;

        JobStationCategoryCacheEntry() :
            category(STATION_OTHER), visualSubtype(STATION_VISUAL_DEFAULT)
        {
        }
    };

    std::vector<JobStationCategoryCacheEntry> g_jobStationCategoryCache;
    std::vector<HandleIdentity> g_jobStationCategoryPending;
    const size_t MAX_JOB_STATION_CATEGORY_TARGETS = 512;

    bool TrySetJobStationCategoryIcon(
        MyGUI::ImageBox* icon,
        const char* resource);

    bool SetJobStationCategoryArtwork(
        MyGUI::ImageBox* icon,
        MyGUI::Widget* overlay,
        StationCategory category,
        StationVisualSubtype visualSubtype)
    {
        if (icon == NULL || overlay == NULL)
        {
            return false;
        }
        const char* resource = GetStationVisualIconResource(
            category, visualSubtype);
        const bool applied = resource != NULL && resource[0] != '\0' &&
            TrySetJobStationCategoryIcon(icon, resource);
        icon->setVisible(applied);
        overlay->setVisible(applied);
        return applied;
    }

    bool TryGetCachedJobStationCategory(
        const HandleIdentity& target,
        StationCategory* categoryOut,
        StationVisualSubtype* visualSubtypeOut)
    {
        if (categoryOut == NULL || visualSubtypeOut == NULL ||
            !target.valid || target.type != BUILDING)
        {
            return false;
        }
        // Prefer the station scan because it is the richer snapshot and can
        // replace an earlier neutral queue-cache result after its one-target
        // enrichment step completes.
        for (size_t index = 0; index < g_stationScan.stations.size(); ++index)
        {
            const StationTargetSnapshot& station = g_stationScan.stations[index];
            if (SameHandleIdentity(station.identity, target))
            {
                *categoryOut = station.category;
                *visualSubtypeOut = station.visualSubtype;
                return true;
            }
        }
        for (size_t index = 0; index < g_jobStationCategoryCache.size(); ++index)
        {
            if (SameHandleIdentity(
                    g_jobStationCategoryCache[index].target, target))
            {
                *categoryOut = g_jobStationCategoryCache[index].category;
                *visualSubtypeOut =
                    g_jobStationCategoryCache[index].visualSubtype;
                return true;
            }
        }
        return false;
    }

    bool QueueJobStationCategory(const HandleIdentity& target)
    {
        if (!target.valid || target.type != BUILDING)
        {
            return false;
        }
        StationCategory ignored = STATION_OTHER;
        StationVisualSubtype ignoredSubtype = STATION_VISUAL_DEFAULT;
        if (TryGetCachedJobStationCategory(
                target, &ignored, &ignoredSubtype))
        {
            return true;
        }
        for (size_t index = 0; index < g_jobStationCategoryPending.size(); ++index)
        {
            if (SameHandleIdentity(g_jobStationCategoryPending[index], target))
            {
                return true;
            }
        }
        if (g_jobStationCategoryPending.size() <
            MAX_JOB_STATION_CATEGORY_TARGETS)
        {
            g_jobStationCategoryPending.push_back(target);
            return true;
        }

        // An extraordinary current squad can exceed the finite live-resolution
        // cap. Give every overflow target immediate neutral artwork instead of
        // leaving its cards blank or extending the synchronous engine pass.
        JobStationCategoryCacheEntry fallback;
        fallback.target = target;
        g_jobStationCategoryCache.push_back(fallback);
        return false;
    }

    bool TryResolveJobStationCategory(
        const HandleIdentity& target,
        StationCategory* categoryOut,
        StationVisualSubtype* visualSubtypeOut)
    {
        if (categoryOut == NULL || visualSubtypeOut == NULL ||
            !target.valid || target.type != BUILDING)
        {
            return false;
        }
        *categoryOut = STATION_OTHER;
        *visualSubtypeOut = STATION_VISUAL_DEFAULT;
        const hand targetHandle(
            target.index, target.serial, target.type,
            target.container, target.containerSerial);
        Building* building = NULL;
        if (!TryResolveStationBuilding(targetHandle, &building) ||
            building == NULL)
        {
            return false;
        }
        StatsEnumerated stat = STAT_NONE;
        bool statKnown = false;
        bool natural = false;
        bool relevant = false;
        bool assignmentSupported = false;
        if (!TryClassifyStationBuilding(
                building, categoryOut, &stat, &statKnown,
                &natural, &relevant, &assignmentSupported))
        {
            *categoryOut = STATION_OTHER;
            *visualSubtypeOut = STATION_VISUAL_DEFAULT;
            return false;
        }
        TryReadStationVisualSubtype(
            building, natural, visualSubtypeOut);
        return true;
    }

    bool ApplyJobStationCategoryArtwork(
        const HandleIdentity& target,
        StationCategory category,
        StationVisualSubtype visualSubtype)
    {
        bool targetUsedBySquadJob = false;
        const size_t memberCount = std::min(
            g_memberWidgets.size(), g_visibleMemberBindings.size());
        for (size_t memberIndex = 0; memberIndex < memberCount; ++memberIndex)
        {
            MemberWidgets& widgets = g_memberWidgets[memberIndex];
            const MemberSnapshot* member = GetVisibleMember(memberIndex);
            if (member == NULL)
            {
                continue;
            }
            const size_t cardCount = std::min(
                widgets.cards.size(), member->jobs.size());
            for (size_t slot = 0; slot < cardCount; ++slot)
            {
                if (!JobRowUsesFixedTarget(member->jobs[slot]) ||
                    !SameHandleIdentity(member->jobs[slot].target, target))
                {
                    continue;
                }
                targetUsedBySquadJob = true;
                if (widgets.cards[slot].categoryIcon == NULL ||
                    widgets.cards[slot].categoryOverlay == NULL)
                {
                    continue;
                }
                SetJobStationCategoryArtwork(
                    widgets.cards[slot].categoryIcon,
                    widgets.cards[slot].categoryOverlay,
                    category,
                    visualSubtype);
            }
        }
        return targetUsedBySquadJob;
    }

    void TickJobStationCategoryCache()
    {
        if (g_jobStationCategoryPending.empty())
        {
            return;
        }
        const HandleIdentity target = g_jobStationCategoryPending.front();
        g_jobStationCategoryPending.erase(g_jobStationCategoryPending.begin());
        StationCategory category = STATION_OTHER;
        StationVisualSubtype visualSubtype = STATION_VISUAL_DEFAULT;
        // The Stations pass may have resolved this target while the hidden
        // Squad artwork queue was paused. Reuse that value snapshot instead
        // of performing a second live-building lookup after scanning ends.
        if (TryGetCachedJobStationCategory(
                target, &category, &visualSubtype))
        {
            ApplyJobStationCategoryArtwork(
                target, category, visualSubtype);
            return;
        }
        // A queue target can be unavailable even though its permanent job is
        // still editable.  Give that card the neutral Other artwork instead
        // of leaving it iconless or retrying an engine lookup every frame.
        TryResolveJobStationCategory(target, &category, &visualSubtype);
        JobStationCategoryCacheEntry entry;
        entry.target = target;
        entry.category = category;
        entry.visualSubtype = visualSubtype;
        g_jobStationCategoryCache.push_back(entry);
        ApplyJobStationCategoryArtwork(target, category, visualSubtype);
    }

    bool DrainPendingJobStationCategoriesImmediately()
    {
        // Card creation already deduplicates exact BUILDING identities and
        // caps the guarded live-resolution pass at 512. Resolve that finite
        // snapshot before the current Squad view can render instead of
        // revealing one icon per later update tick. Do not enumerate
        // ownership or the world here.
        const size_t initialCount = g_jobStationCategoryPending.size();
        const size_t maximumPasses = std::min(
            initialCount, MAX_JOB_STATION_CATEGORY_TARGETS);
        for (size_t pass = 0;
             pass < maximumPasses &&
             !g_jobStationCategoryPending.empty();
             ++pass)
        {
            const size_t countBefore = g_jobStationCategoryPending.size();
            TickJobStationCategoryCache();
            if (g_jobStationCategoryPending.size() >= countBefore)
            {
                return false;
            }
        }
        return g_jobStationCategoryPending.empty();
    }

    __declspec(noinline) bool CallDrainPendingJobStationCategoriesSafely()
    {
        try
        {
            return DrainPendingJobStationCategoriesImmediately();
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TryDrainPendingJobStationCategoriesGuarded()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallDrainPendingJobStationCategoriesSafely();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted)
        {
            ErrorLog(
                "[KenshiJobManagement] Squad job artwork preparation failed safely.");
            return false;
        }
        return result;
    }

    void SyncJobStationArtworkFromSnapshot(
        const StationTargetSnapshot& station)
    {
        // A scanner snapshot supersedes any queued live lookup for the same
        // target. Remove all matches now so reopening Squad Jobs cannot repeat
        // work that the guarded station pass already completed.
        for (size_t pendingIndex = 0;
             pendingIndex < g_jobStationCategoryPending.size();)
        {
            if (SameHandleIdentity(
                    g_jobStationCategoryPending[pendingIndex],
                    station.identity))
            {
                g_jobStationCategoryPending.erase(
                    g_jobStationCategoryPending.begin() + pendingIndex);
            }
            else
            {
                ++pendingIndex;
            }
        }
        bool found = false;
        for (size_t index = 0; index < g_jobStationCategoryCache.size(); ++index)
        {
            JobStationCategoryCacheEntry& entry =
                g_jobStationCategoryCache[index];
            if (!SameHandleIdentity(entry.target, station.identity))
            {
                continue;
            }
            entry.category = station.category;
            entry.visualSubtype = station.visualSubtype;
            found = true;
            break;
        }
        const bool targetUsedBySquadJob = ApplyJobStationCategoryArtwork(
            station.identity, station.category, station.visualSubtype);
        if (!found && targetUsedBySquadJob && station.identity.valid &&
            station.identity.type == BUILDING)
        {
            JobStationCategoryCacheEntry entry;
            entry.target = station.identity;
            entry.category = station.category;
            entry.visualSubtype = station.visualSubtype;
            g_jobStationCategoryCache.push_back(entry);
        }
    }

    void ClearJobStationCategoryCache()
    {
        g_jobStationCategoryCache.clear();
        g_jobStationCategoryPending.clear();
    }

    bool TrySetJobStationCategoryIcon(
        MyGUI::ImageBox* icon,
        const char* resource)
    {
        if (icon == NULL || resource == NULL || resource[0] == '\0')
        {
            return false;
        }
        try
        {
            icon->setImageTexture(resource);
            const MyGUI::IntSize imageSize = icon->getImageSize();
            return imageSize.width > 0 && imageSize.height > 0;
        }
        catch (...)
        {
            return false;
        }
    }

    void SetFittedCardTargetCaption(
        MyGUI::TextBox* text,
        const std::string& caption)
    {
        if (text == NULL)
        {
            return;
        }

        text->setCaption(caption.c_str());
        const int maxWidth = CARD_WIDTH - 16;
        int fontHeight = text->getFontHeight();
        if (fontHeight <= 0)
        {
            return;
        }

        // Target names stay on one line so the arrow remains visually tied
        // to its target.  Reduce only the target font when a narrow card
        // meets an unusually long renamed building.
        while (fontHeight > 10 && text->getTextSize().width > maxWidth)
        {
            --fontHeight;
            text->setFontHeight(fontHeight);
        }
    }

    std::string BuildSkillLineCaption(const SkillValue& skill)
    {
        std::ostringstream caption;
        caption << skill.name << " " << skill.value;
        return caption.str();
    }

    std::string BuildSkillCaption(
        const MemberSnapshot& member,
        size_t firstSkill)
    {
        if (!member.loaded)
        {
            return "Stats unavailable";
        }
        if (member.skills.empty())
        {
            return "No stats above 1";
        }
        if (firstSkill >= member.skills.size())
        {
            return "";
        }

        std::ostringstream caption;
        for (size_t index = firstSkill; index < member.skills.size(); ++index)
        {
            if (index != firstSkill)
            {
                caption << "\n";
            }
            caption << BuildSkillLineCaption(member.skills[index]);
        }
        return caption.str();
    }

    void DestroyCardWidgets(MemberWidgets& widgets)
    {
        if (g_tooltip != NULL)
        {
            g_tooltip->setVisible(false);
        }
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
        {
            widgets.cards.clear();
            widgets.emptyJobs = NULL;
            return;
        }
        for (size_t index = 0; index < widgets.cards.size(); ++index)
        {
            if (widgets.cards[index].card != NULL)
            {
                gui->destroyWidget(widgets.cards[index].card);
            }
        }
        widgets.cards.clear();
        if (widgets.emptyJobs != NULL)
        {
            gui->destroyWidget(widgets.emptyJobs);
            widgets.emptyJobs = NULL;
        }
    }

    void CreateCardWidgets(size_t memberIndex, MemberWidgets& widgets)
    {
        const MemberSnapshot* memberPointer = GetVisibleMember(memberIndex);
        if (memberPointer == NULL || widgets.jobsRoot == NULL)
        {
            return;
        }
        const MemberSnapshot& member = *memberPointer;

        if (member.jobs.empty())
        {
            widgets.emptyJobs = widgets.jobsRoot->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(10, 20, g_jobWidth - 20, 40),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_EmptyQueue");
            widgets.emptyJobs->setNeedMouseFocus(false);
            widgets.emptyJobs->setCaption(
                member.queueAvailable ? "[No permanent jobs]" : "Queue unavailable (read-only)");
            TagThemeStandardText(widgets.emptyJobs);
            // Keep queue text fully opaque when the parent row is dimmed for
            // Jobs OFF or an unavailable queue.
            widgets.emptyJobs->setInheritsAlpha(false);
            return;
        }

        widgets.cards.reserve(member.jobs.size());
        for (size_t slot = 0; slot < member.jobs.size(); ++slot)
        {
            const JobRowSnapshot& row = member.jobs[slot];
            const bool showTarget = JobRowUsesFixedTarget(row);
            const char* roleIconResource =
                GetGlobalJobIconResource(row.taskType);
            const bool hasStationArtworkTarget =
                showTarget && row.target.valid &&
                row.target.type == BUILDING;
            CardWidgets card;
            card.memberIndex = static_cast<int>(memberIndex);
            card.slot = static_cast<int>(slot);
            card.card = widgets.jobsRoot->createWidget<MyGUI::Button>(
                // TickButton1 includes a checkbox strip sized for a short
                // 108x26 control.  Stretching it across a queue card made
                // that strip look like a distorted building icon.  Use the
                // regular nine-slice button until real job artwork exists.
                "Kenshi_Button1",
                MyGUI::IntCoord(
                    static_cast<int>(slot) * CARD_STRIDE,
                    CARD_TOP,
                    CARD_WIDTH,
                    CARD_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_JobCard");
            card.card->setCaption("");
            card.card->setUserString("KJM_Member", IntegerString(memberIndex));
            card.card->setUserString("KJM_Slot", IntegerString(slot));
            card.card->setEnabled(member.queueAvailable);
            const bool selected = IsSelectedJob(
                member.identity, row, static_cast<int>(slot));
            card.card->setStateSelected(selected);
            card.card->eventMouseButtonPressed += MyGUI::newDelegate(OnCardPressed);
            card.card->eventMouseDrag += MyGUI::newDelegate(OnCardDrag);
            card.card->eventMouseButtonReleased += MyGUI::newDelegate(OnCardReleased);
            card.card->eventMouseSetFocus += MyGUI::newDelegate(OnCardMouseSetFocus);
            card.card->eventMouseLostFocus += MyGUI::newDelegate(OnCardMouseLostFocus);
            card.card->setNeedToolTip(true);
            card.card->eventToolTip += MyGUI::newDelegate(OnCardToolTip);
            card.highlightGroup = FindJobHighlightGroup(row);

            // Kenshi_Button1 is a dark native atlas skin. Put the text-heavy
            // queue card on an explicit palette surface while keeping the
            // button itself, its state, and all drag callbacks unchanged.
            MyGUI::Widget* cardSurface = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(3, 3, CARD_WIDTH - 6, CARD_HEIGHT - 6),
                MyGUI::Align::Stretch,
                "KJM_JobCardSurface");
            TagThemeSurface(cardSurface);

            std::ostringstream tooltip;
            tooltip << member.name << " | Priority " << (slot + 1)
                    << "\n" << StripLeadingPriorityPrefix(row.jobLabel);
            if (showTarget)
            {
                tooltip << "\nTarget: " << row.targetLabel;
            }
            card.card->setUserString("KJM_ToolTip", tooltip.str());

            // Reuse the square category artwork from the Stations tab. The
            // card is wider than it is tall, so keep the source at its native
            // 1:1 aspect ratio and center it instead of stretching it across
            // the full queue cell. A matching dark tint gives it the same
            // blended treatment as station headers while the text remains
            // readable above it.
            if (g_stationIconLocationRegistered &&
                (roleIconResource != NULL || hasStationArtworkTarget))
            {
                const int iconSize = CARD_HEIGHT;
                const int iconLeft = (CARD_WIDTH - iconSize) / 2;
                card.categoryIcon =
                    card.card->createWidget<MyGUI::ImageBox>(
                        "ImageBox",
                        MyGUI::IntCoord(iconLeft, 0, iconSize, iconSize),
                        MyGUI::Align::Left | MyGUI::Align::Top,
                        "KJM_JobCategoryBackground");
                card.categoryIcon->setDepth(10);
                // The artwork itself is 33% opaque (67% transparent). Keep
                // this independent from the parent/card alpha so Jobs OFF and
                // read-only row dimming never makes the text compete with a
                // suddenly bright icon.
                card.categoryIcon->setAlpha(0.33f);
                card.categoryIcon->setInheritsAlpha(false);
                card.categoryIcon->setNeedMouseFocus(false);
                card.categoryIcon->setVisible(false);
                card.categoryOverlay =
                    card.card->createWidget<MyGUI::Widget>(
                        "WhiteSkin",
                        MyGUI::IntCoord(0, 0, CARD_WIDTH, CARD_HEIGHT),
                        MyGUI::Align::Stretch,
                        "KJM_JobCategoryOverlay");
                TagThemeBackground(card.categoryOverlay);
                // Keep the dark card treatment behind the now-transparent
                // symbol. The icon owns its exact opacity; this layer must not
                // multiply it down to roughly 11% visibility.
                card.categoryOverlay->setAlpha(0.67f);
                card.categoryOverlay->setDepth(11);
                card.categoryOverlay->setNeedMouseFocus(false);
                card.categoryOverlay->setVisible(false);

                if (roleIconResource != NULL)
                {
                    const bool applied = TrySetJobStationCategoryIcon(
                        card.categoryIcon, roleIconResource);
                    card.categoryIcon->setVisible(applied);
                    card.categoryOverlay->setVisible(applied);
                }
                else
                {
                    StationCategory stationCategory = STATION_OTHER;
                    StationVisualSubtype stationVisualSubtype =
                        STATION_VISUAL_DEFAULT;
                    if (TryGetCachedJobStationCategory(
                            row.target, &stationCategory,
                            &stationVisualSubtype))
                    {
                        SetJobStationCategoryArtwork(
                            card.categoryIcon,
                            card.categoryOverlay,
                            stationCategory,
                            stationVisualSubtype);
                    }
                    else
                    {
                        if (!QueueJobStationCategory(row.target))
                        {
                            SetJobStationCategoryArtwork(
                                card.categoryIcon,
                                card.categoryOverlay,
                                STATION_OTHER,
                                STATION_VISUAL_DEFAULT);
                        }
                    }
                }
            }

            const bool targetUnavailable =
                JobRowTargetUnavailable(row);

            // Keep an unavailable target visible at a glance.  The warning
            // layer is inset only slightly, leaving the button edge intact;
            // the selected green layer below it is inset farther so a
            // selected unavailable job still has a distinct green center and
            // red warning border.
            card.unavailableMarker = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(2, 2, CARD_WIDTH - 4, CARD_HEIGHT - 4),
                MyGUI::Align::Stretch,
                "KJM_CardUnavailableTint");
            card.unavailableMarker->setColour(MyGUI::Colour(0.95f, 0.18f, 0.14f));
            card.unavailableMarker->setAlpha(0.34f);
            card.unavailableMarker->setNeedMouseFocus(false);
            card.unavailableMarker->setVisible(targetUnavailable);

            // Kenshi_Button1 does not provide a distinct checked texture.
            // Tint the card when selected.  This layer stays behind the text
            // and can also sit over future job-specific artwork.
            card.selectionMarker = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(5, 5, CARD_WIDTH - 10, CARD_HEIGHT - 10),
                MyGUI::Align::Stretch,
                "KJM_CardSelectionTint");
            card.selectionMarker->setColour(JobSelectionMarkerColour());
            card.selectionMarker->setAlpha(0.24f);
            card.selectionMarker->setNeedMouseFocus(false);
            card.selectionMarker->setVisible(selected);

            // Keep hover highlighting independent from the selected tint and
            // unavailable warning. Four thin edges make this an outline even
            // when either of those layers is already visible.
            card.highlightTop = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(1, 1, CARD_WIDTH - 2, 2),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_CardHighlightTop");
            card.highlightBottom = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(1, CARD_HEIGHT - 3, CARD_WIDTH - 2, 2),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_CardHighlightBottom");
            card.highlightLeft = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(1, 3, 2, CARD_HEIGHT - 6),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_CardHighlightLeft");
            card.highlightRight = card.card->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(CARD_WIDTH - 3, 3, 2, CARD_HEIGHT - 6),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_CardHighlightRight");
            SetCardHighlightVisible(card, false);
            card.highlightTop->setColour(MyGUI::Colour(1.0f, 0.84f, 0.24f));
            card.highlightBottom->setColour(MyGUI::Colour(1.0f, 0.84f, 0.24f));
            card.highlightLeft->setColour(MyGUI::Colour(1.0f, 0.84f, 0.24f));
            card.highlightRight->setColour(MyGUI::Colour(1.0f, 0.84f, 0.24f));
            card.highlightTop->setAlpha(0.92f);
            card.highlightBottom->setAlpha(0.92f);
            card.highlightLeft->setAlpha(0.92f);
            card.highlightRight->setAlpha(0.92f);
            card.highlightTop->setNeedMouseFocus(false);
            card.highlightBottom->setNeedMouseFocus(false);
            card.highlightLeft->setNeedMouseFocus(false);
            card.highlightRight->setNeedMouseFocus(false);

            card.job = card.card->createWidget<MyGUI::TextBox>(
                // The normal 20pt preset is clear for short work types;
                // SetFittedCardJobCaption scales and wraps longer labels.
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(8, 2, CARD_WIDTH - 16, 38),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_CardJob");
            if (card.job->getFontHeight() > 0)
            {
                card.job->setFontHeight(card.job->getFontHeight() + 1);
            }
            SetFittedCardJobCaption(card.job, row.jobLabel);
            card.job->setTextAlign(MyGUI::Align::Center);
            // Use pure white for the primary card text.  Kenshi's default
            // warm gray is difficult to read against the dark card skin.
            TagThemeStandardText(card.job);
            card.job->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            card.job->setAlpha(1.0f);
            card.job->setInheritsAlpha(false);
            card.job->setNeedMouseFocus(false);

            card.arrow = card.card->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(0, 40, CARD_WIDTH, 15),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_CardArrow");
            if (card.arrow->getFontHeight() > 0)
            {
                card.arrow->setFontHeight(card.arrow->getFontHeight() + 1);
            }
            card.arrow->setCaption(showTarget ? "V" : "");
            card.arrow->setTextAlign(MyGUI::Align::Center);
            TagThemeAccentText(card.arrow);
            card.arrow->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            card.arrow->setAlpha(1.0f);
            card.arrow->setInheritsAlpha(false);
            card.arrow->setNeedMouseFocus(false);

            card.target = card.card->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(8, 56, CARD_WIDTH - 16, 27),
                MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_CardTarget");
            if (card.target->getFontHeight() > 0)
            {
                card.target->setFontHeight(card.target->getFontHeight() + 1);
            }
            if (showTarget)
            {
                SetFittedCardTargetCaption(card.target, row.targetLabel);
                if (targetUnavailable)
                {
                    TagThemeWarningText(card.target);
                }
                else
                {
                    TagThemeStandardText(card.target);
                }
            }
            else
            {
                card.target->setCaption("");
            }
            card.target->setTextAlign(MyGUI::Align::Center);
            card.target->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            card.target->setAlpha(1.0f);
            card.target->setInheritsAlpha(false);
            card.target->setNeedMouseFocus(false);
            widgets.cards.push_back(card);
        }
        BindSquadMouseWheelTree(widgets.jobsRoot);
    }

    void ApplyMemberWidgets(size_t memberIndex, bool forcePortrait)
    {
        if (memberIndex >= g_memberWidgets.size())
        {
            return;
        }
        const MemberSnapshot* memberPointer = GetVisibleMember(memberIndex);
        if (memberPointer == NULL)
        {
            return;
        }
        const MemberSnapshot& member = *memberPointer;
        MemberWidgets& widgets = g_memberWidgets[memberIndex];

        widgets.name->setCaption(member.name.c_str());
        ApplyThemeStandardText(widgets.name);
        widgets.condition->setCaption(member.condition.c_str());
        widgets.condition->setVisible(!member.condition.empty());
        if (!member.condition.empty())
        {
            ApplyThemeWarningText(widgets.condition);
        }
        const bool hasHighestSkill = member.loaded && !member.skills.empty();
        if (widgets.highestSkill != NULL)
        {
            const std::string highestSkillCaption =
                hasHighestSkill ?
                    BuildSkillLineCaption(member.skills[0]) : "";
            widgets.highestSkill->setCaption(highestSkillCaption.c_str());
            widgets.highestSkill->setVisible(hasHighestSkill);
            ApplyThemeStandardText(widgets.highestSkill);
        }
        const std::string skillCaption = BuildSkillCaption(member, 1);
        widgets.skills->setCaption(skillCaption.c_str());
        ApplyThemeStandardText(widgets.skills);

        widgets.jobsToggle->setCaption(member.jobsEnabled ? "Jobs: ON" : "Jobs: OFF");
        widgets.jobsToggle->setFontHeight(12);
        widgets.jobsToggle->setTextAlign(MyGUI::Align::Center);
        ApplyThemeButtonText(widgets.jobsToggle);
        widgets.jobsToggle->setEnabled(member.queueAvailable);
        widgets.clearButton->setEnabled(member.queueAvailable && !member.jobs.empty());
        widgets.clearButton->setFontHeight(12);
        widgets.clearButton->setTextAlign(MyGUI::Align::Center);
        ApplyThemeButtonText(widgets.clearButton);
        widgets.jobsRoot->setAlpha(
            member.queueAvailable ? (member.jobsEnabled ? 1.0f : 0.42f) : 0.35f);

        ApplyPortrait(widgets, member, forcePortrait);

        if (widgets.appliedRevision != member.revision)
        {
            DestroyCardWidgets(widgets);
            CreateCardWidgets(memberIndex, widgets);
            widgets.appliedRevision = member.revision;
        }
    }

    void ApplyRecipientSelectionStates()
    {
        for (size_t index = 0; index < g_memberWidgets.size(); ++index)
        {
            const MemberSnapshot* member = GetVisibleMember(index);
            if (member == NULL)
            {
                continue;
            }
            const bool selected = IsRecipientSelected(member->identity);
            if (g_memberWidgets[index].portraitBorder != NULL)
            {
                g_memberWidgets[index].portraitBorder->setStateSelected(selected);
            }
            if (g_memberWidgets[index].recipientMarker != NULL)
            {
                g_memberWidgets[index].recipientMarker->setVisible(selected);
            }
        }
    }

    void CreateMemberWidgets(size_t memberIndex)
    {
        if (g_memberCanvas == NULL || g_jobCanvas == NULL ||
            memberIndex >= g_visibleMemberBindings.size())
        {
            return;
        }

        const MemberSnapshot* member = GetVisibleMember(memberIndex);
        if (member == NULL)
        {
            return;
        }

        MemberWidgets widgets;
        const int y = g_visibleMemberBindings[memberIndex].rowTop;
        widgets.memberRoot = g_memberCanvas->createWidget<MyGUI::Widget>(
            "Kenshi_SelectionPanel",
            MyGUI::IntCoord(0, y, g_memberWidth, ROW_HEIGHT),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_MemberRow");

        // SelectionPanel is a light vanilla texture and does not follow the
        // optional dark palette. Keep the row surface explicit so member
        // labels remain readable after a live theme toggle.
        MyGUI::Widget* memberSurface =
            widgets.memberRoot->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(3, 3, g_memberWidth - 6, ROW_HEIGHT - 6),
                MyGUI::Align::Stretch,
                "KJM_MemberRowSurface");
        TagThemeSurface(memberSurface);

        widgets.recipientMarker =
            widgets.memberRoot->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(0, 5, 5, ROW_HEIGHT - 10),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_RecipientMarker");
        widgets.recipientMarker->setColour(
            MyGUI::Colour(0.34f, 0.92f, 0.48f));
        widgets.recipientMarker->setAlpha(0.95f);
        widgets.recipientMarker->setNeedMouseFocus(false);
        widgets.recipientMarker->setVisible(false);

        widgets.portraitBorder = widgets.memberRoot->createWidget<MyGUI::Button>(
            "Kenshi_PortraitFrameSkin",
            MyGUI::IntCoord(7, 11, MEMBER_PORTRAIT_SIZE, MEMBER_PORTRAIT_SIZE),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_PortraitFrame");
        widgets.portraitBorder->setNeedMouseFocus(true);
        widgets.portraitBorder->setUserString(
            "KJM_Member", IntegerString(memberIndex));
        widgets.portraitBorder->eventMouseButtonClick +=
            MyGUI::newDelegate(OnRecipientPortraitClicked);
        widgets.portraitBackground = widgets.portraitBorder->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_PortraitBackground");
        widgets.portrait = widgets.portraitBorder->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_Portrait");
        widgets.portraitBackOverlay = widgets.portraitBorder->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_PortraitBackOverlay");
        widgets.portraitFrontOverlay = widgets.portraitBorder->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_PortraitFrontOverlay");

        // Match Kenshi_PortraitCharacter.layout exactly.  MyGUI renders a
        // lower depth over a higher depth.  Without these depths the gray
        // Background/Normal image can cover the generated character atlas,
        // which makes every otherwise valid portrait look blank.
        widgets.portraitBackground->setDepth(5);
        widgets.portraitBackOverlay->setDepth(4);
        widgets.portrait->setDepth(3);
        widgets.portraitFrontOverlay->setDepth(2);
        widgets.portraitBackground->setNeedMouseFocus(false);
        widgets.portrait->setNeedMouseFocus(false);
        widgets.portraitBackOverlay->setNeedMouseFocus(false);
        widgets.portraitFrontOverlay->setNeedMouseFocus(false);

        widgets.name = widgets.memberRoot->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(
                MEMBER_TEXT_LEFT, 8, g_memberWidth - MEMBER_TEXT_LEFT - 7, 25),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_MemberName");
        widgets.name->setFontHeight(21);
        widgets.name->setNeedMouseFocus(false);
        TagThemeStandardText(widgets.name);
        widgets.condition = widgets.memberRoot->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(
                MEMBER_TEXT_LEFT, 33, g_memberWidth - MEMBER_TEXT_LEFT - 7, 16),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_MemberCondition");
        widgets.condition->setFontHeight(15);
        widgets.condition->setNeedMouseFocus(false);
        TagThemeWarningText(widgets.condition);
        widgets.highestSkill = widgets.memberRoot->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(
                MEMBER_TEXT_LEFT, 49, g_memberWidth - MEMBER_TEXT_LEFT - 7, 18),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_MemberHighestSkill");
        widgets.highestSkill->setFontHeight(18);
        widgets.highestSkill->setTextAlign(
            MyGUI::Align::Left | MyGUI::Align::Top);
        widgets.highestSkill->setNeedMouseFocus(false);
        TagThemeStandardText(widgets.highestSkill);
        widgets.skills = widgets.memberRoot->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(
                MEMBER_TEXT_LEFT, 67, g_memberWidth - MEMBER_TEXT_LEFT - 7, 32),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_MemberSkills");
        widgets.skills->setFontHeight(16);
        widgets.skills->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        widgets.skills->setNeedMouseFocus(false);
        TagThemeStandardText(widgets.skills);

        widgets.jobsToggle = widgets.memberRoot->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(7, ROW_HEIGHT - 23, 102, 20),
            MyGUI::Align::Left | MyGUI::Align::Bottom,
            "KJM_JobsToggle");
        widgets.jobsToggle->setUserString("KJM_Member", IntegerString(memberIndex));
        widgets.jobsToggle->eventMouseButtonClick += MyGUI::newDelegate(OnJobsToggleClicked);
        TagThemeButtonText(widgets.jobsToggle);

        widgets.clearButton = widgets.memberRoot->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(114, ROW_HEIGHT - 23, 122, 20),
            MyGUI::Align::Left | MyGUI::Align::Bottom,
            "KJM_ClearMember");
        widgets.clearButton->setCaption("Clear Queue");
        widgets.clearButton->setFontHeight(12);
        widgets.clearButton->setTextAlign(MyGUI::Align::Center);
        TagThemeButtonText(widgets.clearButton);
        widgets.clearButton->setUserString("KJM_Member", IntegerString(memberIndex));
        widgets.clearButton->eventMouseButtonClick += MyGUI::newDelegate(OnClearClicked);

        int canvasWidth = g_jobWidth;
        if (!member->jobs.empty())
        {
            canvasWidth = std::max(
                g_jobWidth,
                static_cast<int>(member->jobs.size()) * CARD_STRIDE);
        }
        widgets.jobsRoot = g_jobCanvas->createWidget<MyGUI::Widget>(
            "PanelEmpty",
            MyGUI::IntCoord(0, y, canvasWidth, ROW_HEIGHT),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_JobRow");
        widgets.jobsRoot->eventMouseButtonPressed += MyGUI::newDelegate(OnEmptyPressed);

        g_memberWidgets.push_back(widgets);
        ApplyMemberWidgets(memberIndex, true);
        BindSquadMouseWheelTree(widgets.memberRoot);
        BindSquadMouseWheelTree(widgets.jobsRoot);
    }

    size_t GetMaximumJobCount()
    {
        return GetDisplayedMaximumJobCount();
    }

    void DestroyPriorityLabels()
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            for (size_t index = 0; index < g_priorityLabels.size(); ++index)
            {
                if (g_priorityLabels[index] != NULL)
                {
                    gui->destroyWidget(g_priorityLabels[index]);
                }
            }
        }
        g_priorityLabels.clear();
    }

    void RebuildPriorityLabels()
    {
        DestroyPriorityLabels();
        if (g_priorityCanvas == NULL)
        {
            return;
        }

        const size_t maximum = GetMaximumJobCount();
        for (size_t slot = 0; slot < maximum; ++slot)
        {
            MyGUI::TextBox* label = g_priorityCanvas->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(
                    static_cast<int>(slot) * CARD_STRIDE,
                    0,
                    CARD_WIDTH,
                    HEADER_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_PriorityHeader");
            std::ostringstream caption;
            caption << "Priority " << (slot + 1);
            label->setCaption(caption.str().c_str());
            label->setTextAlign(MyGUI::Align::Center);
            label->setNeedMouseFocus(false);
            TagThemeStandardText(label);
            g_priorityLabels.push_back(label);
        }
    }

    void SetSquadWidgetVisibleIfChanged(
        MyGUI::Widget* widget,
        bool visible)
    {
        if (widget != NULL && widget->getVisible() != visible)
        {
            widget->setVisible(visible);
        }
    }

    bool IntersectsSquadViewportAxis(
        int itemStart,
        int itemLength,
        int viewportStart,
        int viewportLength,
        int overscan)
    {
        const int visibleStart = viewportStart - overscan;
        const int visibleEnd = viewportStart + viewportLength + overscan;
        return itemStart < visibleEnd &&
            itemStart + itemLength > visibleStart;
    }

    void ApplySquadViewportVisibility()
    {
        // MyGUI clips descendants to the PanelEmpty viewports, but a visible
        // row root can still make it traverse every horizontally offscreen
        // card subtree. Explicit root visibility lets MyGUI reject that work
        // before it reaches the card surfaces, artwork, outlines, and text.
        // Keep one row/card ready outside each edge so incremental scrolling
        // does not reveal a blank strip before the next input callback.
        for (size_t memberIndex = 0;
             memberIndex < g_memberWidgets.size(); ++memberIndex)
        {
            MemberWidgets& widgets = g_memberWidgets[memberIndex];
            const bool rowIntersectsViewport =
                memberIndex < g_visibleMemberBindings.size() &&
                IntersectsSquadViewportAxis(
                    g_visibleMemberBindings[memberIndex].rowTop,
                    ROW_HEIGHT,
                    g_verticalOffset,
                    g_bodyHeight,
                    ROW_STRIDE);
            // Do not hide the captured source subtree during edge-scroll.
            // The viewport still clips it, while MyGUI keeps the drag's mouse
            // capture and delivers the eventual release callback.
            const bool dragSourceRow = g_drag.armed &&
                g_drag.memberIndex == static_cast<int>(memberIndex);
            const bool rowVisible =
                rowIntersectsViewport || dragSourceRow;
            SetSquadWidgetVisibleIfChanged(
                widgets.memberRoot, rowVisible);
            SetSquadWidgetVisibleIfChanged(
                widgets.jobsRoot, rowVisible);

            for (size_t cardIndex = 0;
                 cardIndex < widgets.cards.size(); ++cardIndex)
            {
                CardWidgets& card = widgets.cards[cardIndex];
                const bool dragSourceCard = dragSourceRow &&
                    card.slot == g_drag.slot;
                const bool cardVisible = dragSourceCard ||
                    (rowIntersectsViewport && card.slot >= 0 &&
                        IntersectsSquadViewportAxis(
                            card.slot * CARD_STRIDE,
                            CARD_WIDTH,
                            g_horizontalOffset,
                            g_jobWidth,
                            CARD_STRIDE));
                SetSquadWidgetVisibleIfChanged(
                    card.card, cardVisible);
            }
        }

        for (size_t groupIndex = 0;
             groupIndex < g_squadGroupWidgets.size(); ++groupIndex)
        {
            SquadGroupWidgets& group = g_squadGroupWidgets[groupIndex];
            bool memberHeaderVisible = false;
            if (group.memberHeader != NULL)
            {
                const MyGUI::IntCoord coord =
                    group.memberHeader->getCoord();
                memberHeaderVisible = IntersectsSquadViewportAxis(
                    coord.top,
                    coord.height,
                    g_verticalOffset,
                    g_bodyHeight,
                    ROW_STRIDE);
            }
            SetSquadWidgetVisibleIfChanged(
                group.memberHeader, memberHeaderVisible);
            SetSquadWidgetVisibleIfChanged(
                group.jobHeader, memberHeaderVisible);
        }

        for (size_t slot = 0; slot < g_priorityLabels.size(); ++slot)
        {
            const bool labelVisible = IntersectsSquadViewportAxis(
                static_cast<int>(slot) * CARD_STRIDE,
                CARD_WIDTH,
                g_horizontalOffset,
                g_jobWidth,
                CARD_STRIDE);
            SetSquadWidgetVisibleIfChanged(
                g_priorityLabels[slot], labelVisible);
        }
    }

    void ApplyScrollOffsets()
    {
        if (g_memberCanvas != NULL)
        {
            g_memberCanvas->setPosition(0, -g_verticalOffset);
        }
        if (g_jobCanvas != NULL)
        {
            g_jobCanvas->setPosition(-g_horizontalOffset, -g_verticalOffset);
        }
        if (g_priorityCanvas != NULL)
        {
            g_priorityCanvas->setPosition(-g_horizontalOffset, 0);
        }
        ApplySquadViewportVisibility();
    }

    void UpdateScrollRanges()
    {
        const int contentHeight = GetVisibleContentHeight();
        const int contentWidth =
            static_cast<int>(GetMaximumJobCount()) * CARD_STRIDE;
        const int canvasHeight = std::max(g_bodyHeight, contentHeight);
        const int canvasWidth = std::max(g_jobWidth, contentWidth);
        if (g_memberCanvas != NULL)
        {
            g_memberCanvas->setSize(g_memberWidth, canvasHeight);
        }
        if (g_jobCanvas != NULL)
        {
            g_jobCanvas->setSize(canvasWidth, canvasHeight);
        }
        if (g_priorityCanvas != NULL)
        {
            g_priorityCanvas->setSize(canvasWidth, HEADER_HEIGHT);
        }
        for (size_t index = 0; index < g_memberWidgets.size(); ++index)
        {
            if (g_memberWidgets[index].jobsRoot != NULL)
            {
                g_memberWidgets[index].jobsRoot->setSize(canvasWidth, ROW_HEIGHT);
            }
        }
        for (size_t index = 0; index < g_squadGroupWidgets.size(); ++index)
        {
            if (g_squadGroupWidgets[index].jobHeader != NULL)
            {
                g_squadGroupWidgets[index].jobHeader->setSize(
                    canvasWidth, SQUAD_GROUP_HEADER_HEIGHT);
            }
        }
        g_maxVerticalOffset = std::max(0, contentHeight - g_bodyHeight);
        g_maxHorizontalOffset = std::max(0, contentWidth - g_jobWidth);
        g_verticalOffset = ClampInt(g_verticalOffset, 0, g_maxVerticalOffset);
        g_horizontalOffset = ClampInt(g_horizontalOffset, 0, g_maxHorizontalOffset);

        g_changingScroll = true;
        if (g_verticalScroll != NULL)
        {
            g_verticalScroll->setScrollRange(static_cast<size_t>(g_maxVerticalOffset + 1));
            g_verticalScroll->setScrollPage(static_cast<size_t>(std::max(1, g_bodyHeight)));
            g_verticalScroll->setScrollViewPage(static_cast<size_t>(ROW_STRIDE));
            g_verticalScroll->setScrollPosition(static_cast<size_t>(g_verticalOffset));
            g_verticalScroll->setEnabled(g_maxVerticalOffset > 0);
        }
        if (g_horizontalScroll != NULL)
        {
            g_horizontalScroll->setScrollRange(static_cast<size_t>(g_maxHorizontalOffset + 1));
            g_horizontalScroll->setScrollPage(static_cast<size_t>(std::max(1, g_jobWidth)));
            g_horizontalScroll->setScrollViewPage(static_cast<size_t>(CARD_STRIDE));
            g_horizontalScroll->setScrollPosition(static_cast<size_t>(g_horizontalOffset));
            g_horizontalScroll->setEnabled(g_maxHorizontalOffset > 0);
        }
        g_changingScroll = false;
        ApplyScrollOffsets();
    }

    void DestroyMemberWidgets()
    {
        if (g_tooltip != NULL)
        {
            g_tooltip->setVisible(false);
        }
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            for (size_t index = 0; index < g_memberWidgets.size(); ++index)
            {
                if (g_memberWidgets[index].memberRoot != NULL)
                {
                    gui->destroyWidget(g_memberWidgets[index].memberRoot);
                }
                if (g_memberWidgets[index].jobsRoot != NULL)
                {
                    gui->destroyWidget(g_memberWidgets[index].jobsRoot);
                }
            }
        }
        g_memberWidgets.clear();
        if (gui != NULL)
        {
            for (size_t index = 0; index < g_squadGroupWidgets.size(); ++index)
            {
                if (g_squadGroupWidgets[index].memberHeader != NULL)
                {
                    gui->destroyWidget(g_squadGroupWidgets[index].memberHeader);
                }
                if (g_squadGroupWidgets[index].jobHeader != NULL)
                {
                    gui->destroyWidget(g_squadGroupWidgets[index].jobHeader);
                }
            }
        }
        ResetSquadGroupViewState();
    }

    void CreateSquadGroupHeader(
        size_t squadIndex,
        int y,
        int canvasWidth)
    {
        if (g_memberCanvas == NULL || g_jobCanvas == NULL ||
            squadIndex >= g_allSquads.squads.size())
        {
            return;
        }
        const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
        const bool collapsed = IsSquadCollapsed(squad.identity);
        const bool selected = SameHandleIdentity(
            squad.identity, g_squad.identity);

        SquadGroupWidgets group;
        group.squad = squad.identity;
        group.name = squad.name;
        group.memberCount = squad.members.size();
        group.collapsed = collapsed;
        group.memberHeader = g_memberCanvas->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(0, y, g_memberWidth, SQUAD_GROUP_HEADER_HEIGHT),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_SquadGroupHeader");
        std::ostringstream caption;
        caption << (collapsed ? "+  " : "-  ")
                << (squad.name.empty() ? "Squad" : squad.name)
                << "  |  " << squad.members.size();
        group.memberHeader->setCaption(caption.str());
        group.memberHeader->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Center);
        group.memberHeader->setFontHeight(17);
        TagThemeButtonText(group.memberHeader);
        group.memberHeader->setStateSelected(selected);
        group.memberHeader->setUserString(
            "KJM_SquadGroup", IntegerString(squadIndex));
        group.memberHeader->eventMouseButtonClick +=
            MyGUI::newDelegate(OnSquadGroupToggle);

        group.jobHeader = g_jobCanvas->createWidget<MyGUI::Widget>(
            "WhiteSkin",
            MyGUI::IntCoord(0, y, canvasWidth, SQUAD_GROUP_HEADER_HEIGHT),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_SquadGroupJobHeader");
        TagThemeBackground(group.jobHeader);
        group.jobHeader->setAlpha(selected ? 0.96f : 0.82f);
        group.jobHeader->setNeedMouseFocus(false);
        MyGUI::TextBox* jobCaption =
            group.jobHeader->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(8, 0, std::max(80, g_jobWidth - 16),
                                SQUAD_GROUP_HEADER_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_SquadGroupJobCaption");
        jobCaption->setCaption(selected ? "CURRENT SQUAD" : "");
        jobCaption->setFontHeight(15);
        jobCaption->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Center);
        TagThemeAccentText(jobCaption);
        jobCaption->setNeedMouseFocus(false);
        g_squadGroupWidgets.push_back(group);
        BindSquadMouseWheelTree(group.memberHeader);
        BindSquadMouseWheelTree(group.jobHeader);
    }

    void BuildVisibleMemberBindings()
    {
        g_visibleMemberBindings.clear();
        int y = 0;
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            y += SQUAD_GROUP_HEADER_HEIGHT;
            if (!IsSquadCollapsed(squad.identity))
            {
                for (size_t memberIndex = 0;
                     memberIndex < squad.members.size(); ++memberIndex)
                {
                    VisibleMemberBinding binding;
                    binding.squad = squad.identity;
                    binding.member = squad.members[memberIndex].identity;
                    binding.squadIndex = squadIndex;
                    binding.memberIndex = memberIndex;
                    binding.rowTop = y;
                    g_visibleMemberBindings.push_back(binding);
                    y += ROW_STRIDE;
                }
            }
            if (squadIndex + 1 < g_allSquads.squads.size())
            {
                y += SQUAD_GROUP_GAP;
            }
        }
    }

    void RebuildSquadWidgets()
    {
        CancelDrag();
        DestroyMemberWidgets();
        BuildVisibleMemberBindings();
        const int contentHeight = std::max(
            g_bodyHeight,
            GetVisibleContentHeight());
        const int contentWidth = std::max(
            g_jobWidth,
            static_cast<int>(GetMaximumJobCount()) * CARD_STRIDE);
        if (g_memberCanvas != NULL)
        {
            g_memberCanvas->setSize(g_memberWidth, contentHeight);
        }
        if (g_jobCanvas != NULL)
        {
            g_jobCanvas->setSize(contentWidth, contentHeight);
        }
        if (g_priorityCanvas != NULL)
        {
            g_priorityCanvas->setSize(contentWidth, HEADER_HEIGHT);
        }

        int y = 0;
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            CreateSquadGroupHeader(squadIndex, y, contentWidth);
            y += SQUAD_GROUP_HEADER_HEIGHT;
            if (!IsSquadCollapsed(g_allSquads.squads[squadIndex].identity))
            {
                y += static_cast<int>(
                    g_allSquads.squads[squadIndex].members.size()) * ROW_STRIDE;
            }
            if (squadIndex + 1 < g_allSquads.squads.size())
            {
                y += SQUAD_GROUP_GAP;
            }
        }

        g_memberWidgets.reserve(g_visibleMemberBindings.size());
        for (size_t memberIndex = 0;
             memberIndex < g_visibleMemberBindings.size(); ++memberIndex)
        {
            CreateMemberWidgets(memberIndex);
        }
        RebuildPriorityLabels();
        UpdateScrollRanges();
        ApplyCardSelectionStates();
    }

    void UpdateSquadHeading()
    {
        if (g_prioritizeCoreJobsButton != NULL)
        {
            g_prioritizeCoreJobsButton->setEnabled(
                RecipientSelectionMode() && !g_allSquads.incomplete &&
                !HasPendingJobBatchUiAction() && !g_drag.armed);
        }
        if (g_addHealingJobsButton != NULL)
        {
            g_addHealingJobsButton->setEnabled(
                RecipientSelectionMode() && !g_allSquads.incomplete &&
                !HasPendingJobBatchUiAction() && !g_drag.armed);
        }
        if (g_removeInvalidJobsButton != NULL)
        {
            g_removeInvalidJobsButton->setEnabled(
                RecipientSelectionMode() && !g_allSquads.incomplete &&
                !HasPendingJobBatchUiAction() && !g_drag.armed);
        }
        if (g_squadText != NULL)
        {
            std::ostringstream caption;
            caption << (g_squad.name.empty() ? "Current squad" : g_squad.name)
                    << "  |  " << g_squad.members.size() << " member";
            if (g_squad.members.size() != 1)
            {
                caption << "s";
            }
            if (!g_squad.live)
            {
                caption << "  |  read-only snapshot";
            }
            if (g_allSquads.squads.size() > 1)
            {
                caption << "  |  " << g_allSquads.squads.size() << " squads";
            }
            if (g_allSquads.incomplete)
            {
                caption << "  |  roster incomplete";
            }
            caption << "  |  " << g_selectedRecipients.size()
                    << " recipient";
            if (g_selectedRecipients.size() != 1)
            {
                caption << "s";
            }
            g_squadText->setCaption(caption.str().c_str());
        }

        if (g_emptyText != NULL)
        {
            if (g_allSquads.squads.empty())
            {
                g_emptyText->setVisible(true);
                g_emptyText->setCaption(
                    g_squad.unavailable
                        ? "The active squads are unavailable and have no session snapshot."
                        : "There are no active squads with job-manageable members.");
            }
            else
            {
                g_emptyText->setVisible(false);
            }
        }
    }

    bool RefreshSquadView(bool force)
    {
        if (g_window == NULL)
        {
            return false;
        }

        if (!TrySkipDeadCurrentSquad())
        {
            SetStatus(
                "No active squad is available. Kenshi's __DEAD__ holding squad is hidden.");
            return false;
        }

        // Read only the current identity before the complete all-squad pass.
        // The matching value snapshot below becomes g_squad, which avoids a
        // second full member/stat/job capture for the current squad.
        hand currentHandle;
        std::string currentName;
        RootObjectContainer* currentActive = NULL;
        if (!TryGetCurrentSquad(
                g_playerInterface,
                &currentHandle,
                &currentName,
                &currentActive))
        {
            SetStatus("Kenshi did not expose a current squad. No jobs were changed.");
            return false;
        }
        HandleIdentity currentIdentity;
        CaptureHandleIdentity(currentHandle, &currentIdentity);

        const HandleIdentity previousCurrent = g_squad.identity;
        const unsigned int previousBoardRevision = g_allSquads.revision;
        const bool refreshed = RefreshAllActiveSquadsSnapshot();

        SquadSnapshot fallbackCurrent;
        const SquadSnapshot* nextCurrent = NULL;
        const int currentSquadIndex =
            FindAllSquadSnapshotIndex(currentIdentity);
        if (currentSquadIndex >= 0)
        {
            nextCurrent = &g_allSquads.squads[currentSquadIndex];
        }
        else if (!BuildCurrentSquadSnapshot(&fallbackCurrent))
        {
            // Empty or temporarily omitted squads are not part of the active
            // nonempty board. Retain the established current-squad fallback
            // for that exceptional case.
            SetStatus("Kenshi did not expose a current squad. No jobs were changed.");
            return false;
        }
        else
        {
            nextCurrent = &fallbackCurrent;
        }
        const bool currentChanged = !SameHandleIdentity(
            previousCurrent, nextCurrent->identity);
        const bool boardChanged =
            previousBoardRevision != g_allSquads.revision;
        // g_squad remains an independent value snapshot. Avoid two complete
        // nested-vector copies, and retain the existing copy when this current
        // squad publication is unchanged.
        if (!SameSquadSnapshot(g_squad, *nextCurrent))
        {
            g_squad = *nextCurrent;
        }
        if (refreshed)
        {
            PruneSelectedRecipientsToDisplayedRoster();
        }

        if (g_drag.armed && (force || boardChanged || currentChanged))
        {
            CancelDrag();
            SetStatus(
                "A squad or queue changed during the drag. Review it and try again.");
        }

        // Callers use force to request an immediate live refresh after a
        // mutation. It no longer implies a destructive widget rebuild when
        // the squad/member layout is unchanged.
        const bool structureChanged =
            !SquadWidgetStructureMatchesBoard();
        const bool rebuild = structureChanged || currentChanged;
        if (rebuild)
        {
            ClearJobHoverHighlight();
            RebuildJobHighlightCache();
            RepairSelection();
            RebuildSquadWidgets();
            ApplyCachedJobHighlightGroups();
        }
        else
        {
            const bool refreshPresentation = force || boardChanged;
            if (refreshPresentation)
            {
                ClearJobHoverHighlight();
                RebuildJobHighlightCache();
                RepairSelection();
            }
            for (size_t index = 0;
                 index < g_memberWidgets.size(); ++index)
            {
                const MemberSnapshot* member = GetVisibleMember(index);
                if (member == NULL)
                {
                    continue;
                }
                if (g_memberWidgets[index].appliedRevision !=
                    member->revision)
                {
                    ApplyMemberWidgets(index, false);
                }
                else if (member->loaded &&
                         !g_memberWidgets[index].portraitBound)
                {
                    ApplyPortrait(
                        g_memberWidgets[index], *member, false);
                }
            }
            const size_t maximumJobCount = GetMaximumJobCount();
            if (maximumJobCount != g_priorityLabels.size())
            {
                RebuildPriorityLabels();
                UpdateScrollRanges();
            }
            if (refreshPresentation)
            {
                ApplyCardSelectionStates();
                ApplyCachedJobHighlightGroups();
            }
        }

        // Textures are registered before the first Squad refresh. Resolve all
        // exact targets that card creation just queued now, while the manager
        // still owns the current update, so the first rendered frame already
        // contains the final category/subtype artwork. A running Stations scan
        // remains the sole resolver until its copied candidate pass completes.
        if (!g_stationTabActive || !g_stationScan.started ||
            g_stationScan.complete)
        {
            TryDrainPendingJobStationCategoriesGuarded();
        }

        // An incremental member refresh can replace its complete card set
        // without changing either scroll range. Cull those new roots before
        // MyGUI renders the refreshed board.
        ApplySquadViewportVisibility();
        UpdateSquadHeading();
        if (g_squad.live && (force || boardChanged || currentChanged))
        {
            StoreCurrentSquadCache();
        }
        if (g_stationScan.started && boardChanged)
        {
            // The Stations tab joins against copied queue snapshots. Rebuild
            // that read-only join after any observed member/queue change; the
            // The player-station target view can rebuild for this session.
            g_stationAssignmentsDirty = true;
        }
        if (!refreshed)
        {
            SetStatus(
                "The squad roster refresh failed. Cached rows are read-only until the next refresh.");
        }
        return refreshed || !g_allSquads.squads.empty();
    }
