# Luau Unscrambler — Get Your Executable

## The easiest way: GitHub Actions (free, automatic)

This repo builds **Windows `.exe`**, **Linux binary**, and **macOS `.app`** automatically
using GitHub's free build servers. You never need to install a compiler.

---

## Step-by-step: Get a working .exe in 5 minutes

### 1. Create a free GitHub account
Go to https://github.com and sign up (free).

### 2. Create a new repository
- Click the **+** button → "New repository"
- Name it `luau-unscrambler`
- Set it to **Public** (required for free Actions minutes)
- Click "Create repository"

### 3. Upload these files
Drag and drop everything from this folder into your new repo:
```
.github/
  workflows/
    build.yml        ← the build instructions
src/
  main.cpp
  luau_unscrambler.cpp
  ui.cpp
include/
  luau_unscrambler.h
  ui.h
CMakeLists.txt
```

### 4. Wait ~3 minutes
GitHub will automatically:
- Build a `.exe` for Windows
- Build a binary for Linux  
- Build a `.app` for macOS

### 5. Download your files
- Click the **Actions** tab on your repo
- Click the latest green workflow run
- Scroll down to **Artifacts** — download for your platform

OR once the build finishes, it also creates a **Release** automatically.
Click **Releases** on the right sidebar → download from there.

---

## That's it!

| Platform | What you get | How to run |
|----------|-------------|------------|
| Windows | `luau-unscrambler-windows.zip` | Extract → double-click `.exe` |
| Linux   | `luau-unscrambler-linux` | `chmod +x luau-unscrambler && ./luau-unscrambler` |
| macOS   | `LuauUnscrambler-macOS.zip` | Extract → double-click `.app` |

---

## What the app does

Paste decompiled/obfuscated Luau (Roblox) code on the left →
click **Unscramble** → get readable code on the right.

Transformations applied:
- Renames `l_0_0`, `v1`, `upval3` → readable variable names
- Fixes indentation
- Merges split strings: `"He".."llo"` → `"Hello"`  
- Annotates Roblox API calls (RemoteEvent, FireServer, GetService...)
- Removes dead code, cleans up goto patterns
- Infers function names from assignment context
- Converts hex numbers: `0x1F` → `31`
