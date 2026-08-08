// SPDX-License-Identifier: GPL-3.0-only
    void DestroyJobWindow()
    {
        ResetClearConfirmation();

        if (g_window != NULL)
        {
            MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
            if (gui != NULL)
            {
                try
                {
                    gui->destroyWidget(g_window);
                }
                catch (...)
                {
                    ErrorLog("[KenshiJobManagement] MyGUI threw while destroying the job window.");
                }
            }
        }

        g_window = NULL;
        g_characterText = NULL;
        g_statusText = NULL;
        g_jobList = NULL;
        g_jobsToggleButton = NULL;
        g_moveUpButton = NULL;
        g_moveDownButton = NULL;
        g_removeButton = NULL;
        g_clearButton = NULL;
        g_refreshButton = NULL;
        g_closeButton = NULL;
        g_rows.clear();
        ResetHandleIdentity(&g_displayedCharacter);
        g_refreshInProgress = false;
    }

    void OnCloseClicked(MyGUI::Widget*)
    {
        DestroyJobWindow();
    }

    void OnWindowButtonPressed(MyGUI::Window*, const std::string& name)
    {
        if (name == "close")
        {
            DestroyJobWindow();
        }
    }

    void CreateJobWindow()
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
        {
            ErrorLog("[KenshiJobManagement] MyGUI was unavailable when opening the job window.");
            return;
        }

        if (g_window != NULL)
        {
            DestroyJobWindow();
        }

        g_window = gui->createWidgetReal<MyGUI::Window>(
            "Kenshi_WindowCX",
            0.18f,
            0.10f,
            0.64f,
            0.76f,
            MyGUI::Align::Center,
            "Popup",
            "KenshiJobManagement_Window");
        g_window->setCaption("Kenshi Job Management 0.1.0-alpha");
        g_window->eventWindowButtonPressed += MyGUI::newDelegate(OnWindowButtonPressed);

        MyGUI::Widget* client = g_window->getClientWidget();

        g_characterText = client->createWidgetReal<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            0.03f,
            0.02f,
            0.94f,
            0.07f,
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KenshiJobManagement_Character");
        g_characterText->setTextAlign(MyGUI::Align::Center);
        g_characterText->setCaption("No player character selected");

        MyGUI::TextBox* helpText = client->createWidgetReal<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            0.03f,
            0.09f,
            0.94f,
            0.05f,
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KenshiJobManagement_Help");
        helpText->setTextAlign(MyGUI::Align::Center);
        helpText->setCaption("This edits Kenshi's vanilla permanent-job queue. Top row has highest job priority.");

        g_jobList = client->createWidgetReal<MyGUI::ListBox>(
            "Kenshi_ListBox",
            0.03f,
            0.15f,
            0.94f,
            0.51f,
            MyGUI::Align::Stretch,
            "KenshiJobManagement_JobList");
        g_jobList->setActivateOnClick(true);
        g_jobList->eventListChangePosition += MyGUI::newDelegate(OnJobListSelectionChanged);

        g_statusText = client->createWidgetReal<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            0.03f,
            0.68f,
            0.94f,
            0.06f,
            MyGUI::Align::Bottom | MyGUI::Align::HStretch,
            "KenshiJobManagement_Status");
        g_statusText->setTextAlign(MyGUI::Align::Center);
        g_statusText->setCaption("Select a job, then use the controls below.");

        g_jobsToggleButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.03f,
            0.76f,
            0.22f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Left,
            "KenshiJobManagement_JobsToggle");
        g_jobsToggleButton->setCaption("Jobs: unavailable");
        g_jobsToggleButton->eventMouseButtonClick += MyGUI::newDelegate(OnJobsToggleClicked);

        g_moveUpButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.27f,
            0.76f,
            0.14f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Left,
            "KenshiJobManagement_MoveUp");
        g_moveUpButton->setCaption("Move Up");
        g_moveUpButton->eventMouseButtonClick += MyGUI::newDelegate(OnMoveUpClicked);

        g_moveDownButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.43f,
            0.76f,
            0.14f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Left,
            "KenshiJobManagement_MoveDown");
        g_moveDownButton->setCaption("Move Down");
        g_moveDownButton->eventMouseButtonClick += MyGUI::newDelegate(OnMoveDownClicked);

        g_removeButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.59f,
            0.76f,
            0.17f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Left,
            "KenshiJobManagement_Remove");
        g_removeButton->setCaption("Remove");
        g_removeButton->eventMouseButtonClick += MyGUI::newDelegate(OnRemoveClicked);

        g_clearButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.78f,
            0.76f,
            0.19f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Right,
            "KenshiJobManagement_Clear");
        g_clearButton->setCaption("Clear All");
        g_clearButton->eventMouseButtonClick += MyGUI::newDelegate(OnClearClicked);

        g_refreshButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.03f,
            0.87f,
            0.22f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Left,
            "KenshiJobManagement_Refresh");
        g_refreshButton->setCaption("Refresh Queue");
        g_refreshButton->eventMouseButtonClick += MyGUI::newDelegate(OnRefreshClicked);

        g_closeButton = client->createWidgetReal<MyGUI::Button>(
            "Kenshi_Button1",
            0.78f,
            0.87f,
            0.19f,
            0.09f,
            MyGUI::Align::Bottom | MyGUI::Align::Right,
            "KenshiJobManagement_Close");
        g_closeButton->setCaption("Close");
        g_closeButton->eventMouseButtonClick += MyGUI::newDelegate(OnCloseClicked);

        g_lastRefreshTick = GetTickCount();
        RefreshJobWindow(true);
    }

    void ToggleJobWindow()
    {
        if (g_window != NULL)
        {
            DestroyJobWindow();
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

        const DWORD now = GetTickCount();
        if (g_clearArmed && HasElapsed(now, g_clearArmedTick, CLEAR_CONFIRMATION_MS))
        {
            ResetClearConfirmation();
            SetStatus("Clear-all confirmation expired. No jobs were removed.");
        }

        if (HasElapsed(now, g_lastRefreshTick, REFRESH_INTERVAL_MS))
        {
            g_lastRefreshTick = now;
            RefreshJobWindow(false);
        }
    }
