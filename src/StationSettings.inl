// SPDX-License-Identifier: GPL-3.0-only
// Global, immediately persisted filters shared by the Stations tab.

#ifndef KENSHI_JOB_MANAGEMENT_STATION_SETTINGS_INL
#define KENSHI_JOB_MANAGEMENT_STATION_SETTINGS_INL

    struct StationCategoryDefinition
    {
        StationCategory category;
        const char* name;
        bool defaultEnabled;
    };

    const StationCategoryDefinition STATION_CATEGORY_DEFINITIONS[] =
    {
        { STATION_CRAFTING, "Crafting", true },
        { STATION_REFINING, "Refining", true },
        { STATION_FARMING, "Farming", true },
        { STATION_MINING, "Mining", true },
        { STATION_RESEARCH, "Research", true },
        { STATION_TRAINING, "Training", false },
        { STATION_STORAGE_HAULING, "Storage / Hauling", false },
        { STATION_DEFENSE, "Defense", false },
        { STATION_OTHER, "Other / Unclassified", true }
    };

    const size_t STATION_CATEGORY_DEFINITION_COUNT =
        sizeof(STATION_CATEGORY_DEFINITIONS) /
        sizeof(STATION_CATEGORY_DEFINITIONS[0]);

    struct StationCategoryOptionButton
    {
        MyGUI::Button* button;
        StationCategory category;

        StationCategoryOptionButton() : button(NULL), category(STATION_OTHER) {}
    };

    bool g_stationCategoryEnabled[STATION_OTHER + 1];
    bool g_stationCategoryCollapsed[STATION_OTHER + 1];
    bool g_stationSettingsLoaded = false;
    std::vector<StationCategoryOptionButton> g_stationOptionButtons;

    bool SaveStationCategorySettings();

    bool IsStationCategoryEnabled(StationCategory category)
    {
        return category >= STATION_CRAFTING && category <= STATION_OTHER &&
            g_stationCategoryEnabled[static_cast<int>(category)];
    }

    bool IsStationCategoryCollapsed(StationCategory category)
    {
        return category >= STATION_CRAFTING && category <= STATION_OTHER &&
            g_stationCategoryCollapsed[static_cast<int>(category)];
    }

    bool SetStationCategoryCollapsed(
        StationCategory category,
        bool collapsed)
    {
        if (category < STATION_CRAFTING || category > STATION_OTHER)
        {
            return false;
        }
        g_stationCategoryCollapsed[static_cast<int>(category)] = collapsed;
        return SaveStationCategorySettings();
    }

    void ResetDefaultStationCategories()
    {
        for (int index = 0; index <= STATION_OTHER; ++index)
        {
            g_stationCategoryEnabled[index] = false;
        }
        for (size_t index = 0;
             index < STATION_CATEGORY_DEFINITION_COUNT; ++index)
        {
            const StationCategoryDefinition& definition =
                STATION_CATEGORY_DEFINITIONS[index];
            g_stationCategoryEnabled[definition.category] =
                definition.defaultEnabled;
        }
    }

    void LoadStationCategorySettings()
    {
        if (g_stationSettingsLoaded)
        {
            return;
        }
        ResetDefaultStationCategories();
        for (int category = 0; category <= STATION_OTHER; ++category)
        {
            g_stationCategoryCollapsed[category] = false;
        }
        EnsureSettingsPath();
        for (size_t index = 0;
             index < STATION_CATEGORY_DEFINITION_COUNT; ++index)
        {
            const StationCategoryDefinition& definition =
                STATION_CATEGORY_DEFINITIONS[index];
            char key[32] = { 0 };
            std::sprintf(key, "Category_%d", static_cast<int>(definition.category));
            g_stationCategoryEnabled[definition.category] =
                GetPrivateProfileIntA(
                    "Stations", key, definition.defaultEnabled ? 1 : 0,
                    g_settingsPath.c_str()) != 0;
            std::sprintf(
                key, "Collapsed_%d", static_cast<int>(definition.category));
            g_stationCategoryCollapsed[definition.category] =
                GetPrivateProfileIntA(
                    "Stations", key, 0,
                    g_settingsPath.c_str()) != 0;
        }
        g_stationSettingsLoaded = true;
    }

    bool SaveStationCategorySettings()
    {
        bool success = EnsureSettingsPath();
        for (size_t index = 0;
             index < STATION_CATEGORY_DEFINITION_COUNT; ++index)
        {
            const StationCategoryDefinition& definition =
                STATION_CATEGORY_DEFINITIONS[index];
            char key[32] = { 0 };
            std::sprintf(key, "Category_%d", static_cast<int>(definition.category));
            if (WritePrivateProfileStringA(
                    "Stations", key,
                    g_stationCategoryEnabled[definition.category] ? "1" : "0",
                    g_settingsPath.c_str()) == FALSE)
            {
                success = false;
            }
            std::sprintf(
                key, "Collapsed_%d", static_cast<int>(definition.category));
            if (WritePrivateProfileStringA(
                    "Stations", key,
                    g_stationCategoryCollapsed[definition.category] ? "1" : "0",
                    g_settingsPath.c_str()) == FALSE)
            {
                success = false;
            }
        }
        return success;
    }

#endif
