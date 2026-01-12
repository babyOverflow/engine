#include "Window.h"
#include <iostream>
#include <print>

static core::event::KeyCode GlfwToMyKeycode(int key) {
    using core::event::KeyCode;
    switch (key) {
        case GLFW_KEY_W:
            return KeyCode::W;
        case GLFW_KEY_A:
            return KeyCode::A;
        case GLFW_KEY_S:
            return KeyCode::S;
        case GLFW_KEY_D:
            return KeyCode::D;
        case GLFW_KEY_Q:
            return KeyCode::Q;
        case GLFW_KEY_E:
            return KeyCode::E;
        case GLFW_KEY_R:
            return KeyCode::R;
        case GLFW_KEY_F:
            return KeyCode::F;
        case GLFW_KEY_Z:
            return KeyCode::Z;
        case GLFW_KEY_X:
            return KeyCode::X;
        case GLFW_KEY_C:
            return KeyCode::C;
        case GLFW_KEY_V:
            return KeyCode::V;

        case GLFW_KEY_UP:
            return KeyCode::Up;
        case GLFW_KEY_DOWN:
            return KeyCode::Down;
        case GLFW_KEY_LEFT:
            return KeyCode::Left;
        case GLFW_KEY_RIGHT:
            return KeyCode::Right;

        case GLFW_KEY_SPACE:
            return KeyCode::Space;
        case GLFW_KEY_ENTER:
            return KeyCode::Enter;
        case GLFW_KEY_ESCAPE:
            return KeyCode::Escape;
        case GLFW_KEY_TAB:
            return KeyCode::Tab;
        case GLFW_KEY_BACKSPACE:
            return KeyCode::Backspace;
        case GLFW_KEY_DELETE:
            return KeyCode::Delete;

        case GLFW_KEY_LEFT_SHIFT:
            return KeyCode::LeftShift;
        case GLFW_KEY_RIGHT_SHIFT:
            return KeyCode::RightShift;
        case GLFW_KEY_LEFT_CONTROL:
            return KeyCode::LeftControl;
        case GLFW_KEY_RIGHT_CONTROL:
            return KeyCode::RightControl;
        case GLFW_KEY_LEFT_ALT:
            return KeyCode::LeftAlt;
        case GLFW_KEY_RIGHT_ALT:
            return KeyCode::RightAlt;

        default:
            return KeyCode::None;
    }
}

namespace core {

std::expected<Window, int> core::Window::Create(WindowSpec& spec) {
    glfwSetErrorCallback([](int error, const char* description) {
        // 에러 코드와 사람이 읽을 수 있는 설명을 출력
        std::println(stderr, "GLFW Error [{}]: {}", error, description);
    });

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(spec.width, spec.height, spec.title, nullptr, nullptr);

    glfwSetWindowUserPointer(window, spec.eventDispatcher);
    glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
        EventDispatcher* dispatcher = (EventDispatcher*)glfwGetWindowUserPointer(window);
        dispatcher->Post(event::WindowCloseEvent{});
    });

    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        EventDispatcher* dispatcher = (EventDispatcher*)glfwGetWindowUserPointer(window);
        dispatcher->Post(event::ScrollEvent{.xoffset = xoffset, .yoffset = yoffset});
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mode) {
        EventDispatcher* dispatcher = (EventDispatcher*)glfwGetWindowUserPointer(window);
        if (action == GLFW_PRESS) {
            dispatcher->Post(event::KeyPressEvent{.keyCode = GlfwToMyKeycode(key)});
        } else if (action == GLFW_RELEASE) {
            dispatcher->Post(event::KeyReleaseEvent{.keyCode = GlfwToMyKeycode(key)});
        }
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {});

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        EventDispatcher* dispatcher = (EventDispatcher*)glfwGetWindowUserPointer(window);
        dispatcher->Post(event::CursorPosEvent{.x = xpos, .y = ypos});
    });

    return Window(window, spec);
}

void Window::PollEvent() {
    glfwPollEvents();
}

Window::~Window() {}
}  // namespace core