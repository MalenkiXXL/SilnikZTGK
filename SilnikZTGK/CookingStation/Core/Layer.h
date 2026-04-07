#pragma once
#include "CookingStation/Events/Event.h"
#include "CookingStation/Core/Timestep.h"
#include <sstream>
class Layer
{
public:
	Layer(std::string layerName = "Layer")
		: debugName(layerName) {}
	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate(Timestep ts) {}
	virtual void OnEvent(Event& event) {}

	inline const std::string& GetName() const { return debugName; }

protected:
	std::string debugName;
};

