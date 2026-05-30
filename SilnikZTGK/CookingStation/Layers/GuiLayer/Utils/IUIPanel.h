#pragma once
#include "CookingStation/Events/Event.h"

class IUIPanel {
public:
    virtual ~IUIPanel() = default;

    // Metody cyklu życia panelu
    virtual void OnUpdate(float dt) {}
    virtual void Draw(float baseScale) = 0;
    virtual void OnEvent(Event& e) {}

    // Zarządzanie widocznością
    virtual void SetVisible(bool visible) { m_IsVisible = visible; }
    virtual bool IsVisible() const { return m_IsVisible; }

protected:
    bool m_IsVisible = false;
};