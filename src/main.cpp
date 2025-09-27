#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Model.h"
#include "Triangle.h"
#include "BVH.h"
#include "Camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// screen size settings
const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 1200;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

static void GetModelTriangles(const Model& model, const glm::mat4& M, const glm::vec3& albedo, const glm::vec3& emission, std::vector<bvhTri>& outTris)
{
	for (const auto& mesh : model.meshes) {
		const auto& verts = mesh.vertices;
		const auto& idx = mesh.indices;
		for (size_t i = 0; i + 2 < idx.size(); i += 3) {
			const glm::vec3 p0 = glm::vec3(M * glm::vec4(verts[idx[i + 0]].Position, 1.0f));
			const glm::vec3 p1 = glm::vec3(M * glm::vec4(verts[idx[i + 1]].Position, 1.0f));
			const glm::vec3 p2 = glm::vec3(M * glm::vec4(verts[idx[i + 2]].Position, 1.0f));
			bvhTri tri(p0, p1, p2, 0, albedo, emission);
			outTris.push_back(tri);
		}
	}
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Easy Ray Tracing - yuzhm", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/* Initialize GLEW */
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW init failed\n";
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glDisable(GL_DEPTH_TEST);

	// create and compile the vertex shader and fragment shader
	std::string vertexShaderSource = "resources/shaders/raytracing_vertex.glsl";
	std::string fragmentShaderSource = "resources/shaders/raytracing_fragment.glsl";

	Shader shader = Shader(vertexShaderSource, fragmentShaderSource);

	// Fullscreen draw VAO
	GLuint fsVAO = 0;
	glGenVertexArrays(1, &fsVAO);

	// Load models (geometry only)
	Model tallbox("resources/assets/CornellBox/tallbox.obj");
	Model floor("resources/assets/CornellBox/floor.obj");
	Model shortbox("resources/assets/CornellBox/shortbox.obj");
	Model left("resources/assets/CornellBox/left.obj");
	Model right("resources/assets/CornellBox/right.obj");
	Model light("resources/assets/CornellBox/light.obj");

	// Apply same model transform as before to place the scene
	glm::mat4 M(1.0f);
	M = glm::translate(M, glm::vec3(138.0f, -136.0f, -350.0f));
	M = glm::rotate(M, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	M = glm::scale(M, glm::vec3(0.5f));

	// Build triangles with materials
	std::vector<bvhTri> bvhTris;
	bvhTris.reserve(static_cast<size_t>(tallbox.getTriangles() + floor.getTriangles() + shortbox.getTriangles() + left.getTriangles() + right.getTriangles() + light.getTriangles()));

	GetModelTriangles(tallbox, M, glm::vec3(0.28f, 0.17f, 0.08f), glm::vec3(0.0f), bvhTris);
	GetModelTriangles(floor, M, glm::vec3(0.725f, 0.71f, 0.68f), glm::vec3(0.0f), bvhTris);
	GetModelTriangles(shortbox, M, glm::vec3(0.28f, 0.17f, 0.08f), glm::vec3(0.0f), bvhTris);
	GetModelTriangles(left, M, glm::vec3(0.63f, 0.065f, 0.05f), glm::vec3(0.0f), bvhTris);
	GetModelTriangles(right, M, glm::vec3(0.14f, 0.45f, 0.091f), glm::vec3(0.0f), bvhTris);
	GetModelTriangles(light, M, glm::vec3(0.0f), glm::vec3(6.0f), bvhTris);

	// Build BVH
	BVH bvh;
	bvh.build(bvhTris, 8);

	// Pack triangles into a buffer in BVH primitive order: 5 vec4 per triangle
	std::vector<Triangle> triangles;
	triangles.reserve(bvh.primitives.size());

	glm::vec3 sceneMin(std::numeric_limits<float>::infinity());
	glm::vec3 sceneMax(-std::numeric_limits<float>::infinity());

	for (const auto& t : bvh.primitives) {
		glm::vec4 p0(t.v0, 1.0f);
		glm::vec4 p1(t.v1, 1.0f);
		glm::vec4 p2(t.v2, 1.0f);
		glm::vec4 alb(t.albedo, 0.0f);
		glm::vec4 emi(t.emission, 0.0f);
		triangles.emplace_back(p0, p1, p2, alb, emi);
		sceneMin = glm::min(sceneMin, t.bounds.bmin);
		sceneMax = glm::max(sceneMax, t.bounds.bmax);
	}

	int triCount = static_cast<int>(triangles.size());

	// Upload triangle TBO
	GLuint triBuffer = 0;
	glGenBuffers(1, &triBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, triBuffer);
	glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(triangles.size() * sizeof(Triangle)), triangles.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	GLuint triTex = 0;
	glGenTextures(1, &triTex);
	glBindTexture(GL_TEXTURE_BUFFER, triTex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, triBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	// Pack BVH nodes into texels: 3 RGBA32F per node
	std::vector<glm::vec4> nodeTexels;
	nodeTexels.reserve(bvh.nodes.size() * 3);
	for (const auto& n : bvh.nodes) {
		nodeTexels.emplace_back(glm::vec4(n.bounds.bmin, static_cast<float>(n.left)));
		nodeTexels.emplace_back(glm::vec4(n.bounds.bmax, static_cast<float>(n.right)));
		nodeTexels.emplace_back(glm::vec4(static_cast<float>(n.start), static_cast<float>(n.count), 0.0f, 0.0f));
	}

	GLuint bvhBuffer = 0;
	glGenBuffers(1, &bvhBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, bvhBuffer);
	glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(nodeTexels.size() * sizeof(glm::vec4)), nodeTexels.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	GLuint bvhTex = 0;
	glGenTextures(1, &bvhTex);
	glBindTexture(GL_TEXTURE_BUFFER, bvhTex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, bvhBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	shader.UnBindShader();

	int frame = 0;
	const int spp = 20;     // samples per pixel
	const int maxDepth = 8;  // max bounces

	// FPS calculation variables
	int framesThisSecond = 0;
	double fps = 0.0;
	auto lastFpsTime = std::chrono::high_resolution_clock::now();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		//float currentFrame = static_cast<float>(glfwGetTime());
		//deltaTime = currentFrame - lastFrame;
		//lastFrame = currentFrame;

		// input
		//processInput(window);

		// Resolution and matrices
		int fbw, fbh;
		glfwGetFramebufferSize(window, &fbw, &fbh);
		float aspect = static_cast<float>(fbw) / static_cast<float>(fbh);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(camera.Fov), aspect, 0.1f, 1000.0f);
		glm::mat4 invVP = glm::inverse(proj * view);

		// Render fullscreen path tracing
		glClear(GL_COLOR_BUFFER_BIT);

		shader.BindShader();

		// Bind triangle buffer texture to unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, triTex);
		shader.SetUniform1i("uTriangles", 0);
		shader.SetUniform1i("uTriangleCount", triCount);

		// Bind BVH node buffer to unit 1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, bvhTex);
		shader.SetUniform1i("uBVHNodes", 1);

		// Camera uniforms
		shader.SetUniform3fv("uCamPos", camera.Position);
		shader.SetUniformMat4fv("uInvViewProj", invVP);

		// Scene AABB
		shader.SetUniform3fv("uSceneMin", sceneMin);
		shader.SetUniform3fv("uSceneMax", sceneMax);

		// Resolution and integrator params
		shader.SetUniform2f("uResolution", static_cast<float>(fbw), static_cast<float>(fbh));
		shader.SetUniform1i("uSpp", spp);
		shader.SetUniform1i("uMaxDepth", maxDepth);
		shader.SetUniform1i("uFrame", frame);

		glBindVertexArray(fsVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		shader.UnBindShader();

		// FPS calculation
		framesThisSecond++;
		auto now = std::chrono::high_resolution_clock::now();
		double elapsed = std::chrono::duration<double>(now - lastFpsTime).count();
		if (elapsed >= 1.0) {
			fps = framesThisSecond / elapsed;
			char title[128];
			snprintf(title, sizeof(title), "Easy Ray Tracing - yuzhm | SSP: %d | FPS: %d", spp, static_cast<int>(fps));
			glfwSetWindowTitle(window, title);
			framesThisSecond = 0;
			lastFpsTime = now;
		}

		frame++;

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	if (bvhTex) glDeleteTextures(1, &bvhTex);
	if (bvhBuffer) glDeleteBuffers(1, &bvhBuffer);
	if (triTex) glDeleteTextures(1, &triTex);
	if (triBuffer) glDeleteBuffers(1, &triBuffer);
	if (fsVAO) glDeleteVertexArrays(1, &fsVAO);

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}