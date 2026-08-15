// SPDX-License-Identifier: GPL-3.0-only
    void PlayerInterfaceUpdateHook(PlayerInterface* player)
    {
        if (g_enabled && g_window != NULL)
        {
            // MyGUI and Kenshi receive the wheel independently. The modal
            // consumes clicks, but Kenshi's camera still reads this separate
            // accumulator. Clear it before vanilla updateUT can zoom while
            // preserving MyGUI's own list scrolling.
            __try
            {
                if (key != NULL)
                {
                    key->mWheel = 0;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        if (g_playerInterfaceUpdateOriginal != NULL)
        {
            g_playerInterfaceUpdateOriginal(player);
        }

        if (!g_enabled)
        {
            return;
        }

        g_playerInterface = player;
#ifdef KJM_SCANNER_PROBE
        TickOwnershipProbe(player);
#endif
        TickHotkey();
        TickWindow();
    }

    void PlayerInterfaceClearAndResetHook(PlayerInterface* player)
    {
#ifdef KJM_SCANNER_PROBE
        OwnershipProbeOnWorldReset();
#endif
        DestroyJobWindow(true);
        g_playerInterface = NULL;
        g_hotkeyWasDown = false;
        g_lastResetTick = GetTickCount();

        if (g_playerInterfaceClearAndResetOriginal != NULL)
        {
            g_playerInterfaceClearAndResetOriginal(player);
        }
    }

    void PlayerInterfaceCycleSquadHook(PlayerInterface* player)
    {
        if (g_enabled && g_window != NULL &&
            (GetAsyncKeyState(VK_TAB) & 0x8000) != 0)
        {
            // The manager edge-detects TAB and calls the original function
            // directly. Suppress Kenshi's copy of the same key event so one
            // press cannot advance twice. Modals intentionally do not run
            // the manager's edge-detected path, so TAB remains blocked there.
            return;
        }
        if (g_playerInterfaceCycleSquadOriginal != NULL)
        {
            g_playerInterfaceCycleSquadOriginal(player);
        }
    }
