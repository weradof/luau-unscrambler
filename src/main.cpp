#include "ui.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
    // ── SDL init ──────────────────────────────────────────────────────────────
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    int windowW = 1280, windowH = 760;
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    SDL_Window* window = SDL_CreateWindow(
        "Luau Unscrambler",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowW, windowH,
        windowFlags
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        // Fallback to software renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // ── ImGui init ────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "luau_unscrambler.ini";

    // Fonts
    io.Fonts->AddFontFromMemoryCompressedTTF(
        // Use default ImGui font (Proggy Clean) as fallback
        ImGui::GetIO().Fonts->GetGlyphRangesDefault(), 14.f
    );
    // Just use the default font
    io.Fonts->AddFontDefault();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ── App state ─────────────────────────────────────────────────────────────
    ui::AppState state;
    state.darkMode = true;
    ui::setupTheme(true);

    state.statusMessage = "Ready. Paste decompiled Luau code and click Unscramble.";
    state.inputCode =
        "-- Example: paste your decompiled Luau here\n"
        "local l_0_0 = game:GetService(\"Players\")\n"
        "local v1 = l_0_0.LocalPlayer\n"
        "local upval0 = v1.Character\n"
        "local l_1_2 = \"Hel\" .. \"lo \" .. \"World\"\n"
        "local var_3 = 0x1F\n"
        "local a0 = function(b0, b1)\n"
        "  if true then\n"
        "  local v2 = b0 + b1\n"
        "  return v2\n"
        "  end\n"
        "end\n"
        "local v3 = game:GetService(\"ReplicatedStorage\")\n"
        "local upval1 = v3:WaitForChild(\"RemoteEvent\")\n"
        "upval1:FireServer(l_1_2, var_3)\n";

    // ── Main loop ─────────────────────────────────────────────────────────────
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        // Start frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render UI
        ui::renderMainWindow(state);

        // Render
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
