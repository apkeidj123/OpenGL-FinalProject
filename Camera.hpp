#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "../Externals/Include/Include.h"

class Camera {
private:
    vec3 pos = vec3(0, 0, 10);
    vec3 front;
    vec3 up;
    vec3 right;

    float pitch = 0.0;
    float yaw = -90.0;
    float mouse_last_x;
    float mouse_last_y;
    bool is_dragging = false;

    void update();

public:
    mat4 view;
    mat4 proj;
    Camera();
    void onReshape(int, int);
    void onMouse(int, int, int, int);
    void onMouseMotion(int, int);
    void onKeyPressed(char);
};

void Camera::update() {
    this->front = normalize(vec3(
        cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch)),
        sin(glm::radians(this->pitch)),
        sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch))
    ));
    this->right = normalize(cross(this->front, vec3(0, 1, 0)));
    this->up = normalize(cross(right, front));
    this->view = lookAt(pos, pos + front, up);
}

Camera::Camera() {
    this->update();
}

void Camera::onReshape(int w, int h) {
    glViewport(0, 0, w, h);
    this->proj = perspective(radians(60.0f), float(w) / h, 0.1f, 500.0f);
}

void Camera::onMouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        this->is_dragging = true;
        this->mouse_last_x = x;
        this->mouse_last_y = y;
    }
    if (state == GLUT_UP) {
        this->is_dragging = false;
        this->mouse_last_x = -1.0f;
        this->mouse_last_y = -1.0f;
    }
}

void Camera::onMouseMotion(int x, int y) {
    if (!this->is_dragging) {
        return;
    }

    float delta_x = 0.1 * (x - this->mouse_last_x);
    float delta_y = 0.1 * (y - this->mouse_last_y);
    this->mouse_last_x = x;
    this->mouse_last_y = y;
    this->yaw -= delta_x;
    this->pitch += delta_y;

    if (this->pitch > 89.0) {
        this->pitch = 89;
    }
    if (this->pitch < -89.0) {
        this->pitch = -89;
    }

    this->update();
}

void Camera::onKeyPressed(char key) {
    if (key == 'w') {
        this->pos += this->front * 10.0f;
    }
    if (key == 's') {
        this->pos -= this->front * 10.0f;
    }
    if (key == 'd') {
        this->pos += this->right * 10.0f;
    }
    if (key == 'a') {
        this->pos -= this->right * 10.0f;
    }
    if (key == 'c') {
        this->yaw = 0.0;
        this->pitch = 0.0;
        this->pos = vec3(0, 500, 0);
    }
    this->update();
}

#endif
