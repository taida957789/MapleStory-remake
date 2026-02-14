/**
 * @file map_viewer.cpp
 * @brief Standalone map viewer for visual map verification
 *
 * Usage:
 *   ./MapViewer --wz-path /path/to/Data --map 100000000
 *
 * Controls:
 *   Arrow keys / WASD  - Move camera
 *   Shift              - Fast camera
 *   F                  - Toggle free camera (no view range clipping)
 *   R                  - Reload current map
 *   ESC                - Exit
 */

#include "app/Application.h"
#include "stage/MapViewStage.h"
#include "util/Logger.h"

#include <SDL3/SDL.h>
#include <cstdlib>
#include <cstring>
#include <string>

static void PrintUsage(const char* prog)
{
    SDL_Log("Usage: %s --wz-path <path> --map <mapId>\n", prog);
    SDL_Log("  --wz-path, -w  Path to WZ Data directory\n");
    SDL_Log("  --map, -m      Map ID to load (e.g. 100000000 = Henesys)\n");
}

auto main(int argc, char* argv[]) -> int
{
    ms::Logger::Initialize();

    // Parse --map argument (before passing to Application)
    std::int32_t mapId = -1;
    for (int i = 1; i < argc; ++i)
    {
        if ((std::strcmp(argv[i], "--map") == 0 || std::strcmp(argv[i], "-m") == 0)
            && i + 1 < argc)
        {
            mapId = std::stoi(argv[i + 1]);
        }
    }

    if (mapId < 0)
    {
        PrintUsage(argv[0]);
        ms::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS))
    {
        LOG_CRITICAL("SDL initialization failed: {}", SDL_GetError());
        ms::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    auto& app = ms::Application::GetInstance();

    // Initialize all subsystems (creates Logo stage as default)
    if (!app.Initialize(argc, argv))
    {
        LOG_CRITICAL("Application initialization failed");
        SDL_Quit();
        ms::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    // Replace initial stage with MapViewStage
    auto mapStage = std::make_shared<ms::MapViewStage>(mapId);
    app.SetStage(mapStage);

    app.Run();
    app.Shutdown();

    SDL_Quit();
    ms::Logger::Shutdown();
    return EXIT_SUCCESS;
}
