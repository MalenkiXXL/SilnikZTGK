#pragma once
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>

class IEventSignal {
public:
    virtual ~IEventSignal() = default;
};

template <typename T>
class EventSignal : public IEventSignal {
public:
    using Callback = std::function<void(const T&)>;

    std::unordered_map<std::size_t, Callback> listeners;
    std::size_t nextId = 0;

    std::size_t Subscribe(Callback callback) {
        listeners[nextId] = callback;
        return nextId++;
    }

    void Unsubscribe(std::size_t id) {
        listeners.erase(id);
    }

    void Publish(const T& event) {
        for (auto& [id, listener] : listeners) {
            listener(event);
        }
    }
};

class EventBus {
private:
    std::unordered_map<std::type_index, std::unique_ptr<IEventSignal>> signals;

    template <typename T>
    EventSignal<T>* GetSignal() {
        std::type_index typeIndex(typeid(T));
        if (signals.find(typeIndex) == signals.end()) {
            signals[typeIndex] = std::make_unique<EventSignal<T>>();
        }
        return static_cast<EventSignal<T>*>(signals[typeIndex].get());
    }

public:
    template <typename T>
    std::size_t Subscribe(std::function<void(const T&)> callback) {
        return GetSignal<T>()->Subscribe(callback);
    }

    template <typename T>
    void Unsubscribe(std::size_t id) {
        GetSignal<T>()->Unsubscribe(id);
    }

    template <typename T>
    void Publish(const T& event) {
        GetSignal<T>()->Publish(event);
    }
};