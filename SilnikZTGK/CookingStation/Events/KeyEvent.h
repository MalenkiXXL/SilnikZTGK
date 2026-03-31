#pragma once

#include "Event.h"

class KeyEvent : public Event
{
public:

	KeyEvent(int keycode)
		: m_KeyCode(keycode) {}

	int GetKeyCode() const { return m_KeyCode; }

protected:
	int m_KeyCode;
};


class KeyPressedEvent : public KeyEvent
{
public:
	KeyPressedEvent(int keycode, int repeatCount)
		: KeyEvent(keycode), m_RepeatCount(repeatCount) {}
	
	
	static EventType GetStaticType() { return EventType::KeyPressed; }
	virtual EventType GetEventType() const override { return EventType::KeyPressed; }
	inline int GetRepeatCode() const { return m_RepeatCount; }

private:
	int m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent
{
public:
	KeyReleasedEvent(int keycode)
		: KeyEvent(keycode) {}

	static EventType GetStaticType() { return EventType::KeyReleased; }
	virtual EventType GetEventType() const override { return EventType::KeyReleased; }
};

class KeyTypedEvent : public KeyEvent
{
public:
	KeyTypedEvent(int keycode)
		: KeyEvent(keycode) {
	}

	static EventType GetStaticType() { return EventType::KeyTyped; }
	virtual EventType GetEventType() const override { return EventType::KeyTyped; }
};