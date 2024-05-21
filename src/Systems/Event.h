#pragma once

#include <vector>

#include "Util/Types.h"

namespace whal {

// These aren't going to overflow so why would I check
using EventId = u32;
using ListenerId = u32;

class EventManager;
struct System;

class EventBase {
public:
    EventBase() { mId = S_EVENT_ID++; }
    EventId id() const { return mId; }

private:
    inline static EventId S_EVENT_ID = 0;
    EventId mId;
};

template <typename... T>
class IEvent : public EventBase {
public:
    IEvent() : EventBase() {}
};

template <typename... T>
class EventListener {
public:
    friend EventManager;
    using callbackFunc = void (*)(T... args);
    EventListener(callbackFunc func) : mCallback(func) {}

    void callback(T... args) { mCallback(args...); }
    ListenerId id() const { return mId; }

private:
    callbackFunc mCallback;
    ListenerId mId = 0;
};

class EventManager {
public:
    friend System;

    template <typename... T>
    void registerListener(IEvent<T...> event, EventListener<T...>& listener) {
        listener.mId = S_LISTENER_ID++;
        mEvents.push_back({event, &listener});
    }

    template <typename... T>
    void stopListening(IEvent<T...> event, EventListener<T...>& listener) {
        for (size_t i = 0; i < mEvents.size(); i++) {
            EventListener<T...>* eventListener = static_cast<EventListener<T...>*>(mEvents[i].second);
            if (mEvents[i].first.id() == event.id() && eventListener->id() == listener.id()) {
                removeListenerAt(i);
            }
        }
    }

    template <typename... T>
    void triggerEvent(IEvent<T...> newEvent, T... args) {
        for (auto& [event, listener] : mEvents) {
            if (event.id() != newEvent.id()) {
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
        std::pair<EventBase, void*> last = mEvents.back();
        mEvents[ix] = last;
        mEvents.pop_back();
    }

    // RESEARCH can do a std::unordered_map<EventBase, std::vector<void*>> if this gets too slow
    std::vector<std::pair<EventBase, void*>> mEvents;
    inline static ListenerId S_LISTENER_ID = 1;  // 0 is invalid id;
};

}  // namespace whal
