#pragma once

#include "Event.h"
#include "Scene.h"

namespace core {
class Layer {
  public:
    Layer() = default;
    virtual ~Layer() = default;

    virtual void OnAttach(core::Scene& scene) = 0;
    virtual void OnUpdate(core::Scene& scene) = 0;
    virtual bool OnEvent(Event& event) = 0;
};
}  // namespace core
