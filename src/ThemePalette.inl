// SPDX-License-Identifier: GPL-3.0-only
// Runtime palette helpers for the optional Dark UI text mode.
//
// This file is included after RuntimeAccess.inl, while the translation unit
// is still inside its anonymous namespace.  It intentionally owns no widget
// pointers.  Callers can apply the palette to an open widget tree whenever
// the setting changes or a view is rebuilt.

#ifndef KENSHI_JOB_MANAGEMENT_THEME_PALETTE_INL
#define KENSHI_JOB_MANAGEMENT_THEME_PALETTE_INL

    // The current manager uses a dark brown canvas and warm, light labels.
    // Vanilla Kenshi's standard panels are a light, warm parchment/tan.  Keep
    // these values in one place so a live palette refresh cannot leave the
    // background and text in mismatched modes.
    const char* const THEME_SETTINGS_SECTION = "Appearance";
    const char* const THEME_DARK_UI_KEY = "DarkUIFriendlyFontColors";
    const char* const THEME_TAG_BACKGROUND = "KJM_ThemeBackground";
    const char* const THEME_TAG_STATION_TINT = "KJM_ThemeStationTint";
    const char* const THEME_TINT_STATION_NORMAL = "normal";
    const char* const THEME_TINT_STATION_BLOCKING = "blocking";
    const char* const THEME_TAG_TEXT = "KJM_ThemeText";
    const char* const THEME_TEXT_STANDARD = "standard";
    const char* const THEME_TEXT_MUTED = "muted";
    const char* const THEME_TEXT_BUTTON = "button";
    const char* const THEME_TEXT_ACCENT = "accent";
    const char* const THEME_TEXT_WARNING = "warning";
    const char* const THEME_TEXT_SUCCESS = "success";

    // Dark UI mode is opt-in.  A missing key therefore selects the vanilla
    // light canvas and dark text, which is readable in an unmodified game.
    bool g_darkUiFriendlyFontColors = false;
    bool g_themePaletteLoaded = false;
    unsigned int g_themePaletteRevision = 0;

    struct ThemePalette
    {
        MyGUI::Colour background;
        MyGUI::Colour standardText;
        MyGUI::Colour mutedText;
        MyGUI::Colour buttonText;
        MyGUI::Colour accentText;
        MyGUI::Colour warningText;
        MyGUI::Colour successText;

        ThemePalette() :
            background(MyGUI::Colour::White),
            standardText(MyGUI::Colour::Black),
            mutedText(MyGUI::Colour::Black),
            buttonText(MyGUI::Colour::Black),
            accentText(MyGUI::Colour::Black),
            warningText(MyGUI::Colour::Black),
            successText(MyGUI::Colour::Black)
        {
        }
    };

    ThemePalette g_themePalette;

    ThemePalette BuildThemePalette(bool darkUi)
    {
        ThemePalette palette;
        if (darkUi)
        {
            // Existing KJM dark treatment.  These are the light colours used
            // by the current job cards, headers, and action buttons.
            palette.background = MyGUI::Colour(0.105f, 0.085f, 0.065f);
            palette.standardText = MyGUI::Colour(1.0f, 1.0f, 1.0f);
            palette.mutedText = MyGUI::Colour(0.92f, 0.92f, 0.88f);
            palette.buttonText = MyGUI::Colour(1.0f, 0.96f, 0.84f);
            palette.accentText = MyGUI::Colour(1.0f, 0.84f, 0.45f);
            palette.warningText = MyGUI::Colour(1.0f, 0.38f, 0.27f);
            palette.successText = MyGUI::Colour(0.68f, 1.0f, 0.70f);
        }
        else
        {
            // Warm Kenshi-style light panel.  The slight red/yellow bias keeps
            // the panel close to vanilla's parchment UI instead of using a
            // cold pure white surface.  Text stays well above WCAG's medium
            // contrast threshold against this background.
            // Vanilla's main warm panel is approximately #AFA68B, and its
            // standard text is approximately #140806.
            palette.background = MyGUI::Colour(
                0.686275f, 0.650980f, 0.545098f);
            palette.standardText = MyGUI::Colour(
                0.078431f, 0.031373f, 0.023529f);
            palette.mutedText = MyGUI::Colour(0.24f, 0.20f, 0.16f);
            // Native Kenshi button skins remain dark atlas textures. Keep
            // their captions light in both modes; standard TextBox content
            // uses the dark vanilla colour on the themed light surfaces.
            palette.buttonText = MyGUI::Colour(1.0f, 0.96f, 0.84f);
            palette.accentText = MyGUI::Colour(0.32f, 0.20f, 0.025f);
            palette.warningText = MyGUI::Colour(0.55f, 0.075f, 0.04f);
            palette.successText = MyGUI::Colour(0.08f, 0.34f, 0.12f);
        }
        return palette;
    }

    void RebuildThemePalette()
    {
        g_themePalette = BuildThemePalette(g_darkUiFriendlyFontColors);
        ++g_themePaletteRevision;
        if (g_themePaletteRevision == 0)
        {
            // Keep zero reserved for an unknown/unobserved revision if the
            // process ever survives enough live refreshes to wrap.
            g_themePaletteRevision = 1;
        }
    }

    void LoadThemePaletteSettings()
    {
        if (g_themePaletteLoaded)
        {
            return;
        }

        // RuntimeAccess supplies EnsureSettingsPath and g_settingsPath.
        // Treat a failed directory lookup as a normal first-run default; the
        // in-memory setting still works for this manager session.
        EnsureSettingsPath();
        g_darkUiFriendlyFontColors =
            GetPrivateProfileIntA(
                THEME_SETTINGS_SECTION,
                THEME_DARK_UI_KEY,
                0,
                g_settingsPath.c_str()) != 0;
        g_themePaletteLoaded = true;
        RebuildThemePalette();
    }

    bool SaveThemePaletteSettings()
    {
        if (!EnsureSettingsPath())
        {
            return false;
        }
        return WritePrivateProfileStringA(
            THEME_SETTINGS_SECTION,
            THEME_DARK_UI_KEY,
            g_darkUiFriendlyFontColors ? "1" : "0",
            g_settingsPath.c_str()) != FALSE;
    }

    const ThemePalette& GetThemePalette()
    {
        LoadThemePaletteSettings();
        return g_themePalette;
    }

    unsigned int GetThemePaletteRevision()
    {
        LoadThemePaletteSettings();
        return g_themePaletteRevision;
    }

    bool IsDarkUIFriendlyFontColorsEnabled()
    {
        LoadThemePaletteSettings();
        return g_darkUiFriendlyFontColors;
    }

    // Returns true when the setting changed.  The palette is rebuilt before
    // returning, so callers can immediately repaint an open manager without
    // waiting for its next periodic snapshot refresh.
    bool SetDarkUIFriendlyFontColors(bool enabled)
    {
        LoadThemePaletteSettings();
        if (g_darkUiFriendlyFontColors == enabled)
        {
            return true;
        }
        g_darkUiFriendlyFontColors = enabled;
        RebuildThemePalette();
        return SaveThemePaletteSettings();
    }

    bool ToggleDarkUIFriendlyFontColors()
    {
        return SetDarkUIFriendlyFontColors(
            !IsDarkUIFriendlyFontColorsEnabled());
    }

    MyGUI::Colour ThemeBackgroundColour()
    {
        return GetThemePalette().background;
    }

    MyGUI::Colour ThemeStandardTextColour()
    {
        return GetThemePalette().standardText;
    }

    MyGUI::Colour ThemeMutedTextColour()
    {
        return GetThemePalette().mutedText;
    }

    MyGUI::Colour ThemeButtonTextColour()
    {
        return GetThemePalette().buttonText;
    }

    MyGUI::Colour ThemeButtonAccentTextColour()
    {
        // Selected/partial squad captions are rendered on native dark
        // buttons, so use the existing light accent in either palette.
        return MyGUI::Colour(1.0f, 0.84f, 0.45f);
    }

    MyGUI::Colour ThemeButtonSuccessTextColour()
    {
        // Fully selected squad captions need the same dark-surface contrast.
        return MyGUI::Colour(0.68f, 1.0f, 0.70f);
    }

    MyGUI::Colour ThemeAccentTextColour()
    {
        return GetThemePalette().accentText;
    }

    MyGUI::Colour ThemeWarningTextColour()
    {
        return GetThemePalette().warningText;
    }

    MyGUI::Colour ThemeSuccessTextColour()
    {
        return GetThemePalette().successText;
    }

    void ApplyThemeBackground(MyGUI::Widget* widget)
    {
        if (widget == NULL)
        {
            return;
        }
        widget->setColour(GetThemePalette().background);
    }

    void ApplyThemeStationCardTint(
        MyGUI::Widget* widget,
        bool blocking)
    {
        if (widget == NULL)
        {
            return;
        }
        if (IsDarkUIFriendlyFontColorsEnabled())
        {
            widget->setColour(blocking ?
                MyGUI::Colour(0.38f, 0.07f, 0.05f) :
                MyGUI::Colour(0.09f, 0.07f, 0.05f));
            widget->setAlpha(0.55f);
        }
        else
        {
            // Keep the status tint visible without putting near-black paint
            // behind the vanilla dark labels. The warning tint remains a
            // warm red, but its low alpha preserves the light card surface.
            widget->setColour(blocking ?
                MyGUI::Colour(0.82f, 0.30f, 0.22f) :
                MyGUI::Colour(0.76f, 0.65f, 0.46f));
            widget->setAlpha(blocking ? 0.20f : 0.16f);
        }
    }

    void ApplyThemeStandardText(MyGUI::TextBox* text)
    {
        if (text == NULL)
        {
            return;
        }
        text->setTextColour(GetThemePalette().standardText);
    }

    void ApplyThemeMutedText(MyGUI::TextBox* text)
    {
        if (text == NULL)
        {
            return;
        }
        text->setTextColour(GetThemePalette().mutedText);
    }

    void ApplyThemeButtonText(MyGUI::Button* button)
    {
        if (button == NULL)
        {
            return;
        }
        button->setTextColour(GetThemePalette().buttonText);
    }

    void ApplyThemeAccentText(MyGUI::TextBox* text)
    {
        if (text != NULL)
        {
            text->setTextColour(GetThemePalette().accentText);
        }
    }

    void ApplyThemeWarningText(MyGUI::TextBox* text)
    {
        if (text != NULL)
        {
            text->setTextColour(GetThemePalette().warningText);
        }
    }

    void ApplyThemeSuccessText(MyGUI::TextBox* text)
    {
        if (text != NULL)
        {
            text->setTextColour(GetThemePalette().successText);
        }
    }

    void TagThemeBackground(MyGUI::Widget* widget)
    {
        if (widget == NULL)
        {
            return;
        }
        widget->setUserString(THEME_TAG_BACKGROUND, "1");
        ApplyThemeBackground(widget);
    }

    void TagThemeStationCardTint(
        MyGUI::Widget* widget,
        bool blocking)
    {
        if (widget == NULL)
        {
            return;
        }
        widget->setUserString(
            THEME_TAG_STATION_TINT,
            blocking ? THEME_TINT_STATION_BLOCKING :
                THEME_TINT_STATION_NORMAL);
        ApplyThemeStationCardTint(widget, blocking);
    }

    // A solid theme surface sits below normal-depth child labels. It never
    // receives mouse focus, so it cannot hide captions or steal callbacks.
    void TagThemeSurface(MyGUI::Widget* widget)
    {
        if (widget == NULL)
        {
            return;
        }
        TagThemeBackground(widget);
        widget->setDepth(100);
        widget->setAlpha(1.0f);
        widget->setNeedMouseFocus(false);
    }

    void TagThemeStandardText(MyGUI::TextBox* text)
    {
        if (text == NULL)
        {
            return;
        }
        text->setUserString(THEME_TAG_TEXT, THEME_TEXT_STANDARD);
        ApplyThemeStandardText(text);
    }

    void TagThemeMutedText(MyGUI::TextBox* text)
    {
        if (text == NULL)
        {
            return;
        }
        text->setUserString(THEME_TAG_TEXT, THEME_TEXT_MUTED);
        ApplyThemeMutedText(text);
    }

    void TagThemeButtonText(MyGUI::Button* button)
    {
        if (button == NULL)
        {
            return;
        }
        button->setUserString(THEME_TAG_TEXT, THEME_TEXT_BUTTON);
        ApplyThemeButtonText(button);
    }

    void TagThemeAccentText(MyGUI::TextBox* text)
    {
        if (text != NULL)
        {
            text->setUserString(THEME_TAG_TEXT, THEME_TEXT_ACCENT);
            ApplyThemeAccentText(text);
        }
    }

    void TagThemeWarningText(MyGUI::TextBox* text)
    {
        if (text != NULL)
        {
            text->setUserString(THEME_TAG_TEXT, THEME_TEXT_WARNING);
            ApplyThemeWarningText(text);
        }
    }

    void TagThemeSuccessText(MyGUI::TextBox* text)
    {
        if (text != NULL)
        {
            text->setUserString(THEME_TAG_TEXT, THEME_TEXT_SUCCESS);
            ApplyThemeSuccessText(text);
        }
    }

    // Apply only explicit theme tags.  Warning, success, selection, and
    // station-status colours remain untouched unless a caller tags those
    // widgets deliberately with one of the roles above.
    void ApplyThemeToTaggedTree(MyGUI::Widget* root)
    {
        if (root == NULL)
        {
            return;
        }

        if (root->isUserString(THEME_TAG_BACKGROUND))
        {
            ApplyThemeBackground(root);
        }

        if (root->isUserString(THEME_TAG_STATION_TINT))
        {
            ApplyThemeStationCardTint(
                root,
                root->getUserString(THEME_TAG_STATION_TINT) ==
                    THEME_TINT_STATION_BLOCKING);
        }

        if (root->isUserString(THEME_TAG_TEXT))
        {
            const std::string& role = root->getUserString(THEME_TAG_TEXT);
            if (role == THEME_TEXT_BUTTON && root->isType<MyGUI::Button>())
            {
                ApplyThemeButtonText(root->castType<MyGUI::Button>(false));
            }
            else if (role == THEME_TEXT_MUTED &&
                     root->isType<MyGUI::TextBox>())
            {
                ApplyThemeMutedText(root->castType<MyGUI::TextBox>(false));
            }
            else if (role == THEME_TEXT_STANDARD &&
                     root->isType<MyGUI::TextBox>())
            {
                ApplyThemeStandardText(root->castType<MyGUI::TextBox>(false));
            }
            else if (role == THEME_TEXT_ACCENT &&
                     root->isType<MyGUI::TextBox>())
            {
                ApplyThemeAccentText(root->castType<MyGUI::TextBox>(false));
            }
            else if (role == THEME_TEXT_WARNING &&
                     root->isType<MyGUI::TextBox>())
            {
                ApplyThemeWarningText(root->castType<MyGUI::TextBox>(false));
            }
            else if (role == THEME_TEXT_SUCCESS &&
                     root->isType<MyGUI::TextBox>())
            {
                ApplyThemeSuccessText(root->castType<MyGUI::TextBox>(false));
            }
        }

        const size_t childCount = root->getChildCount();
        for (size_t index = 0; index < childCount; ++index)
        {
            ApplyThemeToTaggedTree(root->getChildAt(index));
        }
    }

    // Convenience for the live-refresh path: callers can set a tag on each
    // root once, then call this after changing the option or rebuilding a
    // tab.  The revision is monotonic for cheap caller-side change checks.
    bool ThemePaletteChangedSince(unsigned int revision)
    {
        return GetThemePaletteRevision() != revision;
    }

#endif
