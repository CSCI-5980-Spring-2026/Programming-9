/* Programming 9: Resource Registration and Cross-Referencing
 * CSCI 5980, Spring 2026, University of Minnesota
 * Instructor: Evan Suma Rosenberg <suma@umn.edu>
 */ 


#include <GopherEngine/Core/MainLoop.hpp>
#include <GopherEngine/Core/LightComponent.hpp>
#include <GopherEngine/Core/MeshComponent.hpp>
#include <GopherEngine/Renderer/BlinnPhongMaterial.hpp>
#include <GopherEngine/Resource/MeshFactory.hpp>
#include <GopherEngine/Core/FileLoader.hpp>
using namespace GopherEngine;

#include <memory>
#include <iostream>
using namespace std;

// A simple subclass of MainLoop to test that the main loop is working
// and the window, scene, and node classes are functioning correctly
class MainLoopTest: public MainLoop
{
	public:
		// Constructor and destructor
		MainLoopTest();
		~MainLoopTest();

	private:

		// Override the pure virtual functions from MainLoop
		void configure() override;
		void initialize() override;
		void update(float delta_time) override;

		shared_ptr<Node> cube_node_;
		shared_ptr<Node> bunny_node_;
		bool resources_loaded_{false};
};


MainLoopTest::MainLoopTest() {

}

MainLoopTest::~MainLoopTest() {

}

// This function is called once at the beginning of the main loop, before the window is created.
// This means the OpenGL context is not yet available. It should be used for initial configuration.
void MainLoopTest::configure() {

	window_.set_title("CSCI 5980 Programming 9");
	renderer_.set_background_color(glm::vec4(0.5f, 0.75f, .9f, 1.f));
}

// This function is called once at the beginning of the main loop, after the window is created
// and the OpenGL context is available. It should be used for initializing the scene.
void MainLoopTest::initialize() {
	
	// Create default camera and set its initial position
	auto camera_node = scene_->create_default_camera();
	camera_node->transform().position_ = glm::vec3(0.f, 0.f, 3.f);

	// Create nodes to the scene
	cube_node_ = scene_->create_node();
	cube_node_->transform().position_ = glm::vec3(-1.f, 0.f, 0.f);
	bunny_node_ = scene_->create_node();
	bunny_node_->transform().position_ = glm::vec3(1.f, 0.f, 0.f);

	// Add mesh components to the nodes
	auto cube_component = make_shared<MeshComponent>();
	cube_node_->add_component(cube_component);
	auto bunny_component = make_shared<MeshComponent>();
	bunny_node_->add_component(bunny_component);
	
	// Create a point light
	auto light_component = make_shared<LightComponent>(LightType::Point);
	light_component->get_light()->ambient_intensity_ = glm::vec3(0.2f, 0.2f, 0.2f);
	light_component->get_light()->diffuse_intensity_ = glm::vec3(1.f, 1.f, 1.f);
	light_component->get_light()->specular_intensity_ = glm::vec3(1.f, 1.f, 1.f);
	
	// Add the point light to the scene
	auto light_node = scene_->create_node();
	light_node->add_component(light_component);
	light_node->transform().position_ = glm::vec3(-10.f, 0.f, 0.f);

	// Add a cube to the cube mesh component
	cube_component->set_mesh(MeshFactory::create_cube());

	// Add a cube to the bunny mesh compoment (placeholder)
	bunny_component->set_mesh(MeshFactory::create_cube());

	// Because the cube mesh was procedurally generated, we can assign the texture immediately
	// after loading via a callback without worrying about load order or cross-referencing.
	ResourceManager::get().load_texture("assets/Gravel_001_BaseColor.jpg",

		// Success callback (optional)
		[cube_component](std::shared_ptr<Texture> texture) { 

			cout << "Loaded texture: " << texture->name_ << " with GUID " << texture->guid_.to_string() << endl;

			auto material = make_shared<BlinnPhongMaterial>();
			material->set_ambient_color(glm::vec3(0.2f, 0.2f, 0.2f));
			material->set_diffuse_color(glm::vec3(0.2f, 0.2f, 0.2f));
			material->set_specular_color(glm::vec3(2.f, 2.f, 2.f));
			material->set_shininess(64.f);
			material->set_texture(texture);
			cube_component->set_material(material);
		},

		// Failure callback (optional)
		[](const std::string& error) {
			cout << error << endl;
		}
		
	);

	ResourceManager::get().load_texture("assets/Bark_007_BaseColor.jpg",

		// Success callback (optional)
		[bunny_component](std::shared_ptr<Texture> texture) { 

			cout << "Loaded texture: " << texture->name_ << " with GUID " << texture->guid_.to_string() << endl;

			auto material = make_shared<BlinnPhongMaterial>();
			material->set_ambient_color(glm::vec3(0.2f, 0.2f, 0.2f));
			material->set_diffuse_color(glm::vec3(0.2f, 0.2f, 0.2f));
			material->set_specular_color(glm::vec3(2.f, 2.f, 2.f));
			material->set_shininess(64.f);
			material->set_texture(texture);
			bunny_component->set_material(material);
		},

		// Failure callback (optional)
		[](const std::string& error) {
			cout << error << endl;
		}
		
	);
}

// This function is called once per frame, before the scene's update function is called.
void MainLoopTest::update(float delta_time) {

	// Rotate the node around the Y axis at a constant speed of 45 degrees per second
    auto frame_rotation = glm::angleAxis(
        delta_time * glm::radians(45.f), 
        glm::vec3(0.f, 1.f, 0.f)
    );

	// Rotate the nodes by multiplying the new frame rotation with the existing rotation
    cube_node_->transform().rotation_ =  frame_rotation * cube_node_->transform().rotation_;
	bunny_node_->transform().rotation_ =  frame_rotation * bunny_node_->transform().rotation_;

}

int main()
{
	// Create an instance of the MainLoop subclass and start the main game loop
	MainLoopTest app;
	return app.run();
}