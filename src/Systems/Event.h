#pragma once

#include <functional>
#include <vector>

#include "Util/Types.h"

namespace whal {

// These aren't going to overflow so why would I check
using EventId = u32;
using ListenerId = u32;

class EventManager;
struct System;

template <typename... T>
class IEvent {};

template <typename... T>
class EventListener {
public:
    friend EventManager;
    using callbackFunc = void (*)(T... args);
    EventListener(std::type_identity_t<std::function<void(T...)>> const& func) : mCallback(func) {}

    void callback(T... args) { mCallback(args...); }
    ListenerId id() const { return mId; }

private:
    std::type_identity_t<std::function<void(T...)>> mCallback;
    ListenerId mId = 0;
};

class EventManager {
public:
    friend System;

    template <typename E, typename... T>
        requires(std::is_base_of<IEvent<T...>, E>::value)
    void registerListener(EventListener<T...>& listener) {
        // static_assert(is_base_of_template<IEvent, E>::value, "Event must inherit from IEvent");
        listener.mId = S_LISTENER_ID++;
        mEvents.push_back({getEventId<E>(), &listener});
    }

    template <typename E, typename... T>
    void stopListening(EventListener<T...>& listener) {
        const EventId eventId = getEventId<E>();
        for (size_t i = 0; i < mEvents.size(); i++) {
            EventListener<T...>* eventListener = static_cast<EventListener<T...>*>(mEvents[i].second);
            if (mEvents[i].first == eventId && eventListener->id() == listener.id()) {
                removeListenerAt(i);
            }
        }
    }

    template <typename E, typename... T>
    void triggerEvent(T... args) {
        // static_assert(is_base_of_template<IEvent, E>::value, "Event must inherit from IEvent");
        for (auto& [eventId, listener] : mEvents) {
            if (eventId != getEventId<E>()) {
                continue;
            }

            EventListener<T...>* eventListener = static_cast<EventListener<T...>*>(listener);
            eventListener->callback(args...);
        }
    }

private:
    EventManager() = default;
    EventManager(EventManager& other) = delete;
    void removeListenerAt(int ix) {
        auto lastPair = mEvents.back();
        mEvents[ix] = lastPair;
        mEvents.pop_back();
    }

    template <typename T>
    EventId getEventId() {
        static EventId id_ = S_EVENT_ID++;
        return id_;
    }

    // RESEARCH can do a std::unordered_map<EventBase, std::vector<void*>> if this gets too slow
    std::vector<std::pair<EventId, void*>> mEvents;
    inline static ListenerId S_LISTENER_ID = 1;  // 0 is invalid id;
    inline static EventId S_EVENT_ID = 1;
};

}  // namespace whal
