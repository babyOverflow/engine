#include "render/render.h"
#include "Application.h"

#include "ExampleLayer.h"
#include <print>

int main()
{
    core::ApplicationSpec spec;
    core::WindowSpec winSpec = {800, 600, "My Application"};
    spec.winSpec = &winSpec;
    auto re = core::Application::Create(spec);

    if (!re) {
        return -1;
    }
    auto app = std::move(re.value());
    auto layer =  ExampleLayer::Create(&app);
    if (!layer)
    {
        std::println("failed to creat layer");
        return -1;
    }
    app.AttachLayer(std::move(layer));
    app.Run();
}