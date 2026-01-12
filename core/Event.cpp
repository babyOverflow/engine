#include "Event.h"

void core::EventDispatcher::Post(Event event) {
    m_commandBuffer.push_back(event);
}
