#include "../Externals/Include/Include.h"
#define sz(x) (int(x.size()))
//#include "Camera.hpp"


#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_BLINN 4
#define MENU_ENVIRONMENT 5
#define MENU_BLINN_ENVIRONMENT 6
#define Shader_None 8
#define Shader_Bloom_Effect 10


int shader_index = -1;
int effect_index = -1;

#define _CRT_SECURE_NO_WARNINGS
GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
const GLfloat Pi = 3.1415926536f;
GLfloat tick;

using namespace glm;
using namespace std;

vec3 temp = vec3();
mat4 view;
mat4 projection;
GLint um4p;
GLint um4mv;
GLint ambient;
GLint diffuse;
GLint specular;
GLint shininess;
GLint light_pos;
GLint light_pos_star;
GLint light_pos_sun;
GLint effect_pos;

const aiScene *scene;
GLint Shader_Loc;
GLint Time;

//track ball

vec3 eye = vec3(0.0f, 1.0f, 0.0f);
bool firstMouse = true;
GLfloat leftRight = -90.0f;
GLfloat upDown = 0.0f;
GLfloat lastX = 0.0f;
GLfloat lastY = 0.0f;
vec3 cameraPosition(0.0f, 0.0f, 10.0f);
vec3 cameraDirection(0.0f, 0.0f, -1.0f);
vec3 cameraNormal(0.0f, 1.0f, 0.0f);

GLuint program;

//skybox
GLuint skybox_prog;
GLuint tex_envmap;
GLuint skybox_vao;

struct
{
	struct
	{
		GLint inv_vp_matrix;
		GLint eye;
	} skybox;
} uniforms;
//sk

GLuint			program2;
GLuint          window_vao;
GLuint			vertex_shader;
GLuint			fragment_shader;
GLuint			window_buffer;

// FBO parameter
GLuint			FBO;
GLuint			depthRBO;
GLuint			FBODataTexture;
//GLuint			FBODataTexture2;

typedef struct Shape
{
	string name;
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
	int animation_id;

}Shape;

typedef struct Material
{
	GLuint diffuse_tex;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
}Material;

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
			int key1 = std::min(int(round(anim_time)), sz(this->rotations) - 1);
			int key2 = std::min(key1 + 1, sz(this->rotations) - 1);
			interR(R,
				this->rotations[key1].mValue,
				this->rotations[key2].mValue,
				anim_time - key1);
		}
		if (this->positions.size() > 1) {
			int key1 = std::min(int(round(anim_time)), sz(this->positions) - 1);
			int key2 = std::min(key1 + 1, sz(this->positions) - 1);
			interT(T,
				this->positions[key1].mValue,
				this->positions[key2].mValue,
				anim_time - key1);
		}
		if (this->scalings.size() > 1) {
			int key1 = std::min(int(round(anim_time)), sz(this->scalings) - 1);
			int key2 = std::min(key1 + 1, sz(this->scalings) - 1);
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


vector<Shape> sp_shapes;
vector<Material> sp_materials;
vector<Animation> animations;
vector<const GLchar*> faces;

void My_Reshape(int width, int height);
static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

//UTIL
string to_string(const vec3& vec) {
	ostringstream ss;
	ss << " (";
	ss << fixed << setprecision(2) << vec.x << ", ";
	ss << fixed << setprecision(2) << vec.y << ", ";
	ss << fixed << setprecision(2) << vec.z;
	ss << ") ";
	return ss.str();
}

string to_string(const mat4& mat) {
	ostringstream ss;
	for (int i = 0; i < 4; i++) {
		ss << ((i == 0) ? "[[" : " [");
		for (int j = 0; j < 4; j++) {
			ss << setw(7) << fixed << setprecision(2) << mat[i][j] << ",";
		}
		ss << ((i == 3) ? "]]" : "]");
		ss << endl;
	}
	return ss.str();
}
//------------

char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

void freeShaderSource(char** srcp)
{
    delete[] srcp[0];
    delete[] srcp;
}

// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
    _TextureData(void) :
        width(0),
        height(0),
        data(0)
    {
    }

    int width;
    int height;
    unsigned char* data;
} TextureData;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath)
{
    TextureData texture;
    int components;

    // load the texture with stb image, force RGBA (4 components required)
    stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);

    // is the image successfully loaded?
    if (data != NULL)
    {
        // copy the raw data
        size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
        texture.data = new unsigned char[dataSize];
        memcpy(texture.data, data, dataSize);

        // mirror the image vertically to comply with OpenGL convention
        for ( size_t i = 0; i < texture.width; ++i)
        {
            for (size_t j = 0; j < texture.height / 2; ++j)
            {
                for (size_t k = 0; k < 4; ++k)
                {
                    size_t coord1 = (j * texture.width + i) * 4 + k;
                    size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
                    std::swap(texture.data[coord1], texture.data[coord2]);
                }
            }
        }

        // release the loaded image
        stbi_image_free(data);
    }

    return texture;
}

void My_LoadModels()
{
	//Material
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
		aiMaterial *material = scene->mMaterials[i];
		Material mtr;
		aiString texturePath;

		//add(
		aiColor3D color;
		// Ambient color
		if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
			cout << "\t" << "Has Ambient" << endl;
			mtr.ambient = vec3(color.r, color.g, color.b);
		}
		// Diffuse color
		if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			cout << "\t" << "Has Diffuse" << endl;
			mtr.diffuse = vec3(color.r, color.g, color.b);
		}
		// Specular color
		if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
			cout << "\t" << "Has Specular" << endl;
			mtr.specular = vec3(color.r, color.g, color.b);
		}
		material->Get(AI_MATKEY_SHININESS, mtr.shininess);

		//add)
		///*
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS) {
			// load width, height and data from texturePath.C_Str();
			int width, height;
			TextureData TD = loadPNG(texturePath.C_Str());
			width = TD.width;
			height = TD.height;
			unsigned char *data = TD.data;

			glGenTextures(1, &mtr.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, mtr.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

		}
		//*/
		// save materialÅc
		sp_materials.push_back(mtr);
	}

	// Animation
	map<string, int> animation_id;
	for (int i = 0; i < scene->mNumAnimations; i++) {
		const auto anim = scene->mAnimations[i];
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

			animations.push_back(dst);
			animation_id[node_name] = animation_id.size();

			cout << "Animation: " << i << " on " << node_name << endl;
			cout << "\t" << "duration: " << dst.duration << endl;
			cout << "\t" << "n_rotation_key: " << dst.rotations.size() << endl;
			cout << "\t" << "n_position_key: " << dst.positions.size() << endl;
			cout << "\t" << "n_scaling_key: " << dst.scalings.size() << endl;
		}
	}

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh *mesh = scene->mMeshes[i];
		Shape shape;
		glGenVertexArrays(1, &shape.vao);
		glBindVertexArray(shape.vao);
		//create 3 vbos to hold data
		float *vertices = new float[mesh->mNumVertices * 3];
		float *texCoords = new float[mesh->mNumVertices * 2];
		float *normals = new float[mesh->mNumVertices * 3];
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
			vertices[v * 3] = mesh->mVertices[v][0];
			vertices[v * 3 + 1] = mesh->mVertices[v][1];
			vertices[v * 3 + 2] = mesh->mVertices[v][2];
			//texCoords[v * 2] = mesh->mTextureCoords[0][v][0];
			//texCoords[v * 2 + 1] = mesh->mTextureCoords[0][v][1];
			normals[v * 3] = mesh->mNormals[v][0];
			normals[v * 3 + 1] = mesh->mNormals[v][1];
			normals[v * 3 + 2] = mesh->mNormals[v][2];
		
		}
		// create 1 ibo to hold data
		unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
			indices[f * 3] = mesh->mFaces[f].mIndices[0];
			indices[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
			indices[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
		}

		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces * 3;

		//add----
		shape.name = string(mesh->mName.C_Str());

		if (animation_id.find(shape.name) != animation_id.end()) {
			shape.animation_id = animation_id[shape.name];
		}
		
		//add----

		glGenBuffers(1, &shape.vbo_position);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 3, vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		//glGenBuffers(1, &shape.vbo_texcoord);
		//glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		//glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 2, texCoords, GL_STATIC_DRAW);
		//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &shape.vbo_normal);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 3, normals, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		//glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		delete[] vertices;
		delete[] texCoords;
		delete[] normals;

		glGenBuffers(1, &shape.ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mNumFaces * sizeof(unsigned int) * 3, indices , GL_STATIC_DRAW);
		
		delete[] indices;


		//save shape
		sp_shapes.push_back(shape);
	}
	aiReleaseImport(scene);
}


#define SHADOW_MAP_SIZE 4096

int flag_for_switch = 3;


struct Program {
	GLuint prog;
	GLuint mv;
	GLuint mvp;
	GLuint p;
	GLuint tex;
	GLuint flag;
	GLuint shadow_tex;
	GLuint shadow_matrix;
	GLuint test;
	GLuint off_shadow;
	GLuint f1;
	GLuint f2;
	GLuint f3;
	GLuint f4;
	GLuint time;
	GLuint m_x;
	GLuint m_y;
};

struct Frame {
	GLuint fbo;
	GLuint rbo;
	GLuint tex;
};

const int WIDTH = 1440;
const int HEIGHT = 900;

mat4 my_projection;
mat4 my_view;

float rot_angle = 0.0;


typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;

	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape2;

vector<Shape2> m_shpae;
vector<Shape2> quad;

void Piyou_LoadModels(vector<Shape2> &curr, string path)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;

	bool ret = tinyobj::LoadObj(shapes, materials, err, path.c_str());
	if (err.size() > 0)
	{
		printf("Load Models Fail! Please check the solution path");
		return;
	}

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());
	TextureData tdata = loadPNG("as_oilrig/Amadeus maps/as_oilrig_texture_8.png");

	for (int i = 0; i < shapes.size(); i++)
	{
		Shape2 tmp;
		glGenVertexArrays(1, &tmp.vao);
		glBindVertexArray(tmp.vao);

		glGenBuffers(3, &tmp.vbo);
		glGenBuffers(1, &tmp.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp.vbo);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float) + shapes[i].mesh.normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, 0, shapes[i].mesh.positions.size() * sizeof(float), &shapes[i].mesh.positions[0]);
		glBufferSubData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.normals.size() * sizeof(float), &shapes[i].mesh.normals[0]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)(shapes[i].mesh.positions.size() * sizeof(float)));

		glBindBuffer(GL_ARRAY_BUFFER, tmp.p_normal);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.normals.size() * sizeof(float), shapes[i].mesh.normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, tmp.vboTex);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(float), shapes[i].mesh.texcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);
		tmp.materialId = shapes[i].mesh.material_ids[0];
		tmp.indexCount = shapes[i].mesh.indices.size();


		glGenTextures(1, &tmp.m_texture);
		glBindTexture(GL_TEXTURE_2D, tmp.m_texture);

		printf("%d %d\n", tdata.width, tdata.height);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		curr.push_back(tmp);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

	}

}


GLuint noise;

void Load_noise() {
	glGenTextures(1, &noise);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, noise);
	TextureData tdata = loadPNG("noise.png");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}


Program star_p;

void Create_star_program() {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("star_vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("star_fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	shaderLog(vertexShader);
	shaderLog(fragmentShader);


	star_p.prog = glCreateProgram();
	glAttachShader(star_p.prog, vertexShader);
	glAttachShader(star_p.prog, fragmentShader);
	glLinkProgram(star_p.prog);
	glUseProgram(star_p.prog);

	star_p.mv = glGetUniformLocation(star_p.prog, "um4mv");
	star_p.p = glGetUniformLocation(star_p.prog, "um4p");
	star_p.tex = glGetUniformLocation(star_p.prog, "tex");
	star_p.flag = glGetUniformLocation(star_p.prog, "flag");
	star_p.shadow_matrix = glGetUniformLocation(star_p.prog, "shadow_matrix");
	star_p.shadow_tex = glGetUniformLocation(star_p.prog, "shadow_tex");
	star_p.test = glGetUniformLocation(star_p.prog, "test_tex");
	star_p.off_shadow = glGetUniformLocation(star_p.prog, "off_shadow");
	star_p.time = glGetUniformLocation(star_p.prog, "iTime");
	light_pos_star = glGetUniformLocation(star_p.prog, "light_pos");
}

Program sun_p;

void Create_sun_program() {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("sun_vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("sun_fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	shaderLog(vertexShader);
	shaderLog(fragmentShader);

	sun_p.prog = glCreateProgram();
	glAttachShader(sun_p.prog, vertexShader);
	glAttachShader(sun_p.prog, fragmentShader);
	glLinkProgram(sun_p.prog);
	glUseProgram(sun_p.prog);

	sun_p.mv = glGetUniformLocation(sun_p.prog, "um4mv");
	sun_p.p = glGetUniformLocation(sun_p.prog, "um4p");
	sun_p.tex = glGetUniformLocation(sun_p.prog, "iChannel0");
	sun_p.flag = glGetUniformLocation(sun_p.prog, "flag");
	sun_p.shadow_matrix = glGetUniformLocation(sun_p.prog, "shadow_matrix");
	sun_p.shadow_tex = glGetUniformLocation(sun_p.prog, "shadow_tex");
	sun_p.test = glGetUniformLocation(sun_p.prog, "test_tex");
	sun_p.off_shadow = glGetUniformLocation(sun_p.prog, "off_shadow");
	sun_p.time = glGetUniformLocation(sun_p.prog, "iTime");
	sun_p.m_x = glGetUniformLocation(sun_p.prog, "m_x");
	sun_p.m_y = glGetUniformLocation(sun_p.prog, "m_y");
	light_pos_sun = glGetUniformLocation(sun_p.prog, "light_pos");
}


Frame F1;

void Create_F1_frame() {
	glGenFramebuffers(1, &F1.fbo);

	glDeleteRenderbuffers(1, &F1.rbo);
	glDeleteTextures(1, &F1.tex);
	glGenRenderbuffers(1, &F1.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, F1.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, WIDTH, HEIGHT);

	glGenTextures(1, &F1.tex);
	glBindTexture(GL_TEXTURE_2D, F1.tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, F1.fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, F1.rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, F1.tex, 0);
}

Frame F2;

void Create_F2_frame() {
	glGenFramebuffers(1, &F2.fbo);

	glDeleteRenderbuffers(1, &F2.rbo);
	glDeleteTextures(1, &F2.tex);
	glGenRenderbuffers(1, &F2.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, F2.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, WIDTH, HEIGHT);

	glGenTextures(1, &F2.tex);
	glBindTexture(GL_TEXTURE_2D, F2.tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, F2.fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, F2.rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, F2.tex, 0);
}

Frame F3;

void Create_F3_frame() {
	glGenFramebuffers(1, &F3.fbo);

	glDeleteRenderbuffers(1, &F3.rbo);
	glDeleteTextures(1, &F3.tex);
	glGenRenderbuffers(1, &F3.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, F3.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, WIDTH, HEIGHT);

	glGenTextures(1, &F3.tex);
	glBindTexture(GL_TEXTURE_2D, F3.tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, F3.fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, F3.rbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, F3.tex, 0);
}

Program FB_p;

void Create_FB_program() {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("FB_vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("FB_fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	shaderLog(vertexShader);
	shaderLog(fragmentShader);

	FB_p.prog = glCreateProgram();
	glAttachShader(FB_p.prog, vertexShader);
	glAttachShader(FB_p.prog, fragmentShader);
	glLinkProgram(FB_p.prog);
	glUseProgram(FB_p.prog);

	FB_p.f1 = glGetUniformLocation(FB_p.prog, "tex1");
	FB_p.f2 = glGetUniformLocation(FB_p.prog, "tex2");
	FB_p.f3 = glGetUniformLocation(FB_p.prog, "tex3");
	FB_p.f4 = glGetUniformLocation(FB_p.prog, "tex4");
	FB_p.flag = glGetUniformLocation(FB_p.prog, "flag");
	Shader_Loc = glGetUniformLocation(FB_p.prog, "shader_index");

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}


GLuint Piyou_window_vao;
GLuint Piyou_window_vbo;

void Load_window_vertex() {
	glGenVertexArrays(1, &Piyou_window_vao);
	glBindVertexArray(Piyou_window_vao);

	glGenBuffers(1, &Piyou_window_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, Piyou_window_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_positions), window_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void Create_skybox_prog() {
	//-------------skybox-----Start---------
	skybox_prog = glCreateProgram();
	GLuint skybox_frag = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint skybox_ver = glCreateShader(GL_VERTEX_SHADER);
	char** skyboxVertexSource = loadShaderSource("skybox.vs.glsl");
	char** skyboxFragmentSource = loadShaderSource("skybox.fs.glsl");
	glShaderSource(skybox_ver, 1, skyboxVertexSource, NULL);
	glShaderSource(skybox_frag, 1, skyboxFragmentSource, NULL);
	freeShaderSource(skyboxVertexSource);
	freeShaderSource(skyboxFragmentSource);
	glCompileShader(skybox_ver);
	glCompileShader(skybox_frag);
	glAttachShader(skybox_prog, skybox_ver);
	glAttachShader(skybox_prog, skybox_frag);
	shaderLog(skybox_ver);
	shaderLog(skybox_frag);
	glLinkProgram(skybox_prog);
	glUseProgram(skybox_prog);

	uniforms.skybox.inv_vp_matrix = glGetUniformLocation(skybox_prog, "inv_vp_matrix");
	uniforms.skybox.eye = glGetUniformLocation(skybox_prog, "eye");

	faces.push_back("star_right.png");
	faces.push_back("star_left.png");
	faces.push_back("star_top.png");
	faces.push_back("star_bottom.png");
	faces.push_back("star_back.png");
	faces.push_back("star_front.png");

	TextureData envmap_data;
	glGenTextures(1, &tex_envmap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);
	for (int i = 0; i < 6; ++i)
	{
		envmap_data = loadPNG(faces[i]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			GL_RGBA,
			envmap_data.width,
			envmap_data.height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			envmap_data.data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	delete[] envmap_data.data;

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glGenVertexArrays(1, &skybox_vao);
	//-------------skybox---End-----------
}

void Create_model_prog() {
	scene = aiImportFile("test2.fbx", aiProcessPreset_TargetRealtime_MaxQuality);
	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	shaderLog(vertexShader);
	shaderLog(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");
	ambient = glGetUniformLocation(program, "ambient");
	diffuse = glGetUniformLocation(program, "diffuse");
	specular = glGetUniformLocation(program, "specular");
	shininess = glGetUniformLocation(program, "shininess");
	light_pos = glGetUniformLocation(program, "light_pos");
	effect_pos = glGetUniformLocation(program, "effect_index");

	glUseProgram(program);

	My_LoadModels();
}

void Create_FBO_Frame() {
	glGenFramebuffers(1, &FBO);

	// If the windows is reshaped, we need to reset some settings of framebuffer
	glDeleteRenderbuffers(1, &depthRBO);
	glDeleteTextures(1, &FBODataTexture);
	glGenRenderbuffers(1, &depthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, WIDTH, HEIGHT);

	// (1) Generate a texture for FBO
	// (2) Bind it so that we can specify the format of the textrue
	glGenTextures(1, &FBODataTexture);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// (1) Bind the framebuffer with first parameter "GL_DRAW_FRAMEBUFFER" 
	// (2) Attach a renderbuffer object to a framebuffer object
	// (3) Attach a texture image to a framebuffer object
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0);
}

void My_Init()
{
    glClearColor(1.0f, 1.6f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	Piyou_LoadModels(m_shpae, "nanosuit.obj");
	Piyou_LoadModels(quad, "quad.obj");
	Load_noise();

	Create_star_program();
	Create_sun_program();
	Create_FB_program();
	
	Load_window_vertex();

	Create_F1_frame();
	Create_F2_frame();
	Create_F3_frame();
	Create_FBO_Frame();

	Create_skybox_prog();
	Create_model_prog();

	//Create_FBO_prog();
	//Load_window_vertex2();

	My_Reshape(1440, 900);	
	
}


void draw_starfile() {

	mat4 view2 = lookAt(vec3(1.0f, 1.0f, 0.0f), vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	mat4 quad_matrix = scale(mat4(1.0f), vec3(5));

	glUseProgram(star_p.prog);
	glUniformMatrix4fv(star_p.p, 1, GL_FALSE, value_ptr(my_projection));
	glUniformMatrix4fv(star_p.mv, 1, GL_FALSE, value_ptr(view2 * quad_matrix));
	glUniform1f(star_p.time, (float)glutGet(GLUT_ELAPSED_TIME) * 0.001);
	/*
	//light-------------------
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	vec3 light_position = vec3(40.0f * sinf(currentTime), 30.0f + 20.0f * cosf(currentTime), 40.0f);
	glUniform3fv(light_pos_star, 1, &light_position[0]);
	//--------------------
	*/
	for (int i = 0; i < quad.size(); i++) {
		glBindVertexArray(quad[i].vao);
		glDrawElements(GL_TRIANGLES, quad[i].indexCount, GL_UNSIGNED_INT, 0);
	}

}

void draw_sun_light() {

	mat4 view2 = lookAt(vec3(1.0f, 1.0f, 0.0f), vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	mat4 quad_matrix = scale(mat4(1.0f), vec3(5));

	glUseProgram(sun_p.prog);
	glUniformMatrix4fv(sun_p.p, 1, GL_FALSE, value_ptr(my_projection));
	glUniformMatrix4fv(sun_p.mv, 1, GL_FALSE, value_ptr(view2 * quad_matrix));
	glUniform1f(sun_p.time, (float)glutGet(GLUT_ELAPSED_TIME) * 0.001);
	/*
	//light-------------------
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	vec3 light_position = vec3(40.0f * sinf(currentTime), 30.0f + 20.0f * cosf(currentTime), 40.0f);
	glUniform3fv(light_pos_sun, 1, &light_position[0]);
	//--------------------
	*/
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, noise);

	glUniform1i(sun_p.flag, flag_for_switch);

	for (int i = 0; i < quad.size(); i++) {
		glBindVertexArray(quad[i].vao);
		glDrawElements(GL_TRIANGLES, quad[i].indexCount, GL_UNSIGNED_INT, 0);
	}

}

void draw_F1() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, F1.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glViewport(0, 0, 1440, 900);

	draw_starfile();
}

void draw_F2() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, F2.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glViewport(0, 0, 1440, 900);

	draw_sun_light();
}


void Draw_FBO() {
	// (1) Bind the framebuffer object correctly
	// (2) Draw the buffer with color
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	// TODO :
	// (1) Clear the color buffer (GL_COLOR) with the color of white
	// (2) Clear the depth buffer (GL_DEPTH) with value one 
	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glViewport(0, 0, 1440, 900);
}

void draw_result() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glViewport(0, 0, 1440, 900);

	glUseProgram(FB_p.prog);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, F1.tex);
	glUniform1i(FB_p.f1, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, F2.tex);
	glUniform1i(FB_p.f2, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, F3.tex);
	glUniform1i(FB_p.f3, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, FBODataTexture);
	glUniform1i(FB_p.f4, 3);

	glUniform1i(FB_p.flag, flag_for_switch);

	glUniform1i(Shader_Loc, shader_index);

	glBindVertexArray(Piyou_window_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

}


void Draw_skybox() {
	//-------------skybox---Start-----------

	mat4 view_matrix = lookAt(eye, vec3(-1.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 inv_vp_matrix = inverse(projection * view_matrix);

	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);

	glUseProgram(skybox_prog);
	glBindVertexArray(skybox_vao);

	glUniformMatrix4fv(uniforms.skybox.inv_vp_matrix, 1, GL_FALSE, &inv_vp_matrix[0][0]);
	glUniform3fv(uniforms.skybox.eye, 1, &eye[0]);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

	//-------------skybox---End-----------
}

void Draw_model() {
	// Draw the Scene
	glUseProgram(program);

	//object translation
	mat4 Identy_Init(1.0);
	mat4 mv_matrix = translate(Identy_Init, temp);

	view = lookAt(cameraPosition, cameraPosition + cameraDirection, cameraNormal);

	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	glUniform1i(effect_pos, effect_index);
	///*
	//light-------------------
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	vec3 light_position = vec3(50.0f * sinf(currentTime),
		50.0f * cosf(currentTime),
		50.0f * sinf(currentTime) + 50.0f * cosf(currentTime));
	// glUniform3fv(light_pos, 1, &light_position[0]);

	//--------------------
	//*/

	float cur_time = glutGet(GLUT_ELAPSED_TIME);
	for (unsigned int i = 0; i < sp_shapes.size(); ++i) {
		glBindVertexArray(sp_shapes[i].vao);
		int materialID = sp_shapes[i].materialID;
		glBindTexture(GL_TEXTURE_2D, sp_materials[materialID].diffuse_tex);

		//add(
		mat4 model = mat4();
		if (sp_shapes[materialID].animation_id != -1) {
			const Animation& anim = animations[sp_shapes[materialID].animation_id];
			mat4 local_model = anim.animate(cur_time);
			model = model * local_model;
		}
		glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));

		const Material& mtl = sp_materials[materialID];
		glUniform3fv(ambient, 1, &mtl.ambient[0]);
		glUniform3fv(diffuse, 1, &mtl.diffuse[0]);
		glUniform3fv(specular, 1, &mtl.specular[0]);
		glUniform1f(shininess, mtl.shininess);
		//add)

		glDrawElements(GL_TRIANGLES, sp_shapes[i].drawCount, GL_UNSIGNED_INT, 0);
	}

}

void draw_F3() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, F3.fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glViewport(0, 0, 1440, 900);

	Draw_model();
}


void My_Display()
{

	Draw_FBO();

	glUseProgram(program);

	Draw_skybox();
	Draw_model();
	
	draw_F1();
	draw_F2();
	draw_F3();
	draw_result();

    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height; 
	projection = perspective(radians(100.0f), viewportAspect, 0.1f, 1000.0f);
	view = lookAt(vec3(-10.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	lastX = width / 2.0;
	lastY = height / 2.0;


	my_projection = perspective(radians(80.0f), viewportAspect, 0.1f, 10000.0f);
	my_view = lookAt(vec3(0, 0, 0), vec3(-1, -1, 0), vec3(0.0f, 1.0f, 0.0f));

}

void My_Timer(int val)
{
	glutPostRedisplay();
	timer_cnt++;
	if (timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if(state == GLUT_DOWN)
	{
		firstMouse = true;
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if(state == GLUT_UP)
	{
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void My_Drag(int nowX, int nowY)
{

	//printf("Mouse dragging: (%d, %d)\n", nowX, nowY);
	if (firstMouse) {
		lastX = nowX;
		lastY = nowY;
		firstMouse = false;
	}

	GLfloat xOffset = nowX - lastX;
	GLfloat yOffset = lastY - nowY;
	lastX = nowX;
	lastY = nowY;

	GLfloat sense = 0.08;
	xOffset *= sense;
	yOffset *= sense;

	printf("(%d, %d)\n", xOffset, yOffset);

	leftRight -= xOffset;
	upDown -= yOffset;


	if (upDown > 89.0f)
		upDown = 89.0f;
	if (upDown < -89.0f)
		upDown = -89.0f;


	vec3 dir;
	dir.x = cos(radians(leftRight)) * cos(radians(upDown));
	dir.y = sin(radians(upDown));
	dir.z = sin(radians(leftRight)) * cos(radians(upDown));
	cameraDirection = normalize(dir);

}


void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	GLfloat cameraSpeed = 0.1f;
	if (key == 'w') {
		cameraPosition += cameraSpeed * cameraDirection;
	}
	else if (key == 's') {
		cameraPosition -= cameraSpeed * cameraDirection;
	}
	else if (key == 'a') {
		cameraPosition -= normalize(cross(cameraDirection, cameraNormal)) * cameraSpeed;
	}
	else if (key == 'd') {
		cameraPosition += normalize(cross(cameraDirection, cameraNormal)) * cameraSpeed;
	}
	else if (key == 'z') {
		cameraPosition += cameraSpeed * cameraNormal;
	}
	else if (key == 'x') {
		cameraPosition -= cameraSpeed * cameraNormal;
	}
	else if (key == 'i') {
		flag_for_switch = (flag_for_switch + 1) % 4;
	}
	
	/*
	if (key == 'w') {
		camera_move += (center - eye) / 20.0f;
		//camera_move += vec3(0.25f, 0.0f, 0.0f);
		//camera_move += cross(vec3(1.0f, 0.0f, 0.0f), (center - eye)) / 20.0f;
	}
	else if (key == 's') {
		camera_move += (eye - center) / 20.0f;
		//camera_move += vec3(-0.25f, 0.0f, 0.0f);
		//camera_move += cross((center - eye), vec3(1.0f, 0.0f, 0.0f)) / 20.0f;
	}
	else if (key == 'd') {
		camera_move += cross((center - eye), vec3(0.0f, 1.0f, 0.0f)) / 20.0f;
	}
	else if (key == 'a') {
		camera_move += cross(vec3(0.0f, 1.0f, 0.0f), (center - eye)) / 20.0f;
	}
	else if (key == 'z') {
		camera_move += vec3(0.0f, 0.25f, 0.0f);
	}
	else if (key == 'x') {
		camera_move += vec3(0.0f, -0.25f, 0.0f);
	}
	*/
	
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);

		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case Shader_None:
		shader_index = 1;
		break;
	case Shader_Bloom_Effect:
		shader_index = 2;
		break;	
	case MENU_BLINN:
		effect_index = 1;
		break;
	case MENU_ENVIRONMENT:
		effect_index = 2;
		break;
	case MENU_BLINN_ENVIRONMENT:
		effect_index = 3;
		break;
	default:
		break;
	}
}


int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1440, 900);
	glutCreateWindow("Final_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
	int shader = glutCreateMenu(My_Menu);
	int menu_effect = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("Effect", menu_effect);

	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_effect);
	glutAddMenuEntry("BlinnPhong", MENU_BLINN);
	glutAddMenuEntry("EnvironmentMapping", MENU_ENVIRONMENT);
	glutAddMenuEntry("Blinn_Environment", MENU_BLINN_ENVIRONMENT);


	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_Drag);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
