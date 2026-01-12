#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <Event.h>
#include <render/render.h>

namespace common {

struct Projection {
    glm::mat4x4 proj;

    static Projection Perspective(float fov, float width, float height, float znear, float zfar);
};

struct GameCamera {
    glm::vec3 position;
    float yaw;
    float pitch;
    Projection proj;

    core::render::CameraUniformData GetCameraUniformData();
};

class CameraController {
  public:
    bool OnKeyboardPress(core::event::KeyPressEvent& event);
    bool OnMouseMove(core::event::CursorPosEvent& event);
    bool OnKeyboardRelease(core::event::KeyReleaseEvent& event);
    void UpdateCamera(GameCamera& camera, float delta);

  private:
    float amountLeft = 0;
    float amountRight = 0;
    float amountForward = 0;
    float amountBackWard = 0;
    float amountUp = 0;
    float amountDown = 0;
    float rotateHorizontal = 0;
    float rotateVertical = 0;
    float scroll = 0;
    float speed = 1.F;
    float sensitivity = 0.1F;

    float prevCusorX = 0;
    float prevCusorY = 0;
};


}  // namespace common