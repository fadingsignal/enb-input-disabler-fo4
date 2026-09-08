#pragma once

namespace ENBInputDisabler
{
struct CursorRect
{
    long left, top, right, bottom;
};

// Only widen a smaller rectangle wholly inside the client area. In particular,
// preserve the desktop-sized rectangle the game restores after pumping messages.
constexpr bool ShouldExpandClip(const CursorRect& requested, const CursorRect& client)
{
    return requested.left < requested.right && requested.top < requested.bottom &&
           client.left < client.right && client.top < client.bottom &&
           requested.left >= client.left && requested.top >= client.top &&
           requested.right <= client.right && requested.bottom <= client.bottom &&
           (requested.left != client.left || requested.top != client.top ||
            requested.right != client.right || requested.bottom != client.bottom);
}

bool InstallCursorBoundsFix();
void SetCursorEditorActive(bool active);
}
