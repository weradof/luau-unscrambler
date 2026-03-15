#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace luau {

// ─── Token types ──────────────────────────────────────────────────────────────
enum class TokenKind {
    Unknown,
    Whitespace,
    Newline,
    Comment,
    String,
    Number,
    Keyword,
    Identifier,
    Operator,
    Punctuation,
    EOF_
};

struct Token {
    TokenKind kind;
    std::string text;
    int line   = 0;
    int column = 0;
};

// ─── Unscramble options ───────────────────────────────────────────────────────
struct UnscrambleOptions {
    bool renameVariables      = true;   // l_0_0, v1, upval3 → readable names
    bool reformatIndentation  = true;   // fix indentation
    bool unwrapStrings        = true;   // "a".."b".."c" → "abc"
    bool annotateRoblox       = true;   // mark game:GetService, RemoteEvents, etc.
    bool reconstructControlFlow = true; // clean up goto/label patterns
    bool removeDeadCode       = true;   // remove obvious no-ops
    bool inferFunctionNames   = true;   // try to name anonymous functions from context
    bool normalizeNumbers     = true;   // 0x1F → 31, 1e3 → 1000 where clear
};

// ─── Diagnostic (warning/info shown to user) ─────────────────────────────────
enum class DiagSeverity { Info, Warning, Error };
struct Diagnostic {
    DiagSeverity severity;
    int line;
    std::string message;
};

// ─── Result of unscramble pass ───────────────────────────────────────────────
struct UnscrambleResult {
    std::string outputCode;
    std::vector<Diagnostic> diagnostics;
    int variablesRenamed     = 0;
    int stringsUnwrapped     = 0;
    int linesReformatted     = 0;
    int annotationsAdded     = 0;
    int controlFlowFixed     = 0;
};

// ─── Main API ────────────────────────────────────────────────────────────────
class Unscrambler {
public:
    explicit Unscrambler(const UnscrambleOptions& opts = {});

    UnscrambleResult process(const std::string& source);

private:
    UnscrambleOptions m_opts;

    // Lexer
    std::vector<Token> tokenize(const std::string& src);

    // Passes
    std::string passRenameVariables(const std::string& src, int& count);
    std::string passReformatIndentation(const std::string& src, int& count);
    std::string passUnwrapStrings(const std::string& src, int& count);
    std::string passAnnotateRoblox(const std::string& src, int& count);
    std::string passReconstructControlFlow(const std::string& src, int& count);
    std::string passRemoveDeadCode(const std::string& src, int& count);
    std::string passInferFunctionNames(const std::string& src, int& count);
    std::string passNormalizeNumbers(const std::string& src, int& count);

    // Helpers
    std::string generateVarName(int index, const std::string& hint = "");
    bool isMangledName(const std::string& name) const;
    std::string indent(const std::string& src);
};

} // namespace luau
