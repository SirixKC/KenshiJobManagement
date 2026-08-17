// SPDX-License-Identifier: GPL-3.0-only
// Deferred HUD entry point for the squad job manager.

#ifndef KENSHI_JOB_MANAGEMENT_HUD_BUTTON_INL
#define KENSHI_JOB_MANAGEMENT_HUD_BUTTON_INL

    // The native OrdersChaseButton is labelled JOBS in Kenshi's main panel.
    // Keep this control as the source of truth and place the plug-in button
    // beside it, rather than creating a second top-level HUD window.
    const char* const HUD_MANAGER_BUTTON_NAME = "KJM_HudJobManagerButton";
    const char* const HUD_MANAGER_BUTTON_TOOLTIP =
        "Open Job Manager (Ctrl+J)";
    const char* const HUD_MANAGER_BUTTON_ICON_TEXTURE = "kjm-hud-icon.png";
    const int HUD_MANAGER_BUTTON_GAP = 2;
    const int HUD_MANAGER_BUTTON_MIN_JOBS_WIDTH = 40;
    const int HUD_MANAGER_BUTTON_MIN_HEIGHT = 16;
    const int HUD_MANAGER_BUTTON_MAX_HEIGHT = 64;

    struct HudButtonContext
    {
        MainBarGUI* mainbar;
        OrdersPanel* ordersPanel;
        MyGUI::Widget* nativeRoot;
        MyGUI::Widget* ordersRoot;
        MyGUI::Button* jobsButton;

        HudButtonContext() :
            mainbar(NULL), ordersPanel(NULL), nativeRoot(NULL),
            ordersRoot(NULL), jobsButton(NULL)
        {
        }
    };

    MyGUI::Button* g_hudManagerButton = NULL;
    MainBarGUI* g_hudMainbar = NULL;
    OrdersPanel* g_hudOrdersPanel = NULL;
    MyGUI::Widget* g_hudNativeRoot = NULL;
    MyGUI::Widget* g_hudOrdersRoot = NULL;
    MyGUI::Button* g_hudJobsButton = NULL;
    MyGUI::IntCoord g_hudOriginalJobsCoord(0, 0, 0, 0);
    MyGUI::IntCoord g_hudSplitJobsCoord(0, 0, 0, 0);
    bool g_hudOriginalJobsCoordValid = false;

    bool SameHudButtonContext(
        const HudButtonContext& context)
    {
        return context.mainbar == g_hudMainbar &&
            context.ordersPanel == g_hudOrdersPanel &&
            context.nativeRoot == g_hudNativeRoot &&
            context.ordersRoot == g_hudOrdersRoot &&
            context.jobsButton == g_hudJobsButton;
    }

    bool SameHudButtonCoord(
        const MyGUI::IntCoord& left,
        const MyGUI::IntCoord& right)
    {
        return left.left == right.left && left.top == right.top &&
            left.width == right.width && left.height == right.height;
    }

    bool TryReadLiveHudManagerButton(
        MyGUI::Widget* ordersRoot,
        MyGUI::Button** buttonOut)
    {
        if (ordersRoot == NULL || buttonOut == NULL)
        {
            return false;
        }
        *buttonOut = NULL;

        MyGUI::Widget* found = ordersRoot->findWidget(
            HUD_MANAGER_BUTTON_NAME);
        if (found == NULL || found->getParent() != ordersRoot)
        {
            return false;
        }
        *buttonOut = found->castType<MyGUI::Button>(false);
        return *buttonOut != NULL;
    }

    // Reset can run while Kenshi is tearing down MyGUI. C++ catch blocks do
    // not catch an access violation from a stale engine widget, so keep the
    // authoritative child lookup behind its own SEH boundary.
    __declspec(noinline) bool TryReadLiveHudManagerButtonGuarded(
        MyGUI::Widget* ordersRoot,
        MyGUI::Button** buttonOut,
        bool* faultedOut)
    {
        if (buttonOut != NULL)
        {
            *buttonOut = NULL;
        }
        if (faultedOut != NULL)
        {
            *faultedOut = false;
        }
        bool found = false;
        bool faulted = false;
        __try
        {
            found = TryReadLiveHudManagerButton(ordersRoot, buttonOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faultedOut != NULL)
        {
            *faultedOut = faulted;
        }
        return !faulted && found;
    }

    void ResetHudManagerButtonBinding()
    {
        g_hudManagerButton = NULL;
        g_hudMainbar = NULL;
        g_hudOrdersPanel = NULL;
        g_hudNativeRoot = NULL;
        g_hudOrdersRoot = NULL;
        g_hudJobsButton = NULL;
        g_hudOriginalJobsCoord = MyGUI::IntCoord(0, 0, 0, 0);
        g_hudSplitJobsCoord = MyGUI::IntCoord(0, 0, 0, 0);
        g_hudOriginalJobsCoordValid = false;
    }

    // Publish the transaction before touching the native JOBS rectangle. If
    // MyGUI faults after a partial setCoord, the guarded reset path can still
    // identify this exact live root/control and attempted split.
    void PublishHudManagerButtonBinding(
        const HudButtonContext& context,
        const MyGUI::IntCoord& original,
        const MyGUI::IntCoord& split,
        MyGUI::Button* managerButton)
    {
        g_hudManagerButton = managerButton;
        g_hudMainbar = context.mainbar;
        g_hudOrdersPanel = context.ordersPanel;
        g_hudNativeRoot = context.nativeRoot;
        g_hudOrdersRoot = context.ordersRoot;
        g_hudJobsButton = context.jobsButton;
        g_hudOriginalJobsCoord = original;
        g_hudSplitJobsCoord = split;
        g_hudOriginalJobsCoordValid = true;
    }

    // This is intentionally a narrow read.  All engine-owned pointers are
    // obtained again from the current live root, and none is retained as a
    // source of truth after a GUI rebuild.
    bool TryReadHudButtonContext(HudButtonContext* context)
    {
        if (context == NULL)
        {
            return false;
        }
        *context = HudButtonContext();

        bool found = false;
        __try
        {
            if (gui == NULL || gui->mainbar == NULL ||
                gui->mainbar->ordersDataPanel == NULL ||
                gui->mainbar->ordersDataPanel->chaseCheckBox == NULL)
            {
                return false;
            }

            context->mainbar = gui->mainbar;
            context->ordersPanel = gui->mainbar->ordersDataPanel;
            context->jobsButton = context->ordersPanel->chaseCheckBox;
            context->ordersRoot = context->jobsButton->getParent();
            context->nativeRoot = context->mainbar->getWidget();
            if (context->jobsButton == NULL || context->ordersRoot == NULL ||
                context->nativeRoot == NULL)
            {
                return false;
            }
            found = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            found = false;
        }
        if (!found)
        {
            *context = HudButtonContext();
        }
        return found;
    }

    bool BuildHudButtonSplit(
        const MyGUI::IntCoord& original,
        MyGUI::IntCoord* jobsCoordOut,
        MyGUI::IntCoord* managerCoordOut)
    {
        if (jobsCoordOut == NULL || managerCoordOut == NULL ||
            original.width <= 0 ||
            original.height < HUD_MANAGER_BUTTON_MIN_HEIGHT ||
            original.height > HUD_MANAGER_BUTTON_MAX_HEIGHT)
        {
            return false;
        }

        // Use the live button height for the square.  The remaining width
        // stays with the native JOBS button, so scaling and layout changes do
        // not require a hard-coded HUD size.
        const int squareSize = original.height;
        const int jobsWidth = original.width - squareSize -
            HUD_MANAGER_BUTTON_GAP;
        if (jobsWidth < HUD_MANAGER_BUTTON_MIN_JOBS_WIDTH)
        {
            return false;
        }

        *jobsCoordOut = MyGUI::IntCoord(
            original.left, original.top, jobsWidth, original.height);
        *managerCoordOut = MyGUI::IntCoord(
            original.left + jobsWidth + HUD_MANAGER_BUTTON_GAP,
            original.top, squareSize, squareSize);
        return true;
    }

    void TryDestroyHudButtonWidget(MyGUI::Button* button)
    {
        if (button == NULL)
        {
            return;
        }
        try
        {
            MyGUI::Gui* manager = MyGUI::Gui::getInstancePtr();
            if (manager != NULL)
            {
                manager->destroyWidget(button);
            }
        }
        catch (...)
        {
            // GUI teardown can already have destroyed a stale child.  The
            // parent/root binding is discarded by the caller in either case.
        }
    }

    __declspec(noinline) bool TryDestroyHudButtonWidgetGuarded(
        MyGUI::Button* button)
    {
        if (button == NULL)
        {
            return true;
        }
        bool completed = false;
        __try
        {
            TryDestroyHudButtonWidget(button);
            completed = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            completed = false;
        }
        return completed;
    }

    __declspec(noinline) bool TryGetHudButtonCoordGuarded(
        MyGUI::Button* button,
        MyGUI::IntCoord* coordOut)
    {
        if (button == NULL || coordOut == NULL)
        {
            return false;
        }
        bool completed = false;
        __try
        {
            *coordOut = button->getCoord();
            completed = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            completed = false;
        }
        return completed;
    }

    __declspec(noinline) bool TrySetHudButtonCoordGuarded(
        MyGUI::Button* button,
        const MyGUI::IntCoord& coord)
    {
        if (button == NULL)
        {
            return false;
        }
        bool completed = false;
        __try
        {
            button->setCoord(coord);
            completed = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            completed = false;
        }
        return completed;
    }

    bool TryRestoreHudButtonIfStillSplit(
        MyGUI::Button* jobsButton,
        const MyGUI::IntCoord& splitCoord,
        const MyGUI::IntCoord& originalCoord)
    {
        MyGUI::IntCoord liveCoord(0, 0, 0, 0);
        if (!TryGetHudButtonCoordGuarded(jobsButton, &liveCoord) ||
            !SameHudButtonCoord(liveCoord, splitCoord))
        {
            return false;
        }
        return TrySetHudButtonCoordGuarded(jobsButton, originalCoord);
    }

    bool CreateHudManagerButton(
        const HudButtonContext& context,
        const MyGUI::IntCoord& original,
        const MyGUI::IntCoord& jobsCoord,
        const MyGUI::IntCoord& managerCoord,
        MyGUI::Button** buttonOut)
    {
        if (buttonOut == NULL)
        {
            return false;
        }
        *buttonOut = NULL;
        MyGUI::Button* managerButton = NULL;
        bool jobsCoordChanged = false;
        try
        {
            PublishHudManagerButtonBinding(
                context, original, jobsCoord, NULL);
            jobsCoordChanged = true;
            context.jobsButton->setCoord(jobsCoord);

            managerButton = context.ordersRoot->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                managerCoord,
                MyGUI::Align::Left | MyGUI::Align::Top,
                HUD_MANAGER_BUTTON_NAME);
            if (managerButton == NULL)
            {
                if (jobsCoordChanged)
                {
                    TryRestoreHudButtonIfStillSplit(
                        context.jobsButton, jobsCoord, original);
                }
                return false;
            }

            // The sibling is deliberately enabled independently of the
            // native JOBS button.  Parent visibility/enabled state still
            // applies, but a disabled Jobs toggle does not disable this entry.
            managerButton->setEnabled(true);
            managerButton->setCaption("JM");
            managerButton->setUserString(
                "KJM_ToolTip", HUD_MANAGER_BUTTON_TOOLTIP);
            managerButton->setNeedToolTip(true);
            managerButton->eventToolTip +=
                MyGUI::newDelegate(OnCardToolTip);
            managerButton->eventMouseButtonClick +=
                MyGUI::newDelegate(OnHudManagerButtonClicked);

            // The image is optional.  StationAssets loads this packaged
            // texture when available; a short caption remains visible if a
            // player has an older/incomplete installation.
            bool iconReady = false;
            if (!g_stationIconLocationRegistered)
            {
                // This is the existing guarded loader.  Its mandatory
                // station set may fail independently; the HUD icon remains
                // optional and keeps the JM caption fallback in that case.
                TryEnsureStationIconResourcesGuarded();
            }
            if (!g_hudManagerIconAvailable)
            {
                // Retry the optional texture independently when the first
                // resource pass happened before Ogre finished its setup.
                TryEnsureHudManagerIconResourceGuarded();
            }
            try
            {
                const int inset = std::max(2, managerCoord.width / 8);
                MyGUI::ImageBox* icon =
                    managerButton->createWidget<MyGUI::ImageBox>(
                        "ImageBox",
                        MyGUI::IntCoord(
                            inset, inset,
                            managerCoord.width - 2 * inset,
                            managerCoord.height - 2 * inset),
                        MyGUI::Align::Stretch,
                        "KJM_HudManagerIcon");
                if (icon != NULL)
                {
                    icon->setNeedMouseFocus(false);
                    icon->setNeedToolTip(false);
                    if (g_hudManagerIconAvailable)
                    {
                        icon->setImageTexture(HUD_MANAGER_BUTTON_ICON_TEXTURE);
                        const MyGUI::IntSize iconSize = icon->getImageSize();
                        iconReady = iconSize.width > 0 && iconSize.height > 0;
                    }
                    if (!iconReady)
                    {
                        icon->setVisible(false);
                    }
                }
            }
            catch (...)
            {
                iconReady = false;
            }
            if (iconReady)
            {
                managerButton->setCaption("");
            }
            *buttonOut = managerButton;
            return true;
        }
        catch (...)
        {
            TryDestroyHudButtonWidgetGuarded(managerButton);
            if (jobsCoordChanged)
            {
                TryRestoreHudButtonIfStillSplit(
                    context.jobsButton, jobsCoord, original);
            }
            return false;
        }
    }

    bool EnsureHudManagerButton(
        const HudButtonContext& context)
    {
        if (context.jobsButton == NULL || context.ordersRoot == NULL ||
            context.nativeRoot == NULL)
        {
            return false;
        }

        // Never trust a cached plug-in child across a MyGUI rebuild.  The
        // authoritative parent/name lookup must succeed before any child
        // geometry or destruction operation is attempted.
        MyGUI::Button* liveManagerButton = NULL;
        bool liveLookupFaulted = false;
        bool liveManagerButtonFound =
            TryReadLiveHudManagerButtonGuarded(
                context.ordersRoot, &liveManagerButton,
                &liveLookupFaulted);
        if (liveLookupFaulted)
        {
            return false;
        }
        const bool sameContext = SameHudButtonContext(context);
        if (!sameContext && liveManagerButtonFound)
        {
            // A prior plug-in child may survive a partial root rebuild even
            // though its native control is no longer current.  It is a live,
            // direct child discovered by name, so remove only that child
            // before creating the new binding.
            TryDestroyHudButtonWidgetGuarded(liveManagerButton);
            liveManagerButton = NULL;
            liveManagerButtonFound = false;
        }
        if (sameContext &&
            g_hudOriginalJobsCoordValid && !liveManagerButtonFound)
        {
            TryRestoreHudButtonIfStillSplit(
                context.jobsButton,
                g_hudSplitJobsCoord,
                g_hudOriginalJobsCoord);
            ResetHudManagerButtonBinding();
            return false;
        }
        if (liveManagerButtonFound)
        {
            g_hudManagerButton = liveManagerButton;
        }

        const MyGUI::IntCoord liveCoord = context.jobsButton->getCoord();
        MyGUI::IntCoord original = liveCoord;
        if (g_hudOriginalJobsCoordValid &&
            SameHudButtonCoord(liveCoord, g_hudSplitJobsCoord))
        {
            original = g_hudOriginalJobsCoord;
        }

        MyGUI::IntCoord jobsCoord;
        MyGUI::IntCoord managerCoord;
        if (!BuildHudButtonSplit(original, &jobsCoord, &managerCoord))
        {
            // If an attached control became too narrow after a resize, leave
            // the native JOBS row intact and remove only our sibling.
            if (SameHudButtonContext(context) &&
                g_hudOriginalJobsCoordValid)
            {
                TryRestoreHudButtonIfStillSplit(
                    context.jobsButton,
                    g_hudSplitJobsCoord,
                    g_hudOriginalJobsCoord);
                if (liveManagerButtonFound)
                {
                    TryDestroyHudButtonWidgetGuarded(liveManagerButton);
                }
            }
            ResetHudManagerButtonBinding();
            return false;
        }

        if (sameContext && liveManagerButtonFound &&
            g_hudOriginalJobsCoordValid)
        {
            // The native root is unchanged.  Reapply only geometry that may
            // have changed through a responsive layout update; never copy
            // the native enabled state into the sibling.
            PublishHudManagerButtonBinding(
                context, original, jobsCoord, liveManagerButton);
            context.jobsButton->setCoord(jobsCoord);
            liveManagerButton->setCoord(managerCoord);
            return true;
        }

        MyGUI::Button* managerButton = NULL;
        if (!CreateHudManagerButton(
                context, original, jobsCoord, managerCoord, &managerButton) ||
            managerButton == NULL)
        {
            ResetHudManagerButtonBinding();
            return false;
        }

        PublishHudManagerButtonBinding(
            context, original, jobsCoord, managerButton);
        return true;
    }

    __declspec(noinline) bool CallEnsureHudManagerButtonSafely(
        const HudButtonContext& context)
    {
        try
        {
            return EnsureHudManagerButton(context);
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TryEnsureHudManagerButtonGuarded(
        const HudButtonContext& context)
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallEnsureHudManagerButtonSafely(context);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || !result)
        {
            // A C++ exception or guarded pointer fault can occur after the
            // native rectangle was split.  Cleanup rechecks the live
            // context/geometry before restoring anything.
            RestoreHudManagerButton();
            return false;
        }
        return result;
    }

    void TickHudManagerButton()
    {
        if (!g_enabled)
        {
            return;
        }

        HudButtonContext context;
        if (!TryReadHudButtonContext(&context))
        {
            // A missing root is normal during load/restart.  Do not call
            // into stale MyGUI pointers while Kenshi rebuilds its mainbar.
            // Keep the old snapshot only until a new live context appears;
            // reset cleanup will discard it if the root never returns.
            return;
        }

        if ((g_hudJobsButton != NULL || g_hudManagerButton != NULL) &&
            !SameHudButtonContext(context))
        {
            // The old root/control is no longer authoritative.  Its children
            // are owned by Kenshi and may already be destroyed; abandon the
            // old binding and reacquire from the new live OrdersPanel.
            ResetHudManagerButtonBinding();
        }
        TryEnsureHudManagerButtonGuarded(context);
    }

    void RestoreHudManagerButton()
    {
        if (g_hudJobsButton == NULL || !g_hudOriginalJobsCoordValid)
        {
            ResetHudManagerButtonBinding();
            return;
        }

        HudButtonContext current;
        if (!TryReadHudButtonContext(&current) ||
            !SameHudButtonContext(current))
        {
            // Do not touch an old native pointer after a GUI rebuild.
            ResetHudManagerButtonBinding();
            return;
        }

        MyGUI::Button* liveManagerButton = NULL;
        bool liveLookupFaulted = false;
        const bool liveManagerButtonFound =
            TryReadLiveHudManagerButtonGuarded(
                current.ordersRoot, &liveManagerButton,
                &liveLookupFaulted);
        if (liveLookupFaulted)
        {
            ResetHudManagerButtonBinding();
            return;
        }
        TryRestoreHudButtonIfStillSplit(
            current.jobsButton,
            g_hudSplitJobsCoord,
            g_hudOriginalJobsCoord);
        if (liveManagerButtonFound)
        {
            TryDestroyHudButtonWidgetGuarded(liveManagerButton);
        }
        ResetHudManagerButtonBinding();
    }

    void OnHudManagerButtonClicked(MyGUI::Widget*)
    {
        // MyGUI callbacks run inside Kenshi's input/update work.  Only mark
        // intent here; PlayerInterface::updateUT consumes it after vanilla
        // processing through the same ToggleJobWindow path as Ctrl+J.
        g_hudManagerButtonRequest = true;
    }

    void ProcessHudManagerButtonRequest()
    {
        if (!g_hudManagerButtonRequest)
        {
            return;
        }
        g_hudManagerButtonRequest = false;
        ToggleJobWindow();
    }

#endif
