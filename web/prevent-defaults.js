// Prevent default browser behavior for ALL keys except essential browser shortcuts
// This allows games to use any keys (WASD, arrows, Q/E, etc.) without triggering
// browser actions like scrolling, page down, tab navigation, etc.

function preventGameKeyDefaults(e) {
    // Allow essential browser shortcuts to pass through
    if (e.key === 'F5' || e.key === 'F11' || e.key === 'F12') {
        // F5 = refresh, F11 = fullscreen, F12 = dev tools
        return;
    }

    if (e.ctrlKey || e.metaKey || e.altKey) {
        // Allow Ctrl/Cmd/Alt shortcuts (Ctrl+R refresh, Ctrl+C copy, etc.)
        return;
    }

    // Prevent default for everything else - this allows games to use any key
    // without triggering browser actions (arrows scrolling, space page down, etc.)
    e.preventDefault();
}

// Use capture phase to intercept events before they reach the canvas
window.addEventListener('keydown', preventGameKeyDefaults, true);
window.addEventListener('keyup', preventGameKeyDefaults, true);
window.addEventListener('keypress', preventGameKeyDefaults, true);

// Also prevent context menu on right-click (for games using right mouse button)
window.addEventListener('contextmenu', function(e) {
    e.preventDefault();
}, false);
