#pragma once

namespace ENBInputDisabler
{
// Only restore a flag we acquired. Inactive frames must not undo another
// plugin's input suppression. The game owns the pointed-to storage.
class InputState
{
public:
    void Update(bool* flag, bool editorOpen)
    {
        if (flag != owner) {
            // A replaced/destroyed singleton must never be dereferenced.
            owner = nullptr;
        }
        if (!flag) return;
        if (editorOpen) {
            if (!owner) {
                previous = *flag;
                owner = flag;
            }
            *flag = true;
        } else if (owner) {
            *flag = previous;
            owner = nullptr;
        }
    }
private:
    bool* owner{};
    bool previous{};
};
}
