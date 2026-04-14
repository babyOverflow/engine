#pragma once

#include "render/RenderGraph.h"
#include "Scene.h"
#include "Event.h"

namespace core {
class Layer {
    public:
        Layer()          = default;
        virtual ~Layer() = default;
    
        virtual void OnAttach(core::Scene& scene) = 0;
        virtual void OnUpdate(core::Scene& scene)   = 0;
        virtual bool OnEvent(Event& event) = 0;
};
}