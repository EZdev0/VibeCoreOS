#!/bin/bash
# ============================================================
#  VibeCore OS — Auth Helper (Desktop-Only GUI Popup)
#
#  Shows a graphical password dialog (like Windows UAC).
#  ONLY desktop GUI methods — NEVER terminal-based sudo.
#  Prevents terminal corruption and broken chat sessions.
#
#  Desktop methods (in priority order):
#    1. zenity --password  (GTK dialog, works on RPi/PiXEL)
#    2. pkexec             (PolicyKit native desktop dialog)
#
#  If no GUI is available → FAILS gracefully (no terminal fallback!)
#
#  Auth is cached for 5 minutes — one prompt per target.
#
#  Usage:  ./scripts/auth-helper.sh <command...>
# ============================================================

set -e

COMMAND="$@"
if [ -z "$COMMAND" ]; then
    echo "Usage: auth-helper.sh <command...>"
    exit 1
fi

# ── Auth cache (session-scoped, fixed per-user) ──────────────
CACHE_FILE="/tmp/vibecore_auth_${USER}"
AUTH_VALID_SECONDS=300

check_cache() {
    if [ -f "$CACHE_FILE" ]; then
        local then=$(cat "$CACHE_FILE")
        local now=$(date +%s)
        if [ $((now - then)) -lt $AUTH_VALID_SECONDS ]; then
            return 0
        fi
        rm -f "$CACHE_FILE"
    fi
    return 1
}

# ── Method 1: zenity GUI password dialog ────────────────────
try_zenity_auth() {
    if ! command -v zenity >/dev/null 2>&1; then
        return 1
    fi
    if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
        return 1
    fi

    local pass
    pass=$(zenity --password \
        --title="VibeCore OS — Authentication Required" \
        --text="VibeCore needs administrative privileges.\n\nPlease enter your password:" \
        --width=420 \
        --ok-label="Allow" \
        --cancel-label="Deny" \
        2>/dev/null) || return 1

    if [ -z "$pass" ]; then
        return 1
    fi

    echo "$pass" | sudo -S -v 2>/dev/null || return 1
    return 0
}

# ── Method 2: pkexec (PolicyKit native dialog) ─────────────
try_pkexec_auth() {
    if ! command -v pkexec >/dev/null 2>&1; then
        return 1
    fi
    if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
        return 1
    fi

    pkexec /bin/true 2>/dev/null || return 1
    return 0
}

# ── Main auth flow (GUI ONLY — no terminal fallback!) ────────
authenticate() {
    # 1. Already cached?
    if check_cache; then
        return 0
    fi

    # 2. zenity popup (most reliable on RPi/PiXEL)
    if try_zenity_auth; then
        date +%s > "$CACHE_FILE"
        return 0
    fi

    # 3. pkexec (native PolicyKit dialog)
    if try_pkexec_auth; then
        date +%s > "$CACHE_FILE"
        return 0
    fi

    # NO terminal fallback — fail gracefully
    echo ""
    echo "[VibeCore] ╔══════════════════════════════════════════╗"
    echo "[VibeCore] ║  Authentication failed!                 ║"
    echo "[VibeCore] ║  No desktop GUI available.              ║"
    echo "[VibeCore] ║  This command requires a graphical      ║"
    echo "[VibeCore] ║  session with zenity or pkexec.         ║"
    echo "[VibeCore] ║  Install: sudo apt install zenity       ║"
    echo "[VibeCore] ║  Then run in a desktop environment.     ║"
    echo "[VibeCore] ╚══════════════════════════════════════════╝"
    echo ""
    return 1
}

# ── Execute ─────────────────────────────────────────────────
if authenticate; then
    echo "[VibeCore] Auth OK. Executing..."
    exec sudo "$@"
else
    echo "[VibeCore] Auth denied — cannot continue."
    exit 1
fi
