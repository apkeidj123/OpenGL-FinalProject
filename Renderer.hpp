#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "../Externals/Include/Include.h"
#include "Scene.hpp"
#include "Camera.hpp"

using namespace glm;
using namespace std;


class Renderer {
private:
    float cur_time;
    GLuint program;
    GLuint loc_model;
    GLuint loc_view;
    GLuint loc_proj;
    GLuint loc_mtl_ambient;
    GLuint loc_mtl_diffuse;
    GLuint loc_mtl_specular;
    GLuint loc_mtl_shininess;
public:
    Renderer() {;}
    Renderer(string vs_path, string fs_path);
    void render(const Scene& scene, const Camera& camera);
};

Renderer::Renderer(string vs_path, string fs_path) {
    this->program = glCreateProgram();
    auto vs_shader = glCreateShader(GL_VERTEX_SHADER);
    auto fs_shader = glCreateShader(GL_FRAGMENT_SHADER);
    char** vs_source = loadShaderSource(vs_path.c_str());
    char** fs_source = loadShaderSource(fs_path.c_str());
    glShaderSource(vs_shader, 1, vs_source, NULL);
    glShaderSource(fs_shader, 1, fs_source, NULL);
    freeShaderSource(vs_source);
    freeShaderSource(fs_source);
    glCompileShader(vs_shader);
    glCompileShader(fs_shader);
    shaderLog(vs_shader);
    shaderLog(fs_shader);
    glAttachShader(this->program, vs_shader);
    glAttachShader(this->program, fs_shader);
    glLinkProgram(this->program);

    this->loc_model = glGetUniformLocation(this->program, "model");
    this->loc_view = glGetUniformLocation(this->program, "view");
    this->loc_proj = glGetUniformLocation(this->program, "proj");
    this->loc_mtl_ambient = glGetUniformLocation(this->program, "mtl.ambient");
    this->loc_mtl_diffuse = glGetUniformLocation(this->program, "mtl.diffuse");
    this->loc_mtl_specular = glGetUniformLocation(this->program, "mtl.specular");
    this->loc_mtl_shininess = glGetUniformLocation(this->program, "mtl.shininess");
}

void Renderer::render(const Scene& scene, const Camera& camera) {
    this->cur_time = glutGet(GLUT_ELAPSED_TIME); // ms
    cout << cur_time << ":" << endl;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.5, 0.5, 0.5, 0.0);
    glUseProgram(this->program);

    glUniformMatrix4fv(this->loc_view, 1, GL_FALSE, value_ptr(camera.view));
    glUniformMatrix4fv(this->loc_proj, 1, GL_FALSE, value_ptr(camera.proj));

    for (const Mesh& mesh : scene.meshes) {
        cout << "Drawing " << mesh.name << endl;
        mat4 model = scene.model;
        if (mesh.animation_id != -1) {
            const Animation& anim = scene.animations[mesh.animation_id];
            mat4 local_model = anim.animate(cur_time);
            model = model * local_model;
        }
        glUniformMatrix4fv(this->loc_model, 1, GL_FALSE, value_ptr(model));

        const Material& mtl = scene.materials[mesh.material_id];
        glUniform3fv(this->loc_mtl_ambient, 1, &mtl.ambient[0]);
        glUniform3fv(this->loc_mtl_diffuse, 1, &mtl.diffuse[0]);
        glUniform3fv(this->loc_mtl_specular, 1, &mtl.specular[0]);
        glUniform1f(this->loc_mtl_shininess, mtl.shininess);

        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
        glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, 0);
    }
    cout << "========================" << endl;

    glutSwapBuffers();
}

#endif