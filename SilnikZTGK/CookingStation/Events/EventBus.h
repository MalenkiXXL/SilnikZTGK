#pragma once
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>

// Bazowa klasa do zacierania typów (type erasure)
class IEventSignal {
public:
    virtual ~IEventSignal() = default;
};

// Konkretny kontener dla danego typu eventu T
template <typename T>
class EventSignal : public IEventSignal {
public:
    using Callback = std::function<void(const T&)>;

    // U¿ywamy unordered_map, by ³atwo móc siê wypisaæ za pomoc¹ ID
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
    // Rejestruje funkcjê nas³uchuj¹c¹ i zwraca ID subskrypcji
    template <typename T>
    std::size_t Subscribe(std::function<void(const T&)> callback) {
        return GetSignal<T>()->Subscribe(callback);
    }

    // Wyrejestrowuje funkcjê po ID
    template <typename T>
    void Unsubscribe(std::size_t id) {
        GetSignal<T>()->Unsubscribe(id);
    }

    // Wysy³a event do wszystkich nas³uchuj¹cych
    template <typename T>
    void Publish(const T& event) {
        GetSignal<T>()->Publish(event);
    }
};