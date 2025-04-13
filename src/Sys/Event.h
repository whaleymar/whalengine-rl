#pragma once

#include <functional>
#include <vector>

#include "Util/Singleton.h"
#include "Util/Types.h"
#include "whalECS/src/Traits.h"

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
    SINGLETON(EventManager)
public:
    template <typename E, typename... T>
        requires(std::is_base_of<IEvent<T...>, E>::value)
    void registerListener(EventListener<T...>& listener) {
        listener.mId = S_LISTENER_ID++;
        auto id = getEventId<E>();
        if (!mListeners.contains(id)) {
            mListeners[id] = {};
        }
        mListeners[id].push_back(&listener);
    }

    template <typename E, typename... T>
    void stopListening(EventListener<T...>& listener) {
        const EventId eventId = getEventId<E>();
        if (!mListeners.contains(eventId)) {
            return;
        }

        size_t ix = 0;
        for (auto genericListener : mListeners[eventId]) {
            EventListener<T...>* eventListener = static_cast<EventListener<T...>*>(genericListener);
            if (eventListener->id() == listener.id()) {
                removeListenerAt(eventId, ix);
                return;
            }
            ix++;
        }
    }

    template <typename E, typename... T>
        requires(is_base_of_template<IEvent, E>::value)
    void emit(T... args) {
        const EventId eventId = getEventId<E>();
        if (!mListeners.contains(eventId)) {
            return;
        }

        for (auto genericListener : mListeners[eventId]) {
            EventListener<T...>* eventListener = static_cast<EventListener<T...>*>(genericListener);
            eventListener->callback(args...);
        }
    }

private:
    void removeListenerAt(EventId eventId, size_t ix) {
        auto last = mListeners[eventId].back();
        mListeners[eventId][ix] = last;
        mListeners[eventId].pop_back();
    }

    template <typename T>
    EventId getEventId() {
        static EventId id_ = S_EVENT_ID++;
        return id_;
    }

    std::unordered_map<EventId, std::vector<void*>> mListeners;
    inline static ListenerId S_LISTENER_ID = 1;  // 0 is invalid id;
    inline static EventId S_EVENT_ID = 1;
};

}  // namespace whal
