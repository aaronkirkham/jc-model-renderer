#pragma once

#include <StdInc.h>
#include <singleton.h>

struct InputEvents
{
    ksignals::Event<void(uint32_t)> KeyDown;
    ksignals::Event<void(uint32_t)> KeyUp;
    ksignals::Event<void(const glm::vec2&)> MouseDown;
    ksignals::Event<void(const glm::vec2&)> MouseUp;
    ksignals::Event<void(const glm::vec2&)> MouseMove;
};

class Input : public Singleton<Input>
{
private:
    InputEvents m_InputEvents;
    std::array<bool, 255> m_KeyboardState = { false };
    glm::vec2 m_MousePosition;
    glm::vec2 m_LastClickPosition;

public:
    virtual ~Input();

    virtual InputEvents& Events() { return m_InputEvents; }

    void Initialise();

    bool HandleMessage(MSG* message);
    void Update();

    bool IsKeyPressed(uint8_t key) { return m_KeyboardState[key]; }

    const glm::vec2& GetMousePosition() { return m_MousePosition; }
    const glm::vec2& GetLastClickPosition() { return m_LastClickPosition; }
};