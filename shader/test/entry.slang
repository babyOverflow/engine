#include <ShaderInterop.h>
#include "core_interface.slang"

#ifndef ENGINE_STATIC_DISPATCH
type_param P : IRenderPass;
#endif

[vk::binding(0, 0)]
ParameterBlock<P.GlobalResourcesType> globalResources;

[vk::binding(0, 1)]
ParameterBlock<P.M.DataType> material;

[vk::binding(0, 2)]
ParameterBlock<P.PassResourceType> currentPass;

[shader("vertex")]
P.VertexOutputType vertexMain(P.VertexInputType input) {
    return P.vertex(globalResources, material, currentPass, input);
}

[shader("fragment")]
P.FragmentOutputType fragmentMain(P.FragmentInputType input) {
    return P.fragment(globalResources, material, currentPass, input);
}
