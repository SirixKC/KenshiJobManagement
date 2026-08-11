// SPDX-License-Identifier: GPL-3.0-only
    void PlayerInterfaceUpdateHook(PlayerInterface* player)
    {
        if (g_playerInterfaceUpdateOriginal != NULL)
        {
            g_playerInterfaceUpdateOriginal(player);
        }

        if (!g_enabled)
        {
            return;
        }

        g_playerInterface = player;
        TickHotkey();
        TickWindow();
    }

    void PlayerInterfaceClearAndResetHook(PlayerInterface* player)
    {
        DestroyJobWindow();
        g_playerInterface = NULL;
        g_hotkeyWasDown = false;
        g_lastResetTick = GetTickCount();

        if (g_playerInterfaceClearAndResetOriginal != NULL)
        {
            g_playerInterfaceClearAndResetOriginal(player);
        }
    }
