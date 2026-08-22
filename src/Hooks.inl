// SPDX-License-Identifier: GPL-3.0-only
    void PlayerInterfaceUpdateHook(PlayerInterface* player)
    {
        if (g_enabled && g_window != NULL)
        {
            // MyGUI and Kenshi receive wheel/modifier input independently.
            // The modal consumes clicks, but Kenshi's camera still reads its
            // wheel accumulator and mouse-rotate command. Left Control is a
            // default mouse_rotate binding, so leaving rotate armed pins the
            // rendered cursor while MyGUI continues to move it logically.
            // Suppress only the native camera inputs before vanilla updateUT;
            // MyGUI keeps its own Ctrl state for multi-selection.
            __try
            {
                if (key != NULL)
                {
                    key->mWheel = 0;
                    key->rotate = false;
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
        // Reacquire the native mainbar after vanilla has rebuilt/updated it.
        // The button callback only sets a request flag, so opening the
        // manager is also deferred until this post-vanilla point.
        TickHudManagerButton();
        ProcessHudManagerButtonRequest();
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
        RestoreHudManagerButton();
        // A world reset is the explicit lifecycle boundary that bypasses the
        // steady-state HUD validation throttle. Ordinary binding failures keep
        // their timestamp so a persistent missing widget cannot become an
        // every-frame probe.
        g_hudLastValidationTick = 0;
        g_hudManagerButtonRequest = false;
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
            (g_modal.kind != MODAL_NONE || IsStationDetailOpen()))
        {
            // These manager-owned modal surfaces block native squad changes.
            // The ordinary manager does not intercept TAB; Kenshi remains the
            // sole owner of its normal cycle timing and key-repeat behavior.
            return;
        }
        if (g_playerInterfaceCycleSquadOriginal != NULL)
        {
            g_playerInterfaceCycleSquadOriginal(player);
            if (g_enabled && g_window != NULL)
            {
                // Defer all MyGUI and snapshot work until the original
                // PlayerInterface update has returned to our update hook.
                g_squadCycleObserved = true;
            }
        }
    }
