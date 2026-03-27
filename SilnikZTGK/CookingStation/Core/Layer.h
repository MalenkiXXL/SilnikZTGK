#pragma once
#include "CookingStation/Events/Event.h"
#include <sstream>
class Layer
{
public:
	Layer(std::string layerName = "Layer")
		: debugName(layerName) {}
	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate() {}
	virtual void OnEvent(Event& event) {}

	inline const std::string& GetName() const { return debugName; }

protected:
	std::string debugName;
};

