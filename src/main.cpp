/**
 * @file main.cpp
 * @brief MapleStory Client Recreation
 *
 * Based on reverse engineering of Korean MapleStoryT v1029
 * Tech Stack: SDL3 + C++ (similar to sdlMS project)
 */

#include "app/Application.h"
#include "util/Logger.h"
#include <SDL3/SDL.h>
#include <cstdlib>

auto main(int argc, char* argv[]) -> int
{
    // Initialize logger early
    ms::Logger::Initialize();

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS))
    {
        LOG_CRITICAL("SDL initialization failed: {}", SDL_GetError());
        ms::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    // Create and run the application
    // This mirrors the original WinMain flow:
    // 1. Create CWvsContext singleton
    // 2. Create CWvsApp
    // 3. CWvsApp::SetUp() - Initialize all subsystems
    // 4. CWvsApp::Run() - Main game loop
    // 5. Cleanup

    auto& app = ms::Application::GetInstance();

    if (!app.Initialize(argc, argv))
    {
        LOG_CRITICAL("Application initialization failed");
        SDL_Quit();
        ms::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    app.Run();
    app.Shutdown();

    SDL_Quit();
    ms::Logger::Shutdown();
    return EXIT_SUCCESS;
}
