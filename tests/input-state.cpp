#include "InputState.h"
#include "CursorBounds.h"
#include <cstdlib>
#include <iostream>
#include <source_location>
void Check(bool value, std::source_location location = std::source_location::current())
{
    if (!value) {
        std::cerr << "Check failed at " << location.file_name() << ':' << location.line() << '\n';
        std::abort();
    }
}
int main()
{
    using ENBInputDisabler::ShouldExpandClip;
    using ENBInputDisabler::CursorRect;
    constexpr CursorRect window{0, 0, 3840, 2160};
    Check(ShouldExpandClip({960, 540, 2880, 1620}, window));
    Check(!ShouldExpandClip(window, window));
    Check(!ShouldExpandClip({-1920, 0, 3840, 2160}, window)); // Desktop restoration.
    Check(!ShouldExpandClip({-1, 0, 3000, 2000}, window)); // Partly outside.
    Check(!ShouldExpandClip({100, 100, 100, 200}, window)); // Empty.
    Check(!ShouldExpandClip({0, 0, 100, 100}, {0, 0, 0, 0})); // Minimized.
    Check(ShouldExpandClip({-1500, 200, -500, 900}, {-1920, 0, 0, 1080})); // Left monitor.
    Check(ShouldExpandClip({400, 300, 900, 600}, {200, 100, 1200, 800})); // Moved window.
    ENBInputDisabler::InputState state;
    bool flag = false;
    state.Update(nullptr, true);
    state.Update(&flag, false); Check(!flag);
    state.Update(&flag, true); Check(flag);
    flag = false; // Another engine writer cleared the flag during the session.
    state.Update(&flag, true);
    Check(flag); // Suppression must be reasserted without forgetting the old value.
    state.Update(&flag, false); Check(!flag);
    flag = true;
    state.Update(&flag, false); Check(flag);
    state.Update(&flag, true);
    state.Update(&flag, false); Check(flag);
    flag = false;
    state.Update(&flag, true);
    state.Update(nullptr, false);
    bool replacement = false;
    state.Update(&replacement, true); Check(replacement);
    state.Update(&replacement, false); Check(!replacement);
    std::cout << "Input ownership, restoration, and cursor bounds checks passed\n";
}
