#pragma once
#include "CookingStation/Events/Event.h"

class GamepadEvent : public Event
{
public:
	inline int GetGamepadId() const { return m_GamepadId; }

protected:
	GamepadEvent(int gamepadId)
		: m_GamepadId(gamepadId) {
	}

	int m_GamepadId;
};

class GamepadButtonEvent : public GamepadEvent
{
public:
	inline int GetButton() const { return m_Button; }

protected:
	GamepadButtonEvent(int button, int gamepadId)
		: GamepadEvent(gamepadId), m_Button(button) {
	}

	int m_Button;
};

class GamepadButtonPressedEvent : public GamepadButtonEvent
{
public:
	GamepadButtonPressedEvent(int button, int gamepadId = 0)
		: GamepadButtonEvent(button, gamepadId) {
	}

	static EventType GetStaticType() { return EventType::GamepadButtonPressed; }
	virtual EventType GetEventType() const override { return EventType::GamepadButtonPressed; }
};

class GamepadButtonReleasedEvent : public GamepadButtonEvent
{
public:
	GamepadButtonReleasedEvent(int button, int gamepadId = 0)
		: GamepadButtonEvent(button, gamepadId) {
	}

	static EventType GetStaticType() { return EventType::GamepadButtonReleased; }
	virtual EventType GetEventType() const override { return EventType::GamepadButtonReleased; }
};

class GamepadAxisMovedEvent : public GamepadEvent
{
public:
	GamepadAxisMovedEvent(int axis, float value, int gamepadId = 0)
		: GamepadEvent(gamepadId), m_Axis(axis), m_Value(value) {
	}

	inline int GetAxis() const { return m_Axis; }
	inline float GetValue() const { return m_Value; }

	static EventType GetStaticType() { return EventType::GamepadAxisMoved; }
	virtual EventType GetEventType() const override { return EventType::GamepadAxisMoved; }

private:
	int m_Axis;
	float m_Value;
};