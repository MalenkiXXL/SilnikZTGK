#pragma once

#include "Event.h"

class WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(int width, int height)
		: m_Width(width), m_Height(height) { }

	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }

	static EventType GetStaticType() { return EventType::WindowResize; };
	virtual EventType GetEventType() const override { return EventType::WindowResize; }

protected:
	int m_Width, m_Height;
};

class WindowCloseEvent : public Event
{
public:
	WindowCloseEvent()
	{
	}
	static EventType GetStaticType() { return EventType::WindowClose;};
	virtual EventType GetEventType() const override { return EventType::WindowClose; };
};
