#include "ui.h"
#include "imgui.h"
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>

// ─── Editor buffer size ───────────────────────────────────────────────────────
static constexpr size_t EDITOR_BUF = 1 << 20; // 1 MB

namespace ui {

// ─── Color theme ─────────────────────────────────────────────────────────────

void setupTheme(bool dark) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 6.f;
    s.FrameRounding     = 4.f;
    s.PopupRounding     = 4.f;
    s.ScrollbarRounding = 4.f;
    s.GrabRounding      = 4.f;
    s.TabRounding       = 4.f;
    s.FramePadding      = {8, 5};
    s.ItemSpacing       = {8, 6};
    s.WindowPadding     = {12, 12};
    s.ScrollbarSize     = 12.f;

    if (dark) {
        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]          = {0.10f, 0.10f, 0.13f, 1.f};
        c[ImGuiCol_ChildBg]           = {0.12f, 0.12f, 0.16f, 1.f};
        c[ImGuiCol_PopupBg]           = {0.13f, 0.13f, 0.17f, 1.f};
        c[ImGuiCol_Border]            = {0.25f, 0.25f, 0.32f, 1.f};
        c[ImGuiCol_FrameBg]           = {0.08f, 0.08f, 0.11f, 1.f};
        c[ImGuiCol_FrameBgHovered]    = {0.16f, 0.16f, 0.22f, 1.f};
        c[ImGuiCol_FrameBgActive]     = {0.20f, 0.20f, 0.28f, 1.f};
        c[ImGuiCol_TitleBg]           = {0.08f, 0.08f, 0.10f, 1.f};
        c[ImGuiCol_TitleBgActive]     = {0.10f, 0.10f, 0.14f, 1.f};
        c[ImGuiCol_MenuBarBg]         = {0.09f, 0.09f, 0.12f, 1.f};
        c[ImGuiCol_ScrollbarBg]       = {0.08f, 0.08f, 0.10f, 1.f};
        c[ImGuiCol_ScrollbarGrab]     = {0.25f, 0.55f, 0.85f, 0.6f};
        c[ImGuiCol_ScrollbarGrabHovered]={0.35f, 0.65f, 0.95f, 0.8f};
        c[ImGuiCol_CheckMark]         = {0.40f, 0.80f, 1.00f, 1.f};
        c[ImGuiCol_SliderGrab]        = {0.35f, 0.65f, 0.95f, 1.f};
        c[ImGuiCol_Button]            = {0.18f, 0.38f, 0.70f, 1.f};
        c[ImGuiCol_ButtonHovered]     = {0.25f, 0.50f, 0.90f, 1.f};
        c[ImGuiCol_ButtonActive]      = {0.15f, 0.30f, 0.60f, 1.f};
        c[ImGuiCol_Header]            = {0.18f, 0.38f, 0.70f, 0.6f};
        c[ImGuiCol_HeaderHovered]     = {0.25f, 0.50f, 0.90f, 0.7f};
        c[ImGuiCol_HeaderActive]      = {0.20f, 0.40f, 0.80f, 1.f};
        c[ImGuiCol_Tab]               = {0.12f, 0.25f, 0.50f, 0.8f};
        c[ImGuiCol_TabHovered]        = {0.25f, 0.50f, 0.90f, 1.f};
        c[ImGuiCol_TabActive]         = {0.20f, 0.42f, 0.82f, 1.f};
        c[ImGuiCol_Separator]         = {0.25f, 0.25f, 0.35f, 1.f};
        c[ImGuiCol_Text]              = {0.90f, 0.92f, 0.96f, 1.f};
        c[ImGuiCol_TextDisabled]      = {0.45f, 0.45f, 0.55f, 1.f};
        c[ImGuiCol_ResizeGrip]        = {0.25f, 0.55f, 0.85f, 0.4f};
        c[ImGuiCol_ResizeGripHovered] = {0.35f, 0.65f, 0.95f, 0.7f};
        c[ImGuiCol_ResizeGripActive]  = {0.40f, 0.70f, 1.00f, 1.f};
    } else {
        ImGui::StyleColorsLight();
        s.Colors[ImGuiCol_WindowBg] = {0.94f, 0.94f, 0.96f, 1.f};
        s.Colors[ImGuiCol_ChildBg]  = {0.97f, 0.97f, 0.98f, 1.f};
        s.Colors[ImGuiCol_FrameBg]  = {1.f, 1.f, 1.f, 1.f};
    }
}

// ─── Menu Bar ────────────────────────────────────────────────────────────────

void renderMenuBar(AppState& state) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Clear Input", "Ctrl+L")) {
                state.inputCode.clear();
                state.outputCode.clear();
                state.hasResult = false;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Signal handled in main loop
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Copy Output")) {
                ImGui::SetClipboardText(state.outputCode.c_str());
            }
            if (ImGui::MenuItem("Paste to Input")) {
                const char* clip = ImGui::GetClipboardText();
                if (clip) state.inputCode = clip;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings",    nullptr, &state.showSettings);
            ImGui::MenuItem("Diagnostics", nullptr, &state.showDiagnostics);
            ImGui::Separator();
            if (ImGui::MenuItem("Dark Mode",  nullptr, nullptr, !state.darkMode)) {
                state.darkMode = true;  setupTheme(true);
            }
            if (ImGui::MenuItem("Light Mode", nullptr, nullptr,  state.darkMode)) {
                state.darkMode = false; setupTheme(false);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", nullptr, &state.showAbout);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

// ─── Stats bar (below editors) ───────────────────────────────────────────────

void renderStatsBar(AppState& state) {
    if (!state.hasResult) return;
    auto& r = state.lastResult;

    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
    ImGui::BeginChild("##stats", {0, 32}, false, ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("  Vars renamed: %d  |  Strings merged: %d  |  Lines reformatted: %d  |  Annotations: %d  |  Control flow fixes: %d  |  Diagnostics: %d",
        r.variablesRenamed, r.stringsUnwrapped, r.linesReformatted,
        r.annotationsAdded, r.controlFlowFixed, (int)r.diagnostics.size());

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Main editor panels ──────────────────────────────────────────────────────

void renderEditorPanels(AppState& state) {
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y - 44; // leave room for button + stats
    float panelW = (w - 12) / 2.f;

    // ── Input panel ────────────────────────────────────────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        ImGui::BeginChild("##input_panel", {panelW, h}, true);

        ImGui::TextColored({0.45f, 0.75f, 1.f, 1.f}, "INPUT  —  Decompiled / Obfuscated Luau");
        ImGui::Separator();

        // Resizable text buffer
        static std::string inputBuf(EDITOR_BUF, '\0');
        // Sync from state
        if (state.inputCode.size() < EDITOR_BUF - 1) {
            std::copy(state.inputCode.begin(), state.inputCode.end(), inputBuf.begin());
            inputBuf[state.inputCode.size()] = '\0';
        }

        ImVec2 editorSize = {ImGui::GetContentRegionAvail().x,
                             ImGui::GetContentRegionAvail().y};
        if (ImGui::InputTextMultiline("##input", inputBuf.data(), EDITOR_BUF,
                                       editorSize,
                                       ImGuiInputTextFlags_AllowTabInput)) {
            state.inputCode = std::string(inputBuf.data());
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::SameLine(0, 12);

    // ── Output panel ───────────────────────────────────────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        ImGui::BeginChild("##output_panel", {panelW, h}, true);

        ImGui::TextColored({0.45f, 1.f, 0.70f, 1.f}, "OUTPUT  —  Cleaned Luau Code");
        ImGui::Separator();

        static std::string outputBuf(EDITOR_BUF, '\0');
        if (state.outputCode.size() < EDITOR_BUF - 1) {
            std::copy(state.outputCode.begin(), state.outputCode.end(), outputBuf.begin());
            outputBuf[state.outputCode.size()] = '\0';
        }

        ImVec2 editorSize = {ImGui::GetContentRegionAvail().x,
                             ImGui::GetContentRegionAvail().y};
        ImGui::InputTextMultiline("##output", outputBuf.data(), EDITOR_BUF,
                                   editorSize,
                                   ImGuiInputTextFlags_ReadOnly);

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // ── Action button row ──────────────────────────────────────────────────────
    ImGui::Separator();
    float btnW = 180.f;
    float btnH = 32.f;
    float centerX = (w - btnW * 3 - 16) / 2.f;
    ImGui::SetCursorPosX(centerX);

    bool busy = state.processing.load();

    if (busy) {
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.3f, 0.3f, 0.3f, 1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.3f, 0.3f, 0.3f, 1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.3f, 0.3f, 0.3f, 1.f});
        ImGui::Button("Processing...", {btnW, btnH});
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f, 0.55f, 0.30f, 1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.20f, 0.70f, 0.40f, 1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.10f, 0.40f, 0.22f, 1.f});
        if (ImGui::Button("  Unscramble  ", {btnW, btnH})) {
            runUnscramble(state);
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine(0, 8);
    if (ImGui::Button("Copy Output", {btnW, btnH})) {
        ImGui::SetClipboardText(state.outputCode.c_str());
    }

    ImGui::SameLine(0, 8);
    if (ImGui::Button("Clear", {btnW, btnH})) {
        state.inputCode.clear();
        state.outputCode.clear();
        state.hasResult = false;
        state.statusMessage = "Cleared.";
    }
}

// ─── Status bar ──────────────────────────────────────────────────────────────

void renderStatusBar(AppState& state) {
    float statusH = 24.f;
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    ImGui::SetNextWindowPos({0, displaySize.y - statusH});
    ImGui::SetNextWindowSize({displaySize.x, statusH});
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8, 4});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);

    ImGui::Begin("##statusbar", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoNav  |
        ImGuiWindowFlags_NoInputs     | ImGuiWindowFlags_NoSavedSettings);

    if (state.processing.load()) {
        ImGui::TextColored({0.8f, 0.8f, 0.3f, 1.f}, "Processing...");
    } else {
        ImGui::Text("%s", state.statusMessage.c_str());
    }

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200);
    ImGui::TextDisabled("Luau Unscrambler v1.0  |  SDL2 + ImGui");

    ImGui::End();
    ImGui::PopStyleVar(2);
}

// ─── Settings modal ──────────────────────────────────────────────────────────

void renderSettingsModal(AppState& state) {
    if (!state.showSettings) return;
    ImGui::SetNextWindowSize({420, 370}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({100, 80}, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Settings", &state.showSettings, ImGuiWindowFlags_NoCollapse)) {
        ImGui::SeparatorText("Transformation Passes");

        auto& o = state.options;
        ImGui::Checkbox("Rename mangled variables",       &o.renameVariables);
        ImGui::SameLine(); ImGui::TextDisabled("(l_0_0, v1, upval3 → readable names)");

        ImGui::Checkbox("Reformat indentation",           &o.reformatIndentation);
        ImGui::SameLine(); ImGui::TextDisabled("(fix alignment, tabs)");

        ImGui::Checkbox("Unwrap string concatenation",    &o.unwrapStrings);
        ImGui::SameLine(); ImGui::TextDisabled("(\"a\"..\"b\" → \"ab\")");

        ImGui::Checkbox("Annotate Roblox patterns",       &o.annotateRoblox);
        ImGui::SameLine(); ImGui::TextDisabled("(RemoteEvent, GetService, etc.)");

        ImGui::Checkbox("Reconstruct control flow",       &o.reconstructControlFlow);
        ImGui::SameLine(); ImGui::TextDisabled("(remove dead gotos, simplify ifs)");

        ImGui::Checkbox("Remove dead code",               &o.removeDeadCode);
        ImGui::SameLine(); ImGui::TextDisabled("(unreachable code after return)");

        ImGui::Checkbox("Infer function names",           &o.inferFunctionNames);
        ImGui::SameLine(); ImGui::TextDisabled("(local fn = function → named)");

        ImGui::Checkbox("Normalize numbers",              &o.normalizeNumbers);
        ImGui::SameLine(); ImGui::TextDisabled("(0x1F → 31)");

        ImGui::Separator();
        ImGui::SeparatorText("Display");
        if (ImGui::SliderFloat("Font size", &state.fontSize, 10.f, 24.f, "%.0f px")) {
            ImGui::GetIO().FontGlobalScale = state.fontSize / 14.f;
        }

        if (ImGui::Button("Close", {100, 30})) state.showSettings = false;
    }
    ImGui::End();
}

// ─── Diagnostics panel ───────────────────────────────────────────────────────

void renderDiagnosticsPanel(AppState& state) {
    if (!state.showDiagnostics) return;
    ImGui::SetNextWindowSize({520, 260}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({120, 460}, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Diagnostics", &state.showDiagnostics)) {
        if (!state.hasResult || state.lastResult.diagnostics.empty()) {
            ImGui::TextDisabled("No diagnostics. Run the unscrambler first.");
        } else {
            ImGui::Text("%d diagnostic(s)", (int)state.lastResult.diagnostics.size());
            ImGui::Separator();
            if (ImGui::BeginTable("##diags", 3,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                    {0, 200})) {
                ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 70);
                ImGui::TableSetupColumn("Line",  ImGuiTableColumnFlags_WidthFixed, 50);
                ImGui::TableSetupColumn("Message");
                ImGui::TableHeadersRow();

                for (auto& d : state.lastResult.diagnostics) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (d.severity == luau::DiagSeverity::Error)
                        ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "Error");
                    else if (d.severity == luau::DiagSeverity::Warning)
                        ImGui::TextColored({1.f, 0.85f, 0.1f, 1.f}, "Warning");
                    else
                        ImGui::TextColored({0.5f, 0.85f, 1.f, 1.f}, "Info");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", d.line);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(d.message.c_str());
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

// ─── About modal ─────────────────────────────────────────────────────────────

void renderAboutModal(AppState& state) {
    if (!state.showAbout) return;
    ImGui::SetNextWindowSize({420, 220}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        {ImGui::GetIO().DisplaySize.x/2 - 210,
         ImGui::GetIO().DisplaySize.y/2 - 110}, ImGuiCond_Always);

    if (ImGui::Begin("About Luau Unscrambler", &state.showAbout,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored({0.45f, 0.75f, 1.f, 1.f}, "Luau Unscrambler  v1.0.0");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Cross-platform GUI tool for cleaning up decompiled and obfuscated "
            "Luau (Roblox) code. Renames mangled variables, fixes indentation, "
            "merges split strings, annotates Roblox API calls, and reconstructs "
            "readable control flow.");
        ImGui::Spacing();
        ImGui::TextWrapped("Built with C++17 · Dear ImGui · SDL2");
        ImGui::Spacing();
        ImGui::TextDisabled("Supports Linux, Windows, macOS");
        ImGui::Spacing();
        if (ImGui::Button("Close", {100, 28})) state.showAbout = false;
    }
    ImGui::End();
}

// ─── Main window ─────────────────────────────────────────────────────────────

void renderMainWindow(AppState& state) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::Begin("Luau Unscrambler", nullptr,
        ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_MenuBar       |
        ImGuiWindowFlags_NoBringToDisplayFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings);

    renderMenuBar(state);
    renderEditorPanels(state);
    renderStatsBar(state);
    renderSettingsModal(state);
    renderDiagnosticsPanel(state);
    renderAboutModal(state);
    renderStatusBar(state);

    ImGui::End();
}

// ─── Run unscramble in background thread ─────────────────────────────────────

void runUnscramble(AppState& state) {
    if (state.processing.load()) return;
    if (state.inputCode.empty()) {
        state.statusMessage = "Paste some code first!";
        return;
    }

    state.processing.store(true);
    state.statusMessage = "Processing...";

    std::string inputCopy = state.inputCode;
    luau::UnscrambleOptions optsCopy = state.options;

    std::thread([&state, inputCopy, optsCopy]() {
        luau::Unscrambler unscrambler(optsCopy);
        auto result = unscrambler.process(inputCopy);

        {
            std::lock_guard<std::mutex> lock(state.resultMutex);
            state.outputCode  = result.outputCode;
            state.lastResult  = std::move(result);
            state.hasResult   = true;
        }

        state.statusMessage = "Done! " +
            std::to_string(state.lastResult.variablesRenamed) + " vars renamed, " +
            std::to_string(state.lastResult.stringsUnwrapped) + " strings merged, " +
            std::to_string(state.lastResult.annotationsAdded) + " annotations added.";
        state.processing.store(false);
    }).detach();
}

} // namespace ui
