#include <cassert>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Camera.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Shape.h"
#include "Material.h"
#include "Light.h"

using namespace std;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = "./"; // Where the resources are loaded from
bool OFFLINE = false;

//Initialize these in init()

shared_ptr<Camera> camera;
shared_ptr<Program> prog;
shared_ptr<Program> prog2;
shared_ptr<Program> prog3;
shared_ptr<Program> prog4;
shared_ptr<Shape> shape; //bunny
shared_ptr<Shape> shape2; //teapot

vector<shared_ptr<Program>> programs;
shared_ptr<Program> currProgram;
int progIndex = 0;

vector<Material> materials;
Material currMaterial;
int matIndex = 0;

vector<Light> lights;
Light* currLight;
int lightIndex = 0;

bool keyToggles[256] = {false}; // only for English keyboards!

// This function is called when a GLFW error occurs
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

// This function is called when a key is pressed
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}


// This function is called when the mouse is clicked
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = (mods & GLFW_MOD_SHIFT) != 0;
		bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
		bool alt   = (mods & GLFW_MOD_ALT) != 0;
		camera->mouseClicked((float)xmouse, (float)ymouse, shift, ctrl, alt); 
	}
}

// This function is called when the mouse moves
static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved((float)xmouse, (float)ymouse); // updates camera transformations
	}
}

// This function is called when a valid character is pressed
static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];

	currProgram = programs[progIndex];
	currMaterial = materials[matIndex];
	currLight = &(lights[lightIndex]);

	float translation_factor = 0.1f;

	switch (key)
	{	
		// Switch to next shader
		case 's': 
		{
			if (progIndex < programs.size() - 1) {
				progIndex++;
				currProgram = programs[progIndex]; 
			}

			break;
		}
		// Switch to previous shader
		case 'S':
		{
			if (progIndex >= 1) {
				progIndex--;
				currProgram = programs[progIndex];
			}
			break;
		}
		// Switch to next material
		case 'm':
		{
			if (matIndex < materials.size() - 1) {
				matIndex++;
				currMaterial = materials[matIndex];
			}
			break;
		}
		// Switch to previous material
		case 'M':
		{
			if (matIndex >= 1) {
				matIndex--;
				currMaterial = materials[matIndex];
			}
			break;
		}
		// Switch to next light
		case 'l':
		{
			if (lightIndex < lights.size() - 1) {
				lightIndex++;
				currLight = &lights[lightIndex];
			}
			break;
		}
		// Switch to previous light
		case 'L':
		{
			if (lightIndex >= 1) {
				lightIndex--;
				currLight = &lights[lightIndex];
			}
			break;
		}
		// Set up light translations
		case 'x':
		{
			currLight->translatePosition_X(-1.0 * translation_factor);
			break;
		}
		case 'X':
		{
			currLight->translatePosition_X(translation_factor);
			break;
		}
		case 'y':
		{
			currLight->translatePosition_Y(-1 * translation_factor);
			break;
		}
		case 'Y':
		{
			currLight->translatePosition_Y(translation_factor);
			break;
		}

		
	}
	

}

// If the window is resized, capture the new size and reset the viewport
static void resize_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
static void saveImage(const char *filepath, GLFWwindow *w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	int rc = stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	if(rc) {
		cout << "Wrote to " << filepath << endl;
	} else {
		cout << "Couldn't write to " << filepath << endl;
	}
}

// This function is called once to initialize the scene and OpenGL
static void init()
{
	// Initialize time.
	glfwSetTime(0.0);
	
	// Set background color.
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	currProgram = make_shared<Program>();

	/*
	
		Setup shader programs. 
		-Add attributes and uniform variables
		-varying variables are passed between vert and frag shader
		-Attributes are sent in from Shape.cpp (aPos/aNor)
		-Unifrom variables are passed from render()
	
	*/

	//normal coloring
	
	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "normal_vert.glsl", RESOURCE_DIR + "normal_frag.glsl");
	prog->setVerbose(true);
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addUniform("MV");
	prog->addUniform("P");
	prog->setVerbose(false);
	programs.push_back(prog);


	//Blinn-Phong Shading
	prog2 = make_shared<Program>();
	prog2->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
	prog2->setVerbose(true);
	prog2->init();
	prog2->addAttribute("aPos");
	prog2->addAttribute("aNor");
	prog2->addUniform("MV");			// modelview matrix
	prog2->addUniform("P");				// projection matrix
	prog2->addUniform("lightPos1");
	prog2->addUniform("lightPos2");
	prog2->addUniform("lightColor1");
	prog2->addUniform("lightColor2");
	prog2->addUniform("ka");			// ambient color
	prog2->addUniform("kd");			// diffuse color
	prog2->addUniform("ks");			// specular color
	prog2->addUniform("s");				// shininess factor
	prog2->setVerbose(false);
	prog2->addUniform("MVit");			// inverse transpose: used to convert normals to camera space
	programs.push_back(prog2);

	//Silhouette shader
	prog3 = make_shared<Program>();
	prog3->setShaderNames(RESOURCE_DIR + "silhouette_vert.glsl", RESOURCE_DIR + "silhouette_frag.glsl");
	prog3->setVerbose(true);
	prog3->init();
	prog3->addAttribute("aPos");
	prog3->addAttribute("aNor");
	prog3->addUniform("MV");
	prog3->addUniform("P");
	prog3->addUniform("MVit");
	prog3->setVerbose(false);
	programs.push_back(prog3);

	//Cel shader
	prog4 = make_shared<Program>();
	prog4->setShaderNames(RESOURCE_DIR + "cel_vert.glsl", RESOURCE_DIR + "cel_frag.glsl");
	prog4->setVerbose(true);
	prog4->init();
	prog4->addAttribute("aPos");
	prog4->addAttribute("aNor");
	prog4->addUniform("MV");
	prog4->addUniform("P");
	prog4->addUniform("lightPos1");
	prog4->addUniform("lightPos2");
	prog4->addUniform("lightColor1");
	prog4->addUniform("lightColor2");
	prog4->addUniform("ka");
	prog4->addUniform("kd");
	prog4->addUniform("ks");
	prog4->addUniform("s");
	prog4->addUniform("MVit");
	prog4->setVerbose(false);
	programs.push_back(prog4);

	/*
	
		Set up the materials 
	
	*/

	Material m1;
	m1.setAmbient({ 0.2f, 0.2f, 0.2f });
	m1.setDiffuse({ 0.8f, 0.7f, 0.7f });
	m1.setSpecular({ 1.0f, 0.9f, 0.8f });
	m1.setShiny(200.0f);
	materials.push_back(m1);

	Material m2;
	m2.setAmbient({ 0.2f, 0.2f, 0.2f });
	m2.setDiffuse({ 0.0f, 0.0f, 0.8f });
	m2.setSpecular({ 0.0f, 0.9f, 0.0f });
	m2.setShiny(200.0f);
	materials.push_back(m2);

	Material m3;
	m3.setAmbient({ 0.1f, 0.1f, 0.1f });
	m3.setDiffuse({ 0.2f, 0.2f, 0.2f });
	m3.setSpecular({ 0.3f, 0.3f, 0.45f });
	m3.setShiny(2.0f);
	materials.push_back(m3);

	/*
	
		Set up the lights
	
	*/

	Light l1;
	l1.setPosition({ 1.0f, 1.0f, 1.0f });
	l1.setColor({ 0.8f, 0.8f, 0.8f });
	lights.push_back(l1);

	Light l2;
	l2.setPosition({ -1.0f, 1.0f, 1.0f });
	l2.setColor({ 0.2f, 0.2f, 0.0f });
	lights.push_back(l2);


	//set default program
	currProgram = programs[0];
	//set the default material
	currMaterial = materials[0];
	//set the default light
	currLight = &lights[0];

	camera = make_shared<Camera>();
	camera->setInitDistance(2.0f); // Camera's initial Z translation
	
	shape = make_shared<Shape>();
	shape->loadMesh(RESOURCE_DIR + "bunny.obj");
	shape->init();

	shape2 = make_shared<Shape>();
	shape2->loadMesh(RESOURCE_DIR + "teapot.obj");
	shape2->init();
	
	GLSL::checkError(GET_FILE_LINE);
}

// This function is called every frame to draw the scene.
static void render()
{
	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	double t = glfwGetTime();
	if(!keyToggles[(unsigned)' ']) {
		// Spacebar turns animation on/off
		t = 0.0f;
	}
	
	// Matrix stacks
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);

	glm::mat4 S(1.0f);
	S[0][1] = 0.5f * cos(t); // Shear matrix

	// Move bunny to the left side of the screen

	MV->pushMatrix();
	MV->translate({ -0.5f, -0.5f, 0.0f });
	MV->scale({ 0.5f, 0.5f, 0.5f });
	MV->rotate(t, 0.0f, 1.0f, 0.0f);
	
	if (currProgram == prog) {
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		shape->draw(prog);																			// draw the bunny
		prog->unbind();
		MV->popMatrix();																			// pop matrix so transformations do not propogate 
		MV->pushMatrix();																			// Move the teapot to the top right of screen
		MV->translate({ 0.5f, 0.0f, 0.0 });
		MV->multMatrix(S);
		MV->rotate(3.1415, { 0, 1, 0 });
		MV->scale(0.5, 0.5, 0.5);
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		shape2->draw(prog);																			// draw teapot
		prog->unbind();
		MV->popMatrix();
	}
	
	
	if (currProgram == prog2) {
		prog2->bind();
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(transpose(inverse(MV->topMatrix()))));
		glUniform3f(prog2->getUniform("lightPos1"), lights[0].getPosition()[0], lights[0].getPosition()[1], lights[0].getPosition()[2]);
		glUniform3f(prog2->getUniform("lightPos2"), lights[1].getPosition()[0], lights[1].getPosition()[1], lights[1].getPosition()[2]);
		glUniform3f(prog2->getUniform("lightColor1"), lights[0].getColor()[0], lights[0].getColor()[1], lights[0].getColor()[2]);
		glUniform3f(prog2->getUniform("lightColor2"), lights[1].getColor()[0], lights[1].getColor()[1], lights[1].getColor()[2]);
		glUniform3f(prog2->getUniform("ka"), currMaterial.getAmbient()[0], currMaterial.getAmbient()[1], currMaterial.getAmbient()[2]);
		glUniform3f(prog2->getUniform("kd"), currMaterial.getDiffuse()[0], currMaterial.getDiffuse()[1], currMaterial.getDiffuse()[2]);
		glUniform3f(prog2->getUniform("ks"), currMaterial.getSpecular()[0], currMaterial.getSpecular()[1], currMaterial.getSpecular()[2]);
		glUniform1f(prog2->getUniform("s"), currMaterial.getShiny());
		shape->draw(prog2);
		prog2->unbind();
		MV->popMatrix();
		MV->pushMatrix();
		MV->translate({ 0.5f, 0.0f, 0.0 });
		MV->multMatrix(S);
		MV->rotate(3.1415, { 0, 1, 0 });
		MV->scale(0.5, 0.5, 0.5);
		prog2->bind();
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(transpose(inverse(MV->topMatrix()))));
		glUniform3f(prog2->getUniform("lightPos1"), lights[0].getPosition()[0], lights[0].getPosition()[1], lights[0].getPosition()[2]);
		glUniform3f(prog2->getUniform("lightPos2"), lights[1].getPosition()[0], lights[1].getPosition()[1], lights[1].getPosition()[2]);
		glUniform3f(prog2->getUniform("lightColor1"), lights[0].getColor()[0], lights[0].getColor()[1], lights[0].getColor()[2]);
		glUniform3f(prog2->getUniform("lightColor2"), lights[1].getColor()[0], lights[1].getColor()[1], lights[1].getColor()[2]);
		glUniform3f(prog2->getUniform("ka"), currMaterial.getAmbient()[0], currMaterial.getAmbient()[1], currMaterial.getAmbient()[2]);
		glUniform3f(prog2->getUniform("kd"), currMaterial.getDiffuse()[0], currMaterial.getDiffuse()[1], currMaterial.getDiffuse()[2]);
		glUniform3f(prog2->getUniform("ks"), currMaterial.getSpecular()[0], currMaterial.getSpecular()[1], currMaterial.getSpecular()[2]);
		glUniform1f(prog2->getUniform("s"), currMaterial.getShiny());
		shape2->draw(prog2);
		prog2->unbind();
		MV->popMatrix();
	}

	if (currProgram == prog3) {

		prog3->bind();
		glUniformMatrix4fv(prog3->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog3->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog3->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(transpose(inverse(MV->topMatrix()))));
		shape->draw(prog3);
		prog3->unbind();
		MV->popMatrix();
		MV->pushMatrix();
		MV->translate({ 0.5f, 0.0f, 0.0 });
		MV->multMatrix(S);
		MV->rotate(3.1415, { 0, 1, 0 });
		MV->scale(0.5, 0.5, 0.5);
		prog3->bind();
		glUniformMatrix4fv(prog3->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog3->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog3->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(transpose(inverse(MV->topMatrix()))));
		shape2->draw(prog3);
		prog3->unbind();
		MV->popMatrix();
	}

	if (currProgram == prog4) {

		prog4->bind();
		glUniformMatrix4fv(prog4->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog4->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog4->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(transpose(inverse(MV->topMatrix()))));
		glUniform3f(prog4->getUniform("lightPos1"), lights[0].getPosition()[0], lights[0].getPosition()[1], lights[0].getPosition()[2]);
		glUniform3f(prog4->getUniform("lightPos2"), lights[1].getPosition()[0], lights[1].getPosition()[1], lights[1].getPosition()[2]);
		glUniform3f(prog4->getUniform("lightColor1"), lights[0].getColor()[0], lights[0].getColor()[1], lights[0].getColor()[2]);
		glUniform3f(prog4->getUniform("lightColor2"), lights[1].getColor()[0], lights[1].getColor()[1], lights[1].getColor()[2]);
		glUniform3f(prog4->getUniform("ka"), currMaterial.getAmbient()[0], currMaterial.getAmbient()[1], currMaterial.getAmbient()[2]);
		glUniform3f(prog4->getUniform("kd"), currMaterial.getDiffuse()[0], currMaterial.getDiffuse()[1], currMaterial.getDiffuse()[2]);
		glUniform3f(prog4->getUniform("ks"), currMaterial.getSpecular()[0], currMaterial.getSpecular()[1], currMaterial.getSpecular()[2]);
		glUniform1f(prog4->getUniform("s"), currMaterial.getShiny());
		shape->draw(prog4);
		prog4->unbind();
		MV->popMatrix();
		MV->pushMatrix();
		MV->translate({ 0.5f, 0.0f, 0.0 });
		MV->multMatrix(S);
		MV->rotate(3.1415, { 0, 1, 0 });
		MV->scale(0.5, 0.5, 0.5);
		prog4->bind();
		glUniformMatrix4fv(prog4->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog4->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniformMatrix4fv(prog4->getUniform("MVit"), 1, GL_FALSE, glm::value_ptr(transpose(inverse(MV->topMatrix()))));
		glUniform3f(prog4->getUniform("lightPos1"), lights[0].getPosition()[0], lights[0].getPosition()[1], lights[0].getPosition()[2]);
		glUniform3f(prog4->getUniform("lightPos2"), lights[1].getPosition()[0], lights[1].getPosition()[1], lights[1].getPosition()[2]);
		glUniform3f(prog4->getUniform("lightColor1"), lights[0].getColor()[0], lights[0].getColor()[1], lights[0].getColor()[2]);
		glUniform3f(prog4->getUniform("lightColor2"), lights[1].getColor()[0], lights[1].getColor()[1], lights[1].getColor()[2]);
		glUniform3f(prog4->getUniform("ka"), currMaterial.getAmbient()[0], currMaterial.getAmbient()[1], currMaterial.getAmbient()[2]);
		glUniform3f(prog4->getUniform("kd"), currMaterial.getDiffuse()[0], currMaterial.getDiffuse()[1], currMaterial.getDiffuse()[2]);
		glUniform3f(prog4->getUniform("ks"), currMaterial.getSpecular()[0], currMaterial.getSpecular()[1], currMaterial.getSpecular()[2]);
		glUniform1f(prog4->getUniform("s"), currMaterial.getShiny());
		shape2->draw(prog4);
		prog4->unbind();
		MV->popMatrix();
	}
	


	MV->popMatrix();
	P->popMatrix();
	
	GLSL::checkError(GET_FILE_LINE);
	
	if(OFFLINE) {
		saveImage("output.png", window);
		GLSL::checkError(GET_FILE_LINE);
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Usage: A3 RESOURCE_DIR" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	
	// Optional argument
	if(argc >= 3) {
		OFFLINE = atoi(argv[2]) != 0;
	}

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "YOUR NAME", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	GLSL::checkVersion();
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Set the window resize call back.
	glfwSetFramebufferSizeCallback(window, resize_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
