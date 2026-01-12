
#include "PipelineManager.h"

namespace core::render {

const GpuRenderPipeline* PipelineManager::GetRenderPipeline(const PipelineDesc& desc) {
    if (m_pipelineCache.contains(desc))
    {
        return &m_pipelineCache.at(desc);
    }
    
       

    return nullptr;
}
}
