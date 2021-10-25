#include "Scene.h"

#include <GLFW/glfw3.h>

#include "Utils/FileHelpers.h"
#include "Utils/GlmBulletConversions.h"

#include "Gameplay/Physics/RigidBody.h"

Scene::Scene() :
	Objects(std::vector<GameObject::Sptr>()),
	Lights(std::vector<Light>()),
	IsPlaying(false),
	Camera(nullptr),
	BaseShader(nullptr),
	_filePath(""),
	_lightingUbo(nullptr),
	_ambientLight(glm::vec3(0.1f)),
	_gravity(glm::vec3(0.0f, 0.0f, -9.81f))
{
	_lightingUbo = std::make_shared<UniformBuffer<LightingUboStruct>>();
	_lightingUbo->GetData().AmbientCol = _ambientLight;
	_lightingUbo->Update();
	_lightingUbo->Bind(LIGHT_UBO_BINDING_SLOT);

	_InitPhysics();
}

Scene::~Scene() {
	Objects.clear();
	_CleanupPhysics();
}

GameObject::Sptr Scene::CreateGameObject(const std::string& name)
{
	GameObject::Sptr result(new GameObject());
	result->Name = name;
	result->_scene = this;
	Objects.push_back(result);
	return result;
}

GameObject::Sptr Scene::FindObjectByName(const std::string name) {
	auto it = std::find_if(Objects.begin(), Objects.end(), [&](const GameObject::Sptr& obj) {
		return obj->Name == name;
	});
	return it == Objects.end() ? nullptr : *it;
}

GameObject::Sptr Scene::FindObjectByGUID(Guid id) {
	auto it = std::find_if(Objects.begin(), Objects.end(), [&](const GameObject::Sptr& obj) {
		return obj->GUID == id;
	});
	return it == Objects.end() ? nullptr : *it;
}

void Scene::SetAmbientLight(const glm::vec3& value) {
	_ambientLight = value;
	_lightingUbo->GetData().AmbientCol = _ambientLight;
	_lightingUbo->Update();
}

const glm::vec3& Scene::GetAmbientLight() const { 
	return _ambientLight;
}

void Scene::Awake() {
	// Not a huge fan of this, but we need to get window size to notify our camera
	// of the current screen size
	int width, height;
	glfwGetWindowSize(Window, &width, &height);
	Camera->ResizeWindow(width, height);

	// Call awake on all gameobjects
	for (auto& obj : Objects) {
		obj->Awake();
	}
	// Set up our lighting 
	SetupShaderAndLights();
}

void Scene::DoPhysics(float dt) {
	if (IsPlaying) {
		for (auto& rb : _rigidBodies) {
			rb->PhysicsPreStep(dt);
		}

		_physicsWorld->stepSimulation(dt, 10);

		for (auto& rb : _rigidBodies) {
			rb->PhysicsPostStep(dt);
		}
	}
}

void Scene::Update(float dt) {
	if (IsPlaying) {
		for (auto& obj : Objects) {
			obj->Update(dt);
		}
	}
}

void Scene::SetShaderLight(int index, bool update /*= true*/) {

	// Make sure that the index is within range
	if (index >= 0 && index < Lights.size() && index < MAX_LIGHTS) {
		// Get a reference to the light UBO data so we can update it
		LightingUboStruct& data = _lightingUbo->GetData();
		Light& light = Lights[index];

		// Copy to the ubo data
		data.Lights[index].Position = light.Position;
		data.Lights[index].Color = light.Color;
		data.Lights[index].Attenuation = 1.0f / (1.0f + light.Range);

		// If requested, send the new data to the UBO
		if (update)	_lightingUbo->Update();
	}
}

void Scene::SetupShaderAndLights() {
	// Get a reference to the light UBO data so we can update it
	LightingUboStruct& data = _lightingUbo->GetData();
	// Send in how many active lights we have and the global lighting settings
	data.AmbientCol = glm::vec3(0.1f);
	data.NumLights = Lights.size();

	// Iterate over all lights that are enabled and configure them
	for (int ix = 0; ix < Lights.size() && ix < MAX_LIGHTS; ix++) {
		SetShaderLight(ix, false);
	}
	// Send updated data to OpenGL
	_lightingUbo->Update();
}

Scene::Sptr Scene::FromJson(const nlohmann::json& data)
{
	Scene::Sptr result = std::make_shared<Scene>();
	result->BaseShader = ResourceManager::Get<Shader>(Guid(data["default_shader"]));

	LOG_ASSERT(data["objects"].is_array(), "Objects not present in scene!");
	for (auto& object : data["objects"]) {
		result->Objects.push_back(GameObject::FromJson(object, result.get()));
	}

	LOG_ASSERT(data["lights"].is_array(), "Lights not present in scene!");
	for (auto& light : data["lights"]) {
		result->Lights.push_back(Light::FromJson(light));
	}

	// Create and load camera config
	result->Camera = Camera::Create();
	result->Camera->SetPosition(ParseJsonVec3(data["camera"]["position"]));
	result->Camera->SetForward(ParseJsonVec3(data["camera"]["normal"]));
	
	return result;
}

nlohmann::json Scene::ToJson() const
{
	nlohmann::json blob;
	// Save the default shader (really need a material class)
	blob["default_shader"] = BaseShader->GetGUID().str();

	// Save renderables
	std::vector<nlohmann::json> objects;
	objects.resize(Objects.size());
	for (int ix = 0; ix < Objects.size(); ix++) {
		objects[ix] = Objects[ix]->ToJson();
	}
	blob["objects"] = objects;

	// Save lights
	std::vector<nlohmann::json> lights;
	lights.resize(Lights.size());
	for (int ix = 0; ix < Lights.size(); ix++) {
		lights[ix] = Lights[ix].ToJson();
	}
	blob["lights"] = lights;

	// Save camera info
	blob["camera"] = {
		{"position", GlmToJson(Camera->GetPosition()) },
		{"normal",   GlmToJson(Camera->GetForward()) }
	};

	return blob;
}

void Scene::Save(const std::string& path) {
	_filePath = path;
	// Save data to file
	FileHelpers::WriteContentsToFile(path, ToJson().dump(1, '\t'));
	LOG_INFO("Saved scene to \"{}\"", path);
}

Scene::Sptr Scene::Load(const std::string& path)
{
	LOG_INFO("Loading scene from \"{}\"", path);
	std::string content = FileHelpers::ReadFile(path);
	nlohmann::json blob = nlohmann::json::parse(content);
	Scene::Sptr result = FromJson(blob);
	result->_filePath = path;
	return result;
}

int Scene::NumObjects() const {
	return Objects.size();
}

GameObject::Sptr Scene::GetObjectByIndex(int index) const {
	return Objects[index];
}

void Scene::_InitPhysics() {
	_collisionConfig = new btDefaultCollisionConfiguration();
	_collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
	_broadphaseInterface = new btDbvtBroadphase();
	_constraintSolver = new btSequentialImpulseConstraintSolver();
	_physicsWorld = new btDiscreteDynamicsWorld(
		_collisionDispatcher,
		_broadphaseInterface,
		_constraintSolver,
		_collisionConfig
	);
	_physicsWorld->setGravity(ToBt(_gravity));
	// TODO bullet debug drawing
}

void Scene::_CleanupPhysics() {
	delete _physicsWorld;
	delete _constraintSolver;
	delete _broadphaseInterface;
	delete _collisionDispatcher;
	delete _collisionConfig;
}
