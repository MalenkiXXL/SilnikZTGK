#pragma once
#include <functional>

enum class EventType
{
	None,
	KeyPressed, KeyReleased, KeyTyped,
	MouseMoved, MouseScrolled, MouseButtonPressed, MouseButtonReleased,
	WindowResize, WindowClose,
	EntityTransformChanged,
	EntityDeleted
};

class Event
{
	
public:
	virtual ~Event() = default;

	virtual EventType GetEventType() const = 0;
	bool Handled = false;
protected:

};

class EventDispatcher
{
public:
	EventDispatcher(Event& event)
		: m_Event(event) { }
	
	template<typename T>
	bool Dispatch(std::function<bool(T&)> func)
	{
		if (m_Event.GetEventType() == T::GetStaticType())
		{
			T& event = static_cast<T&>(m_Event);
			m_Event.Handled = func(event);
			return true;
		}
		return false;
	}
private:
	Event& m_Event;
};