#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexTypes.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"

#include "Camera.h"

//#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
		case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
		case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
			#ifdef LOG_GL_NOTIFICATIONS
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
			#endif
		default: break;
	}
}

// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "INFR-1350U";

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);
	
	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////
////////////////// NEW IN WEEK 7 /////////////////////
//////////////////////////////////////////////////////

glm::mat4 MAT4_IDENTITY = glm::mat4(1.0f);
glm::mat3 MAT3_IDENTITY = glm::mat3(1.0f);

// Helper structure for material parameters
// to our shader
struct MaterialInfo {
	Texture2D::Sptr Texture;
	float           Shininess;
};

// Helper structure to represent an object 
// with a transform, mesh, and material
struct RenderObject {
	std::string             Name;
	glm::mat4               Transform;
	VertexArrayObject::Sptr Mesh;
	MaterialInfo*           Material;

	// Position of the object
	glm::vec3 Position;
	// Rotation of the object in Euler angler
	glm::vec3 Rotation;
	// The scale of the object
	glm::vec3 Scale = glm::vec3(1.0f);

	// Recalculates the Transform from the parameters (pos, rot, scale)
	void RecalcTransform() {
		Rotation = glm::fmod(Rotation, glm::vec3(360.0f)); // Wrap all our angles into the 0-360 degree range
		Transform = glm::translate(MAT4_IDENTITY, Position) * glm::mat4_cast(glm::quat(glm::radians(Rotation))) * glm::scale(MAT4_IDENTITY, Scale);
	}
};

// Helper structure for our light data
struct Light {
	glm::vec3 Position;
	glm::vec3 Color;
	// Basically inverse of how far our light goes (1/dist, approx)
	float Attenuation = 1.0f / 5.0f;
	float Range = 4.0f;
};

/// <summary>
/// Handles setting the shader uniforms for our light structure in our array of lights
/// </summary>
/// <param name="shader">The pointer to the shader</param>
/// <param name="uniformName">The name of the uniform (ex: u_Lights)</param>
/// <param name="index">The index of the light to set</param>
/// <param name="light">The light data to copy over</param>
void SetShaderLight(const Shader::Sptr& shader, const std::string& uniformName, int index, const Light& light) {
	std::stringstream stream;
	stream << uniformName << "[" << index << "]";
	std::string name = stream.str();

	// Set the shader uniforms for the light
	shader->SetUniform(name + ".Position", light.Position);
	shader->SetUniform(name + ".Color",    light.Color);
	shader->SetUniform(name + ".Attenuation",  light.Attenuation);
}

/// <summary>
/// Applies a material to our shader, setting relevent uniforms and binding 
/// textures
/// </summary>
/// <param name="shader">The shader to use</param>
/// <param name="material">The material info to use</param>
void ApplyMaterial(const Shader::Sptr& shader, MaterialInfo* material) {
	if (material != nullptr) {
		// Material properties
		shader->SetUniform("u_Material.Shininess", material->Shininess);
		// For textures, we pass the *slot* that the texture sure draw from
		shader->SetUniform("u_Material.Diffuse", 0);
		// Bind the texture
		if (material->Texture != nullptr) {
			material->Texture->Bind(0);
		}
	}
}

/// <summary>
/// Creates our little color square mesh
/// </summary>
VertexArrayObject::Sptr CreateSquare() {
	static const float interleaved[] = {
		// X      Y    Z       R     G     B        U     V      Nx     Ny    Nz   
		 0.5f, -0.5f, 0.5f,   0.0f, 0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		 0.5f,  0.5f, 0.5f,   0.3f, 0.2f, 0.5f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f, 0.5f,   1.0f, 1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f,   1.0f, 1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f
	};
	VertexBuffer::Sptr interleaved_vbo = VertexBuffer::Create();
	interleaved_vbo->LoadData(interleaved, 11 * 4);

	static const uint16_t indices[] = {
		3, 0, 1,
		3, 1, 2
	};
	IndexBuffer::Sptr interleaved_ibo = IndexBuffer::Create();
	interleaved_ibo->LoadData(indices, 3 * 2);

	size_t stride = sizeof(float) * 11;
	VertexArrayObject::Sptr result = VertexArrayObject::Create();
	result->AddVertexBuffer(interleaved_vbo, {
		BufferAttribute(0, 3, AttributeType::Float, stride, 0, AttribUsage::Position),
		BufferAttribute(1, 3, AttributeType::Float, stride, sizeof(float) * 3, AttribUsage::Color),
		BufferAttribute(3, 2, AttributeType::Float, stride, sizeof(float) * 6, AttribUsage::Texture),
		BufferAttribute(2, 3, AttributeType::Float, stride, sizeof(float) * 8, AttribUsage::Normal),
								 });
	result->SetIndexBuffer(interleaved_ibo);

	return result;
}

/// <summary>
/// Creates the shader and sets up all the lights
/// </summary>
Shader::Sptr SetupShaderAndLights(Light* lights, int numLights) {
	// Load our shaders
	Shader::Sptr shader = Shader::Create();
	shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", ShaderPartType::Vertex);
	shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", ShaderPartType::Fragment);
	shader->Link();

	// Set up our material parameters (mainly lighting)
	shader->Bind();

	// Global light params
	shader->SetUniform("u_AmbientCol", glm::vec3(0.1f));

	// Send the lights to our shader
	shader->SetUniform("u_NumLights", numLights);
	for (int ix = 0; ix < numLights; ix++) {
		SetShaderLight(shader, "u_Lights", ix, lights[ix]);
	}

	return shader;
}

/// <summary>
/// Draws some ImGui controls for the given light
/// </summary>
/// <param name="title">The title for the light's header</param>
/// <param name="light">The light to modify</param>
/// <returns>True if the parameters have changed, false if otherwise</returns>
bool DrawLightImGui(const char* title, Light& light) {
	bool result = false;
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		result |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		result |= ImGui::ColorEdit3("Col", &light.Color.r);
		result |= ImGui::DragFloat("Range", &light.Range, 0.1f);
	}
	ImGui::PopID();
	if (result) {
		light.Attenuation = 1.0f / (light.Range + 1.0f);
	}
	return result;
}

//////////////////////////////////////////////////////
////////////////// END OF NEW ////////////////////////
//////////////////////////////////////////////////////

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Initialize our ImGui helper
	ImGuiHelper::Init(window);

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	// Create some lights for our scene
	Light lights[3];
	lights[0].Position = glm::vec3(0.0f, 1.0f, 3.0f);
	lights[0].Color = glm::vec3(0.5f, 0.0f, 0.7f);

	lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
	lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

	lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
	lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);

	// Set up our shader and lights
	Shader::Sptr shader = SetupShaderAndLights(lights, 3);

	// Load in the textures for our materials
	MaterialInfo boxMaterial;
	boxMaterial.Texture = Texture2D::LoadFromFile("textures/box-diffuse.png");
	boxMaterial.Shininess = 8.0f;

	MaterialInfo monkeyMaterial;
	monkeyMaterial.Texture = Texture2D::LoadFromFile("textures/monkey-uvMap.png");
	monkeyMaterial.Shininess = 1.0f;

	// Our render objects
	RenderObject plane = RenderObject();
	RenderObject square = RenderObject();
	RenderObject monkey1 = RenderObject();
	RenderObject monkey2 = RenderObject();

	// Create our "ground plane" mesh
	MeshBuilder<VertexPosNormTexCol> mesh;
	MeshFactory::AddPlane(mesh, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(5.0f));
	plane.Mesh = mesh.Bake();
	plane.Material = &boxMaterial;
	plane.Name = "Plane";

	// Set up our lil color square, we'll use the box material
	square.Mesh     = CreateSquare();
	square.Position = glm::vec3(0.0f, 0.0f, 2.0f);
	square.Material = &boxMaterial;
	square.Name     = "Square";

	// We can share the mesh for both monkeys
	VertexArrayObject::Sptr monkeyMesh = ObjLoader::LoadFromFile("Monkey.obj");

	monkey1.Position = glm::vec3(1.5f, 0.0f, 1.0f);
	monkey1.Mesh     = monkeyMesh;
	monkey1.Material = &monkeyMaterial;
	monkey1.Name     = "Monkey 1";

	monkey2.Position   = glm::vec3(-1.5f, 0.0f, 1.0f);
	monkey2.Mesh       = monkeyMesh;
	monkey2.Material   = &monkeyMaterial;
	monkey2.Rotation.z = 180.0f;
	monkey2.Name       = "Monkey 2";

	// Put all the objects into a vector for easy access
	std::vector<RenderObject*> renderables = {
		&plane,
		&square,
		&monkey1,
		&monkey2
	};

	// Get uniform location for the model view projection
	Camera::Sptr camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, 4, 4));
	camera->LookAt(glm::vec3(0.0f));

	// Create a mat4 to store our mvp (for now)
	glm::mat4 transform = glm::mat4(1.0f);
	glm::mat4 transform2 = glm::mat4(1.0f);
	glm::mat4 transform3 = glm::mat4(1.0f); 

	// Our high-precision timer
	double lastFrame = glfwGetTime();

	bool isRotating = true;
	bool isButtonPressed = false;

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		// Showcasing how to use the imGui library!
		bool isDebugWindowOpen = ImGui::Begin("Debugging");
		if (isDebugWindowOpen) {
			// Make a checkbox for the monkey rotation
			ImGui::Checkbox("Rotating", &isRotating);
		}

		if (glfwGetKey(window, GLFW_KEY_W)) {
			if (!isButtonPressed) {
				isRotating = !isRotating;
			}
			isButtonPressed = true;
		} else {
			isButtonPressed = false;
		}

		// Rotate our models around the z axis at 90 deg per second
		if (isRotating) {
			monkey1.Rotation += glm::vec3(0.0f, 0.0f, dt * 90.0f);
			monkey2.Rotation -= glm::vec3(0.0f, 0.0f, dt * 90.0f); 
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind our shader and upload the uniform
		shader->Bind();

		// Update our application level uniforms every frame
		shader->SetUniform("u_CamPos", camera->GetPosition());

		// Draw some ImGui stuff for the lights
		for (int ix = 0; ix < 3; ix++) {
			char buff[256];
			sprintf_s(buff, "Light %d##%d", ix, ix);
			if (DrawLightImGui(buff, lights[ix])) {
				SetShaderLight(shader, "u_Lights", ix, lights[ix]);
			}
		}

		// Split lights from the objects in ImGui
		ImGui::Separator();

		// Render all our objects
		for (int ix = 0; ix < renderables.size(); ix++) {
			RenderObject* object = renderables[ix];

			// Update the object's transform for rendering
			object->RecalcTransform();

			// Set vertex shader parameters
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * object->Transform);
			shader->SetUniformMatrix("u_Model", object->Transform);
			shader->SetUniformMatrix("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(object->Transform))));

			// Apply this object's material
			ApplyMaterial(shader, object->Material);

			// Draw the object
			object->Mesh->Draw();

			// If our debug window is open, then let's draw some info for our objects!
			if (isDebugWindowOpen) {
				// All these elements will go into the last opened window
				if (ImGui::CollapsingHeader(object->Name.c_str())) {
					ImGui::PushID(ix); // Push a new ImGui ID scope for this object
					ImGui::DragFloat3("Position", &object->Position.x, 0.01f);
					ImGui::DragFloat3("Rotation", &object->Rotation.x, 1.0f);
					ImGui::DragFloat3("Scale",    &object->Scale.x, 0.01f, 0.0f);
					ImGui::PopID(); // Pop the ImGui ID scope for the object
				}
			}
		}

		// If our debug window is open, notify that we no longer will render new
		// elements to it
		if (isDebugWindowOpen) {
			ImGui::End();
		}

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
		glfwSwapBuffers(window);
	}

	// Clean up the ImGui library
	ImGuiHelper::Cleanup();

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}