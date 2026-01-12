#pragma once
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "event/InputEvent.h"
#include "event/WindowEvent.h"

namespace core {
using Event = std::variant<event::KeyPressEvent,
                           event::KeyReleaseEvent,
                           event::WindowCloseEvent,
                           event::CursorPosEvent,
                           event::WindowResizeEvent,
                           event::ScrollEvent>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class EventDispatcher {
  public:
    void Post(Event event);

    template <typename Handler>
    void ProcessEvent(Handler&& handler)
        requires std::invocable<Handler, const Event&>
    {
        if (m_commandBuffer.empty()) {
            return;
        }
        std::swap(m_commandBuffer, m_processingBuffer);

        for (auto& e : m_processingBuffer) {
            handler(e);
        }
        m_processingBuffer.clear();
    }

  private:
    std::vector<Event> m_commandBuffer;
    std::vector<Event> m_processingBuffer;
};
}  // namespace core