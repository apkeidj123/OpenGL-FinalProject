#ifndef SCENE_HPP
#define SCENE_HPP

#include "../Externals/Include/Include.h"
#include "Util.hpp"
#define sz(x) (int(x.size()))

using namespace glm;
using namespace std;

struct Mesh {
    string name;
    vector<float> xyz;
    vector<float> nml;
    vector<float> uv;
    vector<unsigned int> indices;

    GLuint vao;
    GLuint vbo_xyz;
    GLuint vbo_nml;
    GLuint vbo_uv;
    GLuint ibo;
    int index_count;
    int material_id;
    int animation_id;

    Mesh() {
        xyz = vector<float>();
        nml = vector<float>();
        uv = vector<float>();
        indices = vector<unsigned int>();
        material_id = -1;
        animation_id = -1;
    }
};

struct Material {
    vec3 ambient = vec3(0.0f, 0.0f, 0.0f);
    vec3 diffuse = vec3(0.0f, 0.0f, 0.0f);
    vec3 specular = vec3(0.0f, 0.0f, 0.0f);
    float shininess = 32.0f;
};

struct Light {
    vec3 position;
    vec3 direction;
};

struct Animation {
    int duration;
    vector<aiQuatKey> rotations;
    vector<aiVectorKey> positions;
    vector<aiVectorKey> scalings;

    mat4 animate(float cur_time) const {
        cur_time = cur_time / 1000.0 * 24.0; // ms -> tick
        float anim_time = fmod(cur_time, this->duration);
        aiQuaternion R(this->rotations[0].mValue);
        aiVector3D T(this->positions[0].mValue);
        aiVector3D S(this->scalings[0].mValue);
        Assimp::Interpolator<aiQuaternion> interR;
        Assimp::Interpolator<aiVector3D> interT;
        Assimp::Interpolator<aiVector3D> interS;
        if (this->rotations.size() > 1) {
            int key1 = std::min(int(round(anim_time)),  sz(this->rotations) - 1);
            int key2 = std::min(key1 + 1,  sz(this->rotations) - 1);
            interR(R, 
                this->rotations[key1].mValue,
                this->rotations[key2].mValue,
                anim_time - key1);
        }
        if (this->positions.size() > 1) {
            int key1 = std::min(int(round(anim_time)),  sz(this->positions) - 1);
            int key2 = std::min(key1 + 1,  sz(this->positions) - 1);
            interT(T,
                this->positions[key1].mValue,
                this->positions[key2].mValue,
                anim_time - key1);
        }
        if (this->scalings.size() > 1) {
            int key1 = std::min(int(round(anim_time)),  sz(this->scalings) - 1);
            int key2 = std::min(key1 + 1,  sz(this->scalings) - 1);
            interS(S,
                this->scalings[key1].mValue,
                this->scalings[key2].mValue,
                anim_time - key1);
        }
        mat4 m = mat4();
        m = translate(m, vec3(T.x, T.y, T.z));
        m = m * mat4_cast(quat(R.w, R.x, R.y, R.z));
        m = scale(m, vec3(S.x, S.y, S.z) / vec3(100.0));
        return m;
    }
};

class Scene {
public:
    mat4 model;
    const aiScene* scene;
    vector<Mesh> meshes;
    vector<Material> materials;
    vector<Animation> animations;
    Light light;

    Scene() {;}
    Scene(string path);
    ~Scene();
    void moveToGPU();
};

Scene::Scene(string path) {
    this->model = mat4();
    Assimp::Importer importer;
    this->scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Quality);
    this->meshes.clear();
    this->materials.clear();
    
    // Materials
    for (int i = 0; i < scene->mNumMaterials; i++) {
        const aiMaterial* src = this->scene->mMaterials[i];
        Material dst;
        aiColor3D color;

        if (src->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
            dst.ambient = vec3(color.r, color.g, color.b);
        }
        if (src->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            dst.diffuse = vec3(color.r, color.g, color.b);
        }
        if (src->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            dst.specular = vec3(color.r, color.g, color.b);
        }
        src->Get(AI_MATKEY_SHININESS, dst.shininess);

        this->materials.push_back(dst);

        cout << "Material: " << i << endl;
        cout << "\t" << "Ambient: " << to_string(dst.ambient) << endl;
        cout << "\t" << "Diffuse: " << to_string(dst.diffuse) << endl;
        cout << "\t" << "Specular:" << to_string(dst.specular) << endl;
    }

    // Animation
    map<string, int> animation_id;
    for (int i = 0; i < this->scene->mNumAnimations; i++) {
        const auto anim = this->scene->mAnimations[i];
        for (int j = 0; j < anim->mNumChannels; j++) {
            const auto node_anim = anim->mChannels[j];
            const auto node_name = string(node_anim->mNodeName.C_Str());

            Animation dst;
            dst.duration = anim->mDuration + 1;
            dst.rotations.resize(node_anim->mNumRotationKeys);
            dst.positions.resize(node_anim->mNumPositionKeys);
            dst.scalings.resize(node_anim->mNumScalingKeys);
            copy(node_anim->mRotationKeys, 
                node_anim->mRotationKeys + node_anim->mNumRotationKeys,
                dst.rotations.begin());
            copy(node_anim->mPositionKeys, 
                node_anim->mPositionKeys + node_anim->mNumPositionKeys,
                dst.positions.begin());
            copy(node_anim->mScalingKeys, 
                node_anim->mScalingKeys + node_anim->mNumScalingKeys,
                dst.scalings.begin());
            
            this->animations.push_back(dst);
            animation_id[node_name] = animation_id.size();

            cout << "Animation: " << i << " on " << node_name << endl;
            cout << "\t" << "duration: " << dst.duration << endl;
            cout << "\t" << "n_rotation_key: " << dst.rotations.size() << endl;
            cout << "\t" << "n_position_key: " << dst.positions.size() << endl;
            cout << "\t" << "n_scaling_key: " << dst.scalings.size() << endl;
        }
    }

    // Mesh
    for (int i = 0; i < this->scene->mNumMeshes; i++) {
        const aiMesh* src = this->scene->mMeshes[i];
        Mesh dst;

        for (int v = 0; v < src->mNumVertices; v++) {
            dst.xyz.push_back(src->mVertices[v][0]);
            dst.xyz.push_back(src->mVertices[v][1]);
            dst.xyz.push_back(src->mVertices[v][2]);
            dst.nml.push_back(src->mNormals[v][0]);
            dst.nml.push_back(src->mNormals[v][1]);
            dst.nml.push_back(src->mNormals[v][2]);
        }
        for (int f = 0; f < src->mNumFaces; f++) {
            dst.indices.push_back(src->mFaces[f].mIndices[0]);
            dst.indices.push_back(src->mFaces[f].mIndices[1]);
            dst.indices.push_back(src->mFaces[f].mIndices[2]);
        }

        dst.name = string(src->mName.C_Str());
        dst.index_count = dst.indices.size();
        dst.material_id = src->mMaterialIndex;
        if (animation_id.find(dst.name) != animation_id.end()) {
            dst.animation_id = animation_id[dst.name];
        }
        this->meshes.push_back(dst);

        cout << "Mesh: " << dst.name << endl;
        cout << "\t" << "n_xyz: " << dst.xyz.size() << endl;
        cout << "\t" << "n_nml: " << dst.nml.size() << endl;
        cout << "\t" << "n_idx: " << dst.indices.size() << endl;
        cout << "\t" << "material id: " << dst.material_id << endl;
        cout << "\t" << "animation id: " << dst.animation_id << endl;
    }

    // Light
    // cout << (this->scene->mNumLights) << endl;
    // aiLight* src = this->scene->mLights[0];
    // this->light.position = vec3(src->mPosition.x, src->mPosition.y, src->mPosition.z);
    // this->light.direction = vec3(src->mDirection.x, src->mDirection.y, src->mDirection.z);
    // cout << "Light: " << src->mName.C_Str() << " type " << src->mType << endl;
    // cout << "\t" << "pos: " << to_string(this->light.position) << endl;
    // cout << "\t" << "dir: " << to_string(this->light.direction) << endl;
}

Scene::~Scene() {
    // aiReleaseImport(this->scene);
}

void Scene::moveToGPU() {
    for (Mesh& mesh : this->meshes) {
        glGenVertexArrays(1, &mesh.vao);
        glBindVertexArray(mesh.vao);

        glGenBuffers(1, &mesh.vbo_xyz);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_xyz);
        glBufferData(GL_ARRAY_BUFFER, mesh.xyz.size() * sizeof(float), &mesh.xyz[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glGenBuffers(1, &mesh.vbo_nml);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_nml);
        glBufferData(GL_ARRAY_BUFFER, mesh.nml.size() * sizeof(float), &mesh.nml[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &mesh.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);

    }
}

#endif