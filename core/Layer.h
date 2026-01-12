#pragma once

#include "render/RenderGraph.h"
#include "Event.h"

namespace core {
class Layer {
    public:
        Layer()          = default;
        virtual ~Layer() = default;
    
        virtual void OnAttach() = 0;
        virtual void OnRender(render::FrameContext& context) = 0;
        virtual void OnUpdate()   = 0;
        virtual bool OnEvent(Event& event) = 0;
};
}