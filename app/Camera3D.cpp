#include <print>
#include "Camera3D.h"

bool common::CameraController::OnKeyboardPress(core::event::KeyPressEvent& event) {
    using core::event::KeyCode;
    switch (event.keyCode) {
        case KeyCode::W:
            amountForward = 1.F;
            break;
        case KeyCode::S:
            amountBackWard = 1.F;
            break;
        case KeyCode::A:
            amountLeft = 1.F;
            break;
        case KeyCode::D:
            amountRight = 1.F;
            break;
        default:
            return false;
    }
    return true;
}

bool common::CameraController::OnMouseMove(core::event::CursorPosEvent& event)
{
    rotateHorizontal += prevCusorX - event.x;
    rotateVertical  += prevCusorY - event.y;
    prevCusorX = event.x;
    prevCusorY = event.y;
    return true;
}

bool common::CameraController::OnKeyboardRelease(core::event::KeyReleaseEvent& event) {
    using core::event::KeyCode;
    switch (event.keyCode) {
        case KeyCode::W:
            amountForward = 0.F;
            break;
        case KeyCode::S:
            amountBackWard = 0.F;
            break;
        case KeyCode::A:
            amountLeft = 0.F;
            break;
        case KeyCode::D:
            amountRight = 0.F;
            break;
        default:
            return false;
    }
    return true;
}

void common::CameraController::UpdateCamera(GameCamera& camera, float delta) {
    float yaw_sin = glm::sin(camera.yaw);
    float yaw_cos = glm::cos(camera.yaw);
    glm::vec3 forward = glm::normalize(glm::vec3(yaw_cos, 0.0, yaw_sin));
    glm::vec3 right = glm::normalize(glm::vec3(-yaw_sin, 0.0, yaw_cos));

    glm::vec3 positionDelta = forward * (amountForward - amountBackWard) * speed * delta;
    positionDelta += right * (amountRight - amountLeft) * speed * delta;

    float pitch_sin = glm::sin(camera.pitch);
    float pitch_cos = glm::cos(camera.pitch);

    float up = (amountUp - amountDown) * speed * delta;
    positionDelta.y += up;

    camera.yaw += rotateHorizontal * sensitivity * delta;
    camera.pitch += rotateVertical * sensitivity * delta;
    rotateHorizontal = 0;
    rotateVertical = 0;
    camera.position += positionDelta;
}

core::render::CameraUniformData common::GameCamera::GetCameraUniformData() {
    const float yaw_sin = glm::sin(yaw);
    const float yaw_cos = glm::cos(yaw);
    const float pitch_sin = glm::sin(pitch);
    const float pitch_cos = glm::cos(pitch);

    glm::vec3 direction =
        glm::normalize(glm::vec3(pitch_cos * yaw_cos, pitch_sin, pitch_cos * yaw_sin));

    glm::mat4x4 view = glm::lookAtRH(position, direction + position, glm::vec3(0.F, 1.F, 0.F));

    return core::render::CameraUniformData{
        .view = view,
        .proj = proj.proj,
        .viewProj = proj.proj * view,
        .position = position,
    };
}

common::Projection common::Projection::Perspective(float fov,
                                                   float width,
                                                   float height,
                                                   float znear,
                                                   float zfar) {
    return Projection{glm::perspectiveFovRH(fov, width, height, znear, zfar)};
}
