#pragma once
#include "Event.h"

class MouseButtonEvent : public Event
{
public:
    MouseButtonEvent(int button) : m_Button(button) {}
    int GetButton() const { return m_Button; }
protected:
    int m_Button;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}
    static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}
    static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
};

class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(double xPos, double yPos) : m_MouseX(xPos), m_MouseY(yPos) {}
    inline double GetXPos() const { return m_MouseX; }
    inline double GetYPos() const { return m_MouseY; }
    static EventType GetStaticType() { return EventType::MouseMoved; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
protected:
    double m_MouseX, m_MouseY;
};

class MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(double xOffSet, double yOffSet) : m_XOffset(xOffSet), m_YOffset(yOffSet) {}
    inline double GetXOffset() const { return m_XOffset; }
    inline double GetYOffset() const { return m_YOffset; }
    static EventType GetStaticType() { return EventType::MouseScrolled; }  // DODANE
    virtual EventType GetEventType() const override { return GetStaticType(); }
protected:
    double m_XOffset, m_YOffset;
};
