// SPDX-License-Identifier: GPL-3.0-only
// Session-long registration for the temporary station-category icon set.

#ifndef KENSHI_JOB_MANAGEMENT_STATION_ASSETS_INL
#define KENSHI_JOB_MANAGEMENT_STATION_ASSETS_INL

    bool g_stationIconLocationRegistered = false;

    const char* const STATION_ICON_RESOURCE_GROUP =
        "KenshiJobManagementStationIcons";
    const char* const STATION_HUD_ICON_FILE = "kjm-hud-icon.png";

    const char* const STATION_ICON_FILES[] =
    {
        "station-crafting.png",
        "station-refining.png",
        "station-farming.png",
        "station-mining.png",
        "station-research.png",
        "station-training.png",
        "station-storage.png",
        "station-defense.png",
        "station-other.png"
    };

    const char* const STATION_VISUAL_ICON_FILES[STATION_VISUAL_COUNT] =
    {
        NULL,
        "station-copper-ore.png",
        "station-iron-ore.png",
        "station-copper-plates.png",
        "station-iron-plates.png",
        "station-steel-bars.png",
        "station-copper-alloy-plates.png",
        "station-electronics.png",
        "station-crossbow.png",
        "station-skeleton-limbs.png"
    };

    bool g_stationVisualIconAvailable[STATION_VISUAL_COUNT] = { false };
    bool g_hudManagerIconAvailable = false;

    const char* GetStationCategoryIconResource(StationCategory category)
    {
        if (!g_stationIconLocationRegistered)
        {
            return NULL;
        }
        switch (category)
        {
        case STATION_CRAFTING: return "station-crafting.png";
        case STATION_REFINING: return "station-refining.png";
        case STATION_FARMING: return "station-farming.png";
        case STATION_MINING: return "station-mining.png";
        case STATION_RESEARCH: return "station-research.png";
        case STATION_TRAINING: return "station-training.png";
        case STATION_STORAGE_HAULING: return "station-storage.png";
        case STATION_DEFENSE: return "station-defense.png";
        default: return "station-other.png";
        }
    }

    const char* GetStationVisualIconResource(
        StationCategory category,
        StationVisualSubtype subtype)
    {
        if (g_stationIconLocationRegistered &&
            subtype > STATION_VISUAL_DEFAULT &&
            subtype < STATION_VISUAL_COUNT &&
            g_stationVisualIconAvailable[static_cast<int>(subtype)])
        {
            return STATION_VISUAL_ICON_FILES[static_cast<int>(subtype)];
        }
        return GetStationCategoryIconResource(category);
    }

    bool EnsureStationIconResources()
    {
        if (g_stationIconLocationRegistered)
        {
            return true;
        }

        HMODULE module = NULL;
        char modulePath[MAX_PATH] = { 0 };
        if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(&g_stationIconLocationRegistered),
                &module) == FALSE ||
            module == NULL ||
            GetModuleFileNameA(module, modulePath, MAX_PATH) == 0)
        {
            ErrorLog("[KenshiJobManagement] Could not locate the station icon directory.");
            return false;
        }

        char* separator = std::strrchr(modulePath, '\\');
        if (separator == NULL)
        {
            ErrorLog("[KenshiJobManagement] Could not resolve the station icon path.");
            return false;
        }
        *(separator + 1) = '\0';
        std::string iconDirectory(modulePath);
        iconDirectory += "gui";

        try
        {
            Ogre::ResourceGroupManager* resources =
                Ogre::ResourceGroupManager::getSingletonPtr();
            Ogre::TextureManager* textures =
                Ogre::TextureManager::getSingletonPtr();
            if (resources == NULL || textures == NULL)
            {
                ErrorLog("[KenshiJobManagement] Ogre is not ready for station icons.");
                return false;
            }

            if (!resources->resourceGroupExists(STATION_ICON_RESOURCE_GROUP))
            {
                resources->createResourceGroup(STATION_ICON_RESOURCE_GROUP, true);
            }
            if (!resources->resourceLocationExists(
                    iconDirectory,
                    STATION_ICON_RESOURCE_GROUP))
            {
                resources->addResourceLocation(
                    iconDirectory,
                    "FileSystem",
                    STATION_ICON_RESOURCE_GROUP,
                    false,
                    true);
            }

            // These are raw PNGs, not Ogre resource scripts.  Loading the
            // nine exact files is both cheaper and safer than initialising a
            // resource group, which scans every file in the location.
            const size_t iconCount =
                sizeof(STATION_ICON_FILES) / sizeof(STATION_ICON_FILES[0]);
            for (size_t index = 0; index < iconCount; ++index)
            {
                Ogre::TexturePtr texture = textures->load(
                    STATION_ICON_FILES[index],
                    STATION_ICON_RESOURCE_GROUP);
                if (texture.isNull())
                {
                    ErrorLog("[KenshiJobManagement] A station icon could not be loaded.");
                    return false;
                }
            }

            // Variant icons are optional.  A missing new asset must not take
            // down the established category artwork.  Attempt each exact
            // texture directly: this resource group intentionally is not
            // initialised or indexed, so resourceExists() cannot reliably
            // discover files added through the raw FileSystem location.
            for (int subtype =
                    static_cast<int>(STATION_VISUAL_DEFAULT) + 1;
                 subtype < static_cast<int>(STATION_VISUAL_COUNT);
                 ++subtype)
            {
                const char* file = STATION_VISUAL_ICON_FILES[subtype];
                if (file == NULL)
                {
                    continue;
                }
                try
                {
                    Ogre::TexturePtr texture = textures->load(
                        file, STATION_ICON_RESOURCE_GROUP);
                    g_stationVisualIconAvailable[subtype] = !texture.isNull();
                }
                catch (...)
                {
                    g_stationVisualIconAvailable[subtype] = false;
                }
            }

            // The manager entry icon is optional.  A package without this
            // new file must still keep every mandatory station category
            // resource usable; HudButton.inl keeps its JM caption fallback.
            try
            {
                Ogre::TexturePtr texture = textures->load(
                    STATION_HUD_ICON_FILE, STATION_ICON_RESOURCE_GROUP);
                g_hudManagerIconAvailable = !texture.isNull();
            }
            catch (...)
            {
                g_hudManagerIconAvailable = false;
            }

            g_stationIconLocationRegistered = true;
            DebugLog("[KenshiJobManagement] Station category icons loaded.");
            return true;
        }
        catch (...)
        {
            ErrorLog("[KenshiJobManagement] Ogre rejected the station icon directory.");
            return false;
        }
    }

    __declspec(noinline) bool CallEnsureStationIconResourcesSafely()
    {
        try
        {
            return EnsureStationIconResources();
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool TryEnsureStationIconResourcesGuarded()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = CallEnsureStationIconResourcesSafely();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || !result)
        {
            ErrorLog("[KenshiJobManagement] Station icons failed safely; using text labels.");
            return false;
        }
        return true;
    }

    bool EnsureHudManagerIconResource()
    {
        if (!g_stationIconLocationRegistered)
        {
            return false;
        }
        if (g_hudManagerIconAvailable)
        {
            return true;
        }

        try
        {
            Ogre::TextureManager* textures =
                Ogre::TextureManager::getSingletonPtr();
            if (textures == NULL)
            {
                return false;
            }
            Ogre::TexturePtr texture = textures->load(
                STATION_HUD_ICON_FILE, STATION_ICON_RESOURCE_GROUP);
            g_hudManagerIconAvailable = !texture.isNull();
        }
        catch (...)
        {
            g_hudManagerIconAvailable = false;
        }
        return g_hudManagerIconAvailable;
    }

    __declspec(noinline) bool TryEnsureHudManagerIconResourceGuarded()
    {
        bool result = false;
        bool faulted = false;
        __try
        {
            result = EnsureHudManagerIconResource();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted)
        {
            g_hudManagerIconAvailable = false;
            return false;
        }
        return result;
    }

#endif
