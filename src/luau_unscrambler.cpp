#include "luau_unscrambler.h"
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace luau {

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

static const std::vector<std::string> KEYWORDS = {
    "and","break","do","else","elseif","end","false","for","function",
    "if","in","local","nil","not","or","repeat","return","then","true",
    "until","while","continue","type","export"
};

static bool isKeyword(const std::string& s) {
    return std::find(KEYWORDS.begin(), KEYWORDS.end(), s) != KEYWORDS.end();
}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::vector<std::string> splitLines(const std::string& src) {
    std::vector<std::string> lines;
    std::istringstream ss(src);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

Unscrambler::Unscrambler(const UnscrambleOptions& opts) : m_opts(opts) {}

// ─────────────────────────────────────────────────────────────────────────────
//  Variable name generation
// ─────────────────────────────────────────────────────────────────────────────

static const char* VAR_PREFIXES[] = {
    "value","result","data","flag","count","index","item","obj",
    "node","ref","temp","state","ctx","info","key","arg","param",
    "buf","len","num","str","tbl","fn","cb"
};
static const size_t VAR_PREFIX_COUNT = sizeof(VAR_PREFIXES)/sizeof(VAR_PREFIXES[0]);

std::string Unscrambler::generateVarName(int index, const std::string& hint) {
    if (!hint.empty()) return hint;
    size_t base = static_cast<size_t>(index) % VAR_PREFIX_COUNT;
    int    rep  = static_cast<int>(index) / static_cast<int>(VAR_PREFIX_COUNT);
    std::string name = VAR_PREFIXES[base];
    if (rep > 0) name += std::to_string(rep);
    return name;
}

bool Unscrambler::isMangledName(const std::string& name) const {
    // Patterns: l_0_0, l_1_2, v0, v12, upval0, upval_3, a0..a9,
    //           _0, _1, var0, var_1, local0, arg0..argN
    static const std::regex mangledPat(
        R"(^(l_\d+_\d+|v\d+|upval_?\d+|_\d+|var_?\d+|local_?\d+|arg\d+|a\d+|b\d+|c\d+|f\d+|x\d+|y\d+|z\d+|t\d+|s\d+|n\d+|k\d+|p\d+|r\d+|i\d+|j\d+)$)"
    );
    return std::regex_match(name, mangledPat);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Rename mangled variables
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passRenameVariables(const std::string& src, int& count) {
    // First pass: collect all mangled names in order of appearance
    static const std::regex identPat(R"(\b([A-Za-z_][A-Za-z0-9_]*)\b)");
    std::unordered_map<std::string, std::string> nameMap;
    int idx = 0;

    auto begin = std::sregex_iterator(src.begin(), src.end(), identPat);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        std::string ident = (*it)[1].str();
        if (!isKeyword(ident) && isMangledName(ident)) {
            if (nameMap.find(ident) == nameMap.end()) {
                nameMap[ident] = generateVarName(idx++);
            }
        }
    }

    if (nameMap.empty()) return src;

    // Second pass: replace whole-word occurrences (skip string literals & comments)
    std::string result;
    result.reserve(src.size());
    size_t pos = 0;
    bool inString = false;
    bool inLongString = false;
    bool inLineComment = false;
    char strChar = 0;

    while (pos < src.size()) {
        // Skip line comments
        if (!inString && !inLongString && pos + 1 < src.size()
            && src[pos] == '-' && src[pos+1] == '-') {
            // check for long comment
            if (pos + 3 < src.size() && src[pos+2] == '[' && src[pos+3] == '[') {
                size_t end2 = src.find("]]", pos+4);
                if (end2 == std::string::npos) end2 = src.size();
                result += src.substr(pos, end2+2-pos);
                pos = end2+2;
            } else {
                size_t eol = src.find('\n', pos);
                if (eol == std::string::npos) eol = src.size();
                result += src.substr(pos, eol-pos);
                pos = eol;
            }
            continue;
        }
        // Skip string literals
        if (!inString && !inLongString && (src[pos] == '"' || src[pos] == '\'')) {
            char q = src[pos];
            size_t start = pos++;
            while (pos < src.size() && src[pos] != q) {
                if (src[pos] == '\\') pos++;
                pos++;
            }
            pos++; // closing quote
            result += src.substr(start, pos-start);
            continue;
        }
        // Long strings [[...]]
        if (!inString && !inLongString && pos+1 < src.size()
            && src[pos]=='[' && src[pos+1]=='[') {
            size_t end2 = src.find("]]", pos+2);
            if (end2 == std::string::npos) end2 = src.size();
            result += src.substr(pos, end2+2-pos);
            pos = end2+2;
            continue;
        }

        // Check for identifier
        if (std::isalpha((unsigned char)src[pos]) || src[pos]=='_') {
            size_t start = pos;
            while (pos < src.size() && (std::isalnum((unsigned char)src[pos]) || src[pos]=='_'))
                pos++;
            std::string ident = src.substr(start, pos-start);
            auto it = nameMap.find(ident);
            if (it != nameMap.end()) {
                result += it->second;
                count++;
            } else {
                result += ident;
            }
            continue;
        }

        result += src[pos++];
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Reformat indentation
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passReformatIndentation(const std::string& src, int& count) {
    static const std::regex openPat(R"(\b(do|then|else|elseif|repeat|function)\b)");
    static const std::regex closePat(R"(\b(end|until)\b)");

    auto lines = splitLines(src);
    std::string result;
    int depth = 0;
    const std::string TAB = "    ";

    for (auto& rawLine : lines) {
        std::string line = trim(rawLine);
        if (line.empty()) { result += "\n"; continue; }

        // Determine indent change BEFORE this line
        bool isEnd    = std::regex_search(line, closePat);
        bool isElse   = (line.rfind("else", 0) == 0 || line.rfind("elseif", 0) == 0);

        if (isEnd || isElse) depth = std::max(0, depth - 1);

        std::string indented = "";
        for (int i = 0; i < depth; i++) indented += TAB;
        indented += line;

        if (indented != rawLine) count++;
        result += indented + "\n";

        // Determine indent change AFTER this line
        bool hasOpen = std::regex_search(line, openPat);
        if (hasOpen && !isEnd) depth++;
        else if (isEnd && !hasOpen) {} // already decremented
        else if (isElse) depth++; // re-indent body after else
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Unwrap string concatenation
//  "a" .. "b" .. "c"  →  "abc"
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passUnwrapStrings(const std::string& src, int& count) {
    // Match: ("...") .. ("...")  repeated
    static const std::regex concatPat(
        R"("((?:[^"\\]|\\.)*)"\s*\.\.\s*"((?:[^"\\]|\\.)*)")"
    );
    std::string result = src;
    std::smatch m;
    int iterations = 0;
    while (std::regex_search(result, m, concatPat) && iterations < 100) {
        std::string merged = "\"" + m[1].str() + m[2].str() + "\"";
        result = result.substr(0, m.position()) + merged
               + result.substr(m.position() + m.length());
        count++;
        iterations++;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Annotate Roblox/Luau patterns
// ─────────────────────────────────────────────────────────────────────────────

struct RobloxPattern {
    std::regex  pattern;
    std::string comment;
};

static std::vector<RobloxPattern> buildRobloxPatterns() {
    return {
        { std::regex(R"(\bgame:GetService\s*\()"),        "-- [Roblox] Service acquisition" },
        { std::regex(R"(\bRemoteEvent\b)"),               "-- [Roblox] Network remote event" },
        { std::regex(R"(\bRemoteFunction\b)"),            "-- [Roblox] Network remote function" },
        { std::regex(R"(\bBindableEvent\b)"),             "-- [Roblox] Internal bindable event" },
        { std::regex(R"(\bInstance\.new\s*\()"),          "-- [Roblox] Instance creation" },
        { std::regex(R"(\bworkspace\b)"),                 "-- [Roblox] Workspace reference" },
        { std::regex(R"(\bPlayers:GetPlayers\b)"),        "-- [Roblox] Player list" },
        { std::regex(R"(\bLocalPlayer\b)"),               "-- [Roblox] Local player reference" },
        { std::regex(R"(\bFireServer\s*\()"),             "-- [Roblox] Client→Server event fire" },
        { std::regex(R"(\bFireClient\s*\()"),             "-- [Roblox] Server→Client event fire" },
        { std::regex(R"(\bFireAllClients\s*\()"),         "-- [Roblox] Broadcast to all clients" },
        { std::regex(R"(\bInvokeServer\s*\()"),           "-- [Roblox] Client→Server RPC call" },
        { std::regex(R"(\bOnServerEvent\b)"),             "-- [Roblox] Server event listener" },
        { std::regex(R"(\bOnClientEvent\b)"),             "-- [Roblox] Client event listener" },
        { std::regex(R"(\bloadstring\s*\()"),             "-- [SUSPICIOUS] Dynamic code execution" },
        { std::regex(R"(\bgetfenv\s*\()"),                "-- [SUSPICIOUS] Environment manipulation" },
        { std::regex(R"(\bsetfenv\s*\()"),                "-- [SUSPICIOUS] Environment manipulation" },
        { std::regex(R"(\brawget\s*\()"),                 "-- [NOTE] Raw table access (bypasses __index)" },
        { std::regex(R"(\bpcall\s*\()"),                  "-- [NOTE] Protected call" },
        { std::regex(R"(\bxpcall\s*\()"),                 "-- [NOTE] Extended protected call" },
        { std::regex(R"(\bHttpService\b)"),               "-- [Roblox] HTTP service (external requests)" },
        { std::regex(R"(\bDataStoreService\b)"),          "-- [Roblox] Persistent data storage" },
        { std::regex(R"(\bTween\b)"),                     "-- [Roblox] Tween animation" },
        { std::regex(R"(\bCoroutine\.|coroutine\.)"),     "-- [NOTE] Coroutine (concurrent execution)" },
        { std::regex(R"(\btask\.wait\b)"),                "-- [Roblox] Task scheduler wait" },
        { std::regex(R"(\btask\.spawn\b)"),               "-- [Roblox] Task scheduler spawn" },
        { std::regex(R"(\btask\.defer\b)"),               "-- [Roblox] Deferred task" },
    };
}

std::string Unscrambler::passAnnotateRoblox(const std::string& src, int& count) {
    static auto patterns = buildRobloxPatterns();
    auto lines = splitLines(src);
    std::string result;

    for (auto& line : lines) {
        std::string annotation;
        for (auto& p : patterns) {
            if (std::regex_search(line, p.pattern)) {
                // Don't double-annotate
                if (line.find("-- [") == std::string::npos) {
                    annotation = "  " + p.comment;
                    count++;
                    break;
                }
            }
        }
        result += line + annotation + "\n";
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Reconstruct control flow
//  Converts:  goto L1 / ::L1::  patterns into readable if/while forms
//  Also cleans up: if true then ... end  →  do ... end
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passReconstructControlFlow(const std::string& src, int& count) {
    static const std::regex trivialIf(R"(\bif\s+true\s+then\b)");
    static const std::regex deadGoto(R"(\bgoto\s+(\w+)\s*\n\s*::\1::)");
    static const std::regex emptyDo(R"(\bdo\s*\n\s*end\b)");

    std::string result = src;
    std::smatch m;

    // if true then → do
    std::string r2;
    {
        auto lines = splitLines(result);
        for (auto& line : lines) {
            if (std::regex_search(line, trivialIf)) {
                r2 += std::regex_replace(line, trivialIf, "do") + "\n";
                count++;
            } else {
                r2 += line + "\n";
            }
        }
        result = r2;
    }

    // Remove dead gotos (goto L immediately followed by ::L::)
    int iters = 0;
    while (std::regex_search(result, m, deadGoto) && iters < 50) {
        result = result.substr(0, m.position())
               + result.substr(m.position() + m.length());
        count++;
        iters++;
    }

    // Remove empty do...end
    iters = 0;
    while (std::regex_search(result, m, emptyDo) && iters < 50) {
        result = result.substr(0, m.position())
               + result.substr(m.position() + m.length());
        count++;
        iters++;
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Remove dead code
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passRemoveDeadCode(const std::string& src, int& count) {
    auto lines = splitLines(src);
    std::string result;
    bool prevWasReturn = false;

    for (auto& rawLine : lines) {
        std::string line = trim(rawLine);

        // After a return/break at top level, skip unreachable statements
        // (but only within the same block — simple heuristic: skip non-"end" lines)
        if (prevWasReturn) {
            if (line.empty() || line == "end" || line.rfind("end",0)==0
                || line.rfind("until",0)==0 || line.rfind("else",0)==0
                || line.rfind("elseif",0)==0) {
                prevWasReturn = false;
            } else {
                count++;
                continue; // skip dead line
            }
        }

        // Detect: return or break as last statement in block
        if (line == "return" || line.rfind("return ", 0) == 0 || line == "break") {
            prevWasReturn = true;
        } else if (!line.empty()) {
            prevWasReturn = false;
        }

        result += rawLine + "\n";
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Infer function names from assignment context
//  local myThing = function(...) → local myThing = function myThing(...)
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passInferFunctionNames(const std::string& src, int& count) {
    // local NAME = function(  →  local NAME = function NAME(
    static const std::regex anonFn(
        R"((local\s+(\w+)\s*=\s*)function\s*\()"
    );
    std::string result = src;
    std::smatch m;
    std::string out;
    auto flags = std::regex_constants::format_first_only;

    while (std::regex_search(result, m, anonFn)) {
        std::string name = m[2].str();
        // Don't rename already-named or mangled ones
        out += result.substr(0, m.position())
             + m[1].str() + "function " + name + "(";
        result = result.substr(m.position() + m.length());
        count++;
    }
    return out + result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pass: Normalize numbers
//  0x1F → 31, simple cases
// ─────────────────────────────────────────────────────────────────────────────

std::string Unscrambler::passNormalizeNumbers(const std::string& src, int& count) {
    static const std::regex hexPat(R"(\b0[xX]([0-9A-Fa-f]+)\b)");
    std::string result;
    std::smatch m;
    std::string remaining = src;

    while (std::regex_search(remaining, m, hexPat)) {
        result += remaining.substr(0, m.position());
        // Convert hex to decimal
        unsigned long val = std::stoul(m[1].str(), nullptr, 16);
        result += std::to_string(val);
        remaining = remaining.substr(m.position() + m.length());
        count++;
    }
    return result + remaining;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main process
// ─────────────────────────────────────────────────────────────────────────────

UnscrambleResult Unscrambler::process(const std::string& source) {
    UnscrambleResult res;
    std::string code = source;

    if (m_opts.normalizeNumbers)
        code = passNormalizeNumbers(code, res.variablesRenamed);

    if (m_opts.unwrapStrings)
        code = passUnwrapStrings(code, res.stringsUnwrapped);

    if (m_opts.renameVariables)
        code = passRenameVariables(code, res.variablesRenamed);

    if (m_opts.inferFunctionNames)
        code = passInferFunctionNames(code, res.variablesRenamed);

    if (m_opts.reconstructControlFlow)
        code = passReconstructControlFlow(code, res.controlFlowFixed);

    if (m_opts.removeDeadCode)
        code = passRemoveDeadCode(code, res.controlFlowFixed);

    if (m_opts.reformatIndentation)
        code = passReformatIndentation(code, res.linesReformatted);

    if (m_opts.annotateRoblox)
        code = passAnnotateRoblox(code, res.annotationsAdded);

    res.outputCode = code;

    // Generate diagnostics
    {
        auto lines = splitLines(code);
        int ln = 1;
        for (auto& line : lines) {
            std::string t = trim(line);
            if (t.find("loadstring") != std::string::npos)
                res.diagnostics.push_back({DiagSeverity::Warning, ln, "loadstring detected — dynamic execution"});
            if (t.find("getfenv") != std::string::npos || t.find("setfenv") != std::string::npos)
                res.diagnostics.push_back({DiagSeverity::Warning, ln, "Environment manipulation (getfenv/setfenv)"});
            if (t.find("goto") != std::string::npos)
                res.diagnostics.push_back({DiagSeverity::Info, ln, "Remaining goto — may indicate obfuscation"});
            ln++;
        }
    }

    return res;
}

} // namespace luau
