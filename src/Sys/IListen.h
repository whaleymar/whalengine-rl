#pragma once

#include <type_traits>
#include "Event.h"
#include "System.h"

namespace whal {

template <typename E, bool RunOnPause, typename... T>
    requires(std::is_base_of<IEvent<T...>, E>::value)
class IListen {
public:
    virtual ~IListen() { Event.stopListening<E, T...>(mListener); }
    virtual void onEvent(E, T...) = 0;

protected:
    IListen()
        : mListener([this](T... args) {
              if constexpr (RunOnPause) {
                  this->onEvent(E{}, args...);
              } else {
                  if (!System::isPaused())
                      this->onEvent(E{}, args...);
              }
          }) {
        Event.registerListener<E>(mListener);
    }

private:
    EventListener<T...> mListener;
};

}  // namespace whal
