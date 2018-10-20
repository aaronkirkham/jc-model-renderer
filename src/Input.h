#pragma once

#include <StdInc.h>
#include <singleton.h>

struct InputEvents {
    ksignals::Event<void(uint32_t)>                   KeyDown;
    ksignals::Event<void(uint32_t)>                   KeyUp;
    ksignals::Event<void(uint32_t, const glm::vec2&)> MousePress;
    ksignals::Event<void(const glm::vec2&)>           MouseMove;
    ksignals::Event<void(float)>                      MouseScroll;
};

class Input : public Singleton<Input>
{
  private:
    InputEvents           m_InputEvents;
    std::array<bool, 255> m_KeyboardState = {false};
    glm::vec2             m_MousePosition;
    glm::vec3             m_MouseWorldPosition;
    glm::vec2             m_LastClickPosition;
    glm::vec3             m_LastClickWorldPosition;

  public:
    Input()          = default;
    virtual ~Input() = default;

    virtual InputEvents& Events()
    {
        return m_InputEvents;
    }

    void Initialise();
    bool HandleMessage(MSG* message);

    bool IsKeyPressed(uint8_t key)
    {
        return m_KeyboardState[key];
    }

    const glm::vec2& GetMousePosition()
    {
        return m_MousePosition;
    }
    const glm::vec3& GetMouseWorldPosition()
    {
        return m_MouseWorldPosition;
    }
    const glm::vec2& GetLastClickPosition()
    {
        return m_LastClickPosition;
    }
    const glm::vec3& GetLastClickWorldPosition()
    {
        return m_LastClickWorldPosition;
    }
};
