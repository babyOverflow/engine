#pragma once

#include <expected>
#include <memory>
#include <functional>

#include <GLFW/glfw3.h>

#include "Event.h"

struct GLFWWindowDestroyer {
    void operator()(GLFWwindow* ptr) const {
        // 이미 nullptr인지는 unique_ptr이 검사해주지만, 안전을 위해
        if (ptr) {
            glfwDestroyWindow(ptr);
        }
    }
};

namespace core {

struct WindowSpec {
    uint32_t width;
    uint32_t height;
    const char* title;

    using EventCallbackFn = std::function<void(Event&)>;
    EventDispatcher* eventDispatcher;
};

class Window {
  public:
    static std::expected<Window, int> Create(WindowSpec& spec);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) = default;
    Window& operator=(Window&& other) = default;

    GLFWwindow* GetGLFWwindow() { return m_window.get(); }
    uint32_t GetWidth() const { return m_spec.width; }
    uint32_t GetHeight() const { return m_spec.height; }
    void PollEvent();

  private:
    Window(GLFWwindow* window, WindowSpec spec) : m_window(window), m_spec(spec) {}
    WindowSpec m_spec;
    std::unique_ptr<GLFWwindow, GLFWWindowDestroyer> m_window;
};
}  // namespace core