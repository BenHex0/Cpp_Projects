#pragma once

#include <windows.h>
#include <unordered_map>

class InputHandler
{
private:
    std::unordered_map<int, bool> currentKeys;
    std::unordered_map<int, bool> previousKeys;

public:
    void update()
    {
        previousKeys = currentKeys; // Save previous state

        // List of keys you want to monitor
        int keys[] = {VK_LEFT, VK_RIGHT, VK_SPACE, 'A', 'D', 'W', 'S', VK_ESCAPE};

        for (int key : keys)
        {
            currentKeys[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
        }
    }

    bool isKeyDown(int key) const
    {
        auto it = currentKeys.find(key);
        return it != currentKeys.end() ? it->second : false;
    }

    bool isKeyPressed(int key) const
    {
        return isKeyDown(key) && !wasKeyDown(key);
    }

    bool wasKeyDown(int key) const
    {
        auto it = previousKeys.find(key);
        return it != previousKeys.end() ? it->second : false;
    }

    bool isKeyReleased(int key) const
    {
        return !isKeyDown(key) && wasKeyDown(key);
    }
};