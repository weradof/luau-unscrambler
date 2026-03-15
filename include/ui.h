#pragma once
#include "luau_unscrambler.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace ui {

struct AppState {
    // ── Editor buffers ────────────────────────────────────────────────────────
    std::string inputCode;
    std::string outputCode;

    // ── Options ───────────────────────────────────────────────────────────────
    luau::UnscrambleOptions options;

    // ── Last result ───────────────────────────────────────────────────────────
    luau::UnscrambleResult lastResult;
    bool hasResult = false;

    // ── Processing state ──────────────────────────────────────────────────────
    std::atomic<bool> processing{false};
    std::mutex resultMutex;
    std::string statusMessage;
    float processingProgress = 0.f;

    // ── UI state ──────────────────────────────────────────────────────────────
    bool showSettings      = false;
    bool showDiagnostics   = false;
    bool showAbout         = false;
    bool darkMode          = true;
    float fontSize         = 14.f;
    int   selectedDiag     = -1;
};

void setupTheme(bool dark);
void renderMainWindow(AppState& state);
void renderMenuBar(AppState& state);
void renderEditorPanels(AppState& state);
void renderStatusBar(AppState& state);
void renderSettingsModal(AppState& state);
void renderDiagnosticsPanel(AppState& state);
void renderAboutModal(AppState& state);
void renderStatsBar(AppState& state);

void runUnscramble(AppState& state);

} // namespace ui
