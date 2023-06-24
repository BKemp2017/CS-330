#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <direct.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

//GLM math header inclusions
#include <glm/glm.hpp>        
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE

//Camera Class
#include <learnOpengl/camera.h>

using namespace std; // Uses the standard namespace

//Shader Macros
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Final Project"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[4];         // Handle for the vertex buffer object
		GLuint nIndices;   // Number of vertices of the mesh
		GLuint nVertices;
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Texture ID
	GLuint texture1;
	GLuint texture2;
	GLuint texture3;

	// Shader program
	GLuint gProgramId;

	// Shape Mesh
	GLMesh gMesh;
	GLMesh cubeMesh;
	GLMesh planeMesh;
	GLMesh pyramidMesh;
	GLMesh cylinderMesh;
	GLMesh longRectangleMesh;

	// camera
	Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	bool gIsOrthographic = false;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;

	// Light position and scale
	glm::vec3 gLightPosition(1.0f, 2.0f, 1.0f);
	glm::vec3 gLightScale(0.3f);
	glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
	glm::vec3 gLightCubeColor(0.0f, 1.0f, 1.0f);
	float gShininess = 32.0f;
}


/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UCreateCubeMesh(GLMesh& mesh);
void UCreateCylinderMesh(GLMesh& mesh);
void UCreatePlaneMesh(GLMesh& mesh);
void UCreatePyramidMesh(GLMesh& mesh);
void UCreateLongRectangle(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;
	layout(location = 1) in vec4 color;
	layout(location = 2) in vec2 texCoord;
	layout(location = 3) in vec3 normal;

	out vec4 vertexColor;
	out vec2 vertexTexCoord;
	out vec3 vertexPos;
	out vec3 vertexNormal;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	uniform vec3 keyLightDirection;
	uniform vec3 keyLightColor;
	uniform float keyLightIntensity;
	uniform vec3 fillLightDirection;
	uniform vec3 fillLightColor;
	uniform float fillLightIntensity;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0);
	vertexColor = color;
	vertexTexCoord = texCoord;
	vertexPos = vec3(model * vec4(position, 1.0));
	vertexNormal = mat3(transpose(inverse(model))) * normal;

	// Calculate lighting
	float keyLightIntensityFactor = max(dot(vertexNormal, normalize(keyLightDirection)), 0.0);
	float fillLightIntensityFactor = max(dot(vertexNormal, normalize(fillLightDirection)), 0.0);
	vec3 totalLightColor = keyLightIntensity * keyLightColor * keyLightIntensityFactor +
		fillLightIntensity * fillLightColor * fillLightIntensityFactor;
	vertexColor.rgb *= totalLightColor;
}
);


const GLchar* fragmentShaderSource = GLSL(440,
	in vec3 vertexColor;
	in vec2 vertexTexCoord;
	in vec3 vertexPos;
	in vec3 vertexNormal;

	out vec4 fragmentColor;

	uniform sampler2D uTextureBase;
	uniform sampler2D uTextureExtra1;
	uniform sampler2D uTextureExtra2;
	uniform sampler2D uTextureExtra3;
	uniform bool multipleTextures;

	uniform vec3 lightPosition;
	uniform vec3 lightCubeColor;
	uniform float shininess;


void main()
{
	vec4 baseColor = texture(uTextureBase, vertexTexCoord);
	if (multipleTextures)
	{
		vec4 extraTexture1 = texture(uTextureExtra1, vertexTexCoord);
		vec4 extraTexture2 = texture(uTextureExtra2, vertexTexCoord);
		vec4 extraTexture3 = texture(uTextureExtra3, vertexTexCoord);

		// Check alpha value to determine which texture to use
		if (extraTexture1.a != 0.0)
			baseColor = extraTexture1;
		else if (extraTexture2.a != 0.0)
			baseColor = extraTexture2;
		else if (extraTexture3.a != 0.0)
			baseColor = extraTexture3;
	}

	// Ambient lighting
	vec3 ambient = lightCubeColor;

	// Diffuse lighting
	vec3 normal = normalize(vertexNormal);
	vec3 lightDirection = normalize(lightPosition - vertexPos);
	float diffuseStrength = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diffuseStrength * lightCubeColor;

	// Specular lighting
	vec3 viewDirection = normalize(-vertexPos);
	vec3 reflectDirection = reflect(-lightDirection, normal);
	float specularStrength = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
	vec3 specular = specularStrength * lightCubeColor;

	// Final color calculation
	vec3 finalColor = (ambient + diffuse + specular) * baseColor.rgb;
	fragmentColor = vec4(finalColor, baseColor.a);
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}



int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	//UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

	// Create the shader program
	UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId);

	char currentDir[FILENAME_MAX];
	if (_getcwd(currentDir, sizeof(currentDir)) != nullptr)
	{
		std::cout << "Current working directory: " << currentDir << std::endl;
	}

	// Load texture 1
	const char* texFilename = "C:/Users/blake/OneDrive/Desktop/Final Project/Libraries/Textures/Desk.jpg";
	if (!UCreateTexture(texFilename, texture1))
	{
		std::cout << "Failed to load texture 1: " << texFilename << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture 3
	const char* tex3Filename = "C:/Users/blake/OneDrive/Desktop/Final Project/Libraries/Textures/coaster.jpg";
	if (!UCreateTexture(tex3Filename, texture3))
	{
		std::cout << "Failed to load texture 3: " << tex3Filename << std::endl;
		return EXIT_FAILURE;
	}

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTextureBase"), 0);
	// We set the texture as texture unit 1
	glUniform1i(glGetUniformLocation(gProgramId, "uTextureExtra1"), 1);
	// We set the texture as texture unit 2
	glUniform1i(glGetUniformLocation(gProgramId, "uTextureExtra2"), 2);
	// We set the texture as texture unit 3
	glUniform1i(glGetUniformLocation(gProgramId, "uTextureExtra3"), 3);

	// Setting the background color of the window to light blue using glClear
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // This makes it light blue

	// render loop
	while (!glfwWindowShouldClose(gWindow))
	{

		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwSwapBuffers(gWindow);
		glfwPollEvents();
	}
	// Release mesh data
	UDestroyMesh(gMesh);
	UDestroyTexture(texture1);
	UDestroyTexture(texture2);
	UDestroyTexture(texture3);

	// Release shader program
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS); //Terminates the program
}


bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	//GLFW: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	//GLFW: Window Creation 
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create the GLFW Window" << std::endl; //Message if the window fails
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//GLEW: Initialize
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	//Displays GPU OpenGL Version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{

	static const float cameraSpeed = 2.5f;
	static const float cameraSensitivity = 0.1f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraOffset = cameraSpeed * gDeltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UPWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWNWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		gCamera.ProcessMouseMovement(0.0f, -cameraSensitivity);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		gCamera.ProcessMouseMovement(0.0f, cameraSensitivity);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		gCamera.ProcessMouseMovement(cameraSensitivity, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		gCamera.ProcessMouseMovement(-cameraSensitivity, 0.0f);

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		if (gIsOrthographic)
			gIsOrthographic = false;
		else
			gIsOrthographic = true;
	}

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

float gCameraSpeed = 0.1f;

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{

	float scrollSpeed = 0.1f;
	yoffset *= scrollSpeed;
	gCameraSpeed += yoffset;

	//Ensure camera speed is always positive
	if (gCameraSpeed < 0.1f)
		gCameraSpeed = 0.1f;
	gCamera.ProcessMouseScroll(yoffset);
}

// Global variables for the key light and fill light
glm::vec3 gKeyLightDirection(1.0f, -1.0f, 0.0f); // Direction towards the front face
glm::vec3 gKeyLightColor(1.0f, 0.0f, 0.0f); // Green color for the key light
float gKeyLightIntensity = 2.0f; // 100% intensity
glm::vec3 gFillLightDirection(1.0f, 0.0f, 1.0f); // Direction towards the right face
glm::vec3 gFillLightColor(0.0f, 1.0f, 0.0f); // Green color for the fill light
float gFillLightIntensity = 0.2f; // 10% intensity


// Function called to render a frame
void URender()
{
	// Enables the Z-Depth
	glEnable(GL_DEPTH_TEST);

	// Clear the background
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	// Creates a perspective projection
	glm::mat4 projection;
	if (gIsOrthographic) {
		float left = -10.0f;
		float right = 10.0f;
		float bottom = -10.0f;
		float top = 10.0f;
		projection = glm::ortho(left, right, bottom, top, 0.1f, 100.0f);
	}
	else {
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// Create the cube mesh
	GLMesh cubeMesh;
	UCreateCubeMesh(cubeMesh);

	//Create Then Cylinder Mesh 
	GLMesh cylinderMesh;
	UCreateCylinderMesh(cylinderMesh);

	// Create the plane mesh
	GLMesh planeMesh;
	UCreatePlaneMesh(planeMesh);

	GLMesh pyramidMesh;
	UCreatePyramidMesh(pyramidMesh);

	//Long Pyramid
	GLMesh longRectangleMesh;
	UCreateLongRectangle(longRectangleMesh);

	// Set the shader to be used
	glUseProgram(gProgramId);

	// Sends transform information to the Vertex Shader
	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	// Adds the lighting uniform variables
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPosition");
	GLint lightCubeColorLoc = glGetUniformLocation(gProgramId, "lightCubeColor");
	GLint shininessLoc = glGetUniformLocation(gProgramId, "shininess");

	// Set the lighting uniform variables
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	glUniform3f(lightCubeColorLoc, gLightCubeColor.r, gLightCubeColor.g, gLightCubeColor.b);
	glUniform1f(shininessLoc, gShininess);

	GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
	glUniform3f(lightCubeColorLoc, gLightCubeColor.r, gLightCubeColor.g, gLightCubeColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);


	// Set lighting uniform variables
	GLint keyLightDirectionLoc = glGetUniformLocation(gProgramId, "keyLightDirection");
	GLint keyLightColorLoc = glGetUniformLocation(gProgramId, "keyLightColor");
	GLint keyLightIntensityLoc = glGetUniformLocation(gProgramId, "keyLightIntensity");
	GLint fillLightDirectionLoc = glGetUniformLocation(gProgramId, "fillLightDirection");
	GLint fillLightColorLoc = glGetUniformLocation(gProgramId, "fillLightColor");
	GLint fillLightIntensityLoc = glGetUniformLocation(gProgramId, "fillLightIntensity");


	// Set texture uniforms
	GLint textureBaseLoc = glGetUniformLocation(gProgramId, "uTextureBase");
	GLint textureExtra1Loc = glGetUniformLocation(gProgramId, "uTextureExtra1");
	GLint textureExtra2Loc = glGetUniformLocation(gProgramId, "uTextureExtra2");
	GLint textureExtra3Loc = glGetUniformLocation(gProgramId, "uTextureExtra3");
	GLint multipleTexturesLoc = glGetUniformLocation(gProgramId, "multipleTextures");

	// Set the flag for multiple textures
	glUniform1i(multipleTexturesLoc, true); // Enable multiple textures

	// Pass the light parameters to the shader
	glUniform3fv(keyLightDirectionLoc, 1, glm::value_ptr(gKeyLightDirection));
	glUniform3fv(keyLightColorLoc, 1, glm::value_ptr(gKeyLightColor));
	glUniform1f(keyLightIntensityLoc, gKeyLightIntensity);
	glUniform3fv(fillLightDirectionLoc, 1, glm::value_ptr(gFillLightDirection));
	glUniform3fv(fillLightColorLoc, 1, glm::value_ptr(gFillLightColor));
	glUniform1f(fillLightIntensityLoc, gFillLightIntensity);

	// Render the first cube with texture 2
	glm::mat4 cubeModel1 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.05f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel1));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(cubeMesh.vao);

	// Activate and bind texture 2 (cube 1)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture3);
	glDrawElements(GL_TRIANGLES, cubeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
	glBindVertexArray(0);

	// Render the second cube with texture 3
	glm::mat4 cubeModel2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.10f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel2));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(cubeMesh.vao);

	// Activate and bind texture 3 (cube 2)
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture3);
	glDrawElements(GL_TRIANGLES, cubeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
	glBindVertexArray(0);

	// Render the plane
	glm::mat4 planeModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)); // Identity matrix for plane
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(planeModel));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(planeMesh.vao);

	// Activate and bind texture 2 (plane)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glUniform1i(textureBaseLoc, 1); // Set the texture unit for the base texture
	glDrawElements(GL_TRIANGLES, planeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
	glBindVertexArray(0);

	// Render the long pyramid
	glm::mat4 longRectangleModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.20f, 0.4f, -0.95f)); // Set the position of the prism
	longRectangleModel = glm::rotate(longRectangleModel, glm::radians(85.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	longRectangleModel = glm::rotate(longRectangleModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	// Modify the translation part to position the triangular prism
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(longRectangleModel));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(longRectangleMesh.vao);
	glDrawElements(GL_TRIANGLES, longRectangleMesh.nIndices, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);

	// Render the cylinder
	glm::mat4 cylinderModel = glm::translate(glm::mat4(1.0f), glm::vec3(1.25f, 0.20f, -1.0f)); // Set the position of the cylinder
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cylinderModel));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(cylinderMesh.vao);
	glDrawElements(GL_TRIANGLES, cylinderMesh.nIndices, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);

	// Render the pyramid
	glm::mat4 pyramidModel = glm::translate(glm::mat4(1.0f), glm::vec3(1.25f, 0.5f, 0.0f)); // Set the position of the pyramid
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pyramidModel));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(pyramidMesh.vao);
	glDrawElements(GL_TRIANGLES, pyramidMesh.nIndices, GL_UNSIGNED_SHORT, nullptr);


	// Disable multiple textures
	glUniform1i(multipleTexturesLoc, false);

	glUseProgram(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved, etc.)
	glfwSwapBuffers(gWindow); // Flips the back buffer with the front buffer every frame.
}


// Create a mesh for a cube
void UCreateCubeMesh(GLMesh& mesh)
{
	GLfloat cubeVerts[] =
	{
		// Vertex Positions     // Color                 // Texture Coordinates
		-0.5f, -0.05f, -0.5f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 1.0f, // Vertex 1
		 0.5f, -0.05f, -0.5f,   0.0f, 1.0f, 0.0f, 1.0f,   1.0f, 1.0f, // Vertex 2
		 0.5f,  0.00f, -0.5f,   0.0f, 0.0f, 1.0f, 1.0f,   1.0f, 0.0f, // Vertex 3
		-0.5f,  0.00f, -0.5f,   1.0f, 1.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Vertex 4

		-0.5f, -0.05f,  0.5f,   0.0f, 1.0f, 1.0f, 1.0f,   0.0f, 0.0f, // Vertex 5
		 0.5f, -0.05f,  0.5f,   1.0f, 0.0f, 1.0f, 1.0f,   1.0f, 0.0f, // Vertex 6
		 0.5f,  0.00f,  0.5f,   1.0f, 1.0f, 1.0f, 1.0f,   1.0f, 1.0f, // Vertex 7
		-0.5f,  0.00f,  0.5f,   0.5f, 0.5f, 0.5f, 1.0f,   0.0f, 1.0f  // Vertex 8
	};

	GLushort cubeIndices[] = {
		0, 1, 2,   // Triangle 1
		2, 3, 0,   // Triangle 2
		1, 5, 6,   // Triangle 3
		6, 2, 1,   // Triangle 4
		7, 6, 5,   // Triangle 5
		5, 4, 7,   // Triangle 6
		4, 0, 3,   // Triangle 7
		3, 7, 4,   // Triangle 8
		4, 5, 1,   // Triangle 9
		1, 0, 4,   // Triangle 10
		3, 2, 6,   // Triangle 11
		6, 7, 3    // Triangle 12
	};

	mesh.nVertices = sizeof(cubeVerts) / (sizeof(cubeVerts[0]) * 9);
	mesh.nIndices = sizeof(cubeIndices) / sizeof(cubeIndices[0]);

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(2, mesh.vbos);

	// Create VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

	// Specify the vertex attributes
	GLsizei stride = 9 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(7 * sizeof(float))); // Add texture coordinates attribute
	glEnableVertexAttribArray(2);

	// Create VBO for index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
}

void UCreatePyramidMesh(GLMesh& mesh) {
	// Vertex positions for pyramid
	GLfloat pyramidVerts[] = {
		// Vertex positions             // Colors
		 0.0f,  0.45f,  0.0f,         1.0f,  0.0f,  0.0f,  1.0f, // Top Vertex 0 (Red)
		-0.5f, -0.45f,  0.5f,         0.0f,  1.0f,  0.0f,  1.0f, // Back left Vertex 1 (Green)
		 0.5f, -0.45f,  0.5f,         0.0f,  0.0f,  1.0f,  1.0f, // Front left Vertex 2 (Blue)
		 0.5f, -0.45f, -0.5f,         1.0f,  0.0f,  0.0f,  1.0f, // Front right Vertex 3 (Purple)
		-0.5f, -0.45f, -0.5f,         0.0f,  1.0f,  0.0f,  1.0f  // Back right Vertex 4 (Yellow)
	};

	GLushort pyramidIndices[] = {
		0, 1, 2,    // Triangle 1
		0, 2, 3,    // Triangle 2
		0, 3, 4,    // Triangle 3
		0, 4, 1,    // Triangle 4
		1, 2, 4,    // Triangle 5
		2, 3, 4     // Triangle 6
	};

	mesh.nVertices = sizeof(pyramidVerts) / (sizeof(pyramidVerts[0]) * 9);
	mesh.nIndices = sizeof(pyramidIndices) / sizeof(pyramidIndices[0]);

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(2, mesh.vbos);

	// Create VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVerts), pyramidVerts, GL_STATIC_DRAW);

	// Specify the vertex attributes
	GLsizei stride = 7 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Create VBO for index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pyramidIndices), pyramidIndices, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
}

void UCreateCylinderMesh(GLMesh& mesh) {
	const float radius = 0.5f;
	const float height = 0.5f;
	const int sectorCount = 36;
	const int stackCount = 1;

	std::vector<GLfloat> cylinderVerts;
	std::vector<GLushort> cylinderIndices;

	const float sectorStep = 2.0f * glm::pi<float>() / sectorCount;
	const float stackHeight = height / stackCount;

	// Create vertices for the sides of the cylinder
	for (int i = 0; i <= stackCount; ++i) {
		float y = -0.5f * height + i * stackHeight;

		for (int j = 0; j <= sectorCount; ++j) {
			float x = radius * cos(j * sectorStep);
			float z = radius * sin(j * sectorStep);

			float u = (float)j / sectorCount;
			float v = (float)i / stackCount;

			cylinderVerts.push_back(x);
			cylinderVerts.push_back(y);
			cylinderVerts.push_back(z);

			cylinderVerts.push_back(0.2f);  // Red color
			cylinderVerts.push_back(0.2f);  // Green color
			cylinderVerts.push_back(0.2f);  // Blue color
			cylinderVerts.push_back(1.0f);  // Alpha color

			cylinderVerts.push_back(u);
			cylinderVerts.push_back(v);
		}
	}

	// Create vertices for the top and bottom of the cylinder
	float yTop = 0.5f * height;
	float yBottom = -0.5f * height;

	// Top vertex
	cylinderVerts.push_back(0.0f);
	cylinderVerts.push_back(yTop);
	cylinderVerts.push_back(0.0f);
	cylinderVerts.push_back(1.0f);  // Red color
	cylinderVerts.push_back(1.0f);  // Green color
	cylinderVerts.push_back(1.0f);  // Blue color
	cylinderVerts.push_back(1.0f);  // Alpha color
	cylinderVerts.push_back(0.5f);
	cylinderVerts.push_back(1.0f);

	// Bottom vertex
	cylinderVerts.push_back(0.0f);
	cylinderVerts.push_back(yBottom);
	cylinderVerts.push_back(0.0f);
	cylinderVerts.push_back(1.0f);  // Red color
	cylinderVerts.push_back(1.0f);  // Green color
	cylinderVerts.push_back(1.0f);  // Blue color
	cylinderVerts.push_back(1.0f);  // Alpha color
	cylinderVerts.push_back(0.5f);
	cylinderVerts.push_back(0.0f);

	// Indices for the sides of the cylinder
	for (int i = 0; i < stackCount; ++i) {
		int k1 = i * (sectorCount + 1);
		int k2 = k1 + sectorCount + 1;

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
			cylinderIndices.push_back(k1);
			cylinderIndices.push_back(k2);
			cylinderIndices.push_back(k1 + 1);

			cylinderIndices.push_back(k1 + 1);
			cylinderIndices.push_back(k2);
			cylinderIndices.push_back(k2 + 1);
		}
	}

	// Indices for the top and bottom of the cylinder
	int centerVertexTopIndex = cylinderVerts.size() / 9 - 2;   // Index of the top vertex
	int centerVertexBottomIndex = cylinderVerts.size() / 9 - 1;   // Index of the bottom vertex
	int firstVertexIndex = stackCount * (sectorCount + 1);   // Index of the first side vertex

	// Indices for the top of the cylinder
	for (int i = 0; i < sectorCount; ++i) {
		cylinderIndices.push_back(centerVertexTopIndex);
		cylinderIndices.push_back(firstVertexIndex + i);
		cylinderIndices.push_back(firstVertexIndex + i + 1);
	}

	// Indices for the bottom of the cylinder
	for (int i = 0; i < sectorCount; ++i) {
		cylinderIndices.push_back(centerVertexBottomIndex);
		cylinderIndices.push_back(firstVertexIndex + i + 1);
		cylinderIndices.push_back(firstVertexIndex + i);
	}

	mesh.nVertices = cylinderVerts.size() / 9;
	mesh.nIndices = cylinderIndices.size();

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(2, mesh.vbos);

	// Create VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, cylinderVerts.size() * sizeof(GLfloat), cylinderVerts.data(), GL_STATIC_DRAW);

	// Specify the vertex attributes
	GLsizei stride = 9 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Create VBO for index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinderIndices.size() * sizeof(GLushort), cylinderIndices.data(), GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
}

// Create a mesh for a plane
void UCreatePlaneMesh(GLMesh& mesh)
{
	GLfloat planeVerts[] = {
		// Plane vertices
		// Vertex positions          // Colors                  // Texture Coordinates
		-3.0f,  0.0f, -3.0f,        0.0f, 0.0f, 1.0f, 0.0f,      0.0f, 1.0f, // Vertex 1
		 3.0f,  0.0f, -3.0f,        0.0f, 0.0f, 1.0f, 0.0f,      1.0f, 1.0f, // Vertex 2
		 3.0f,  0.0f,  3.0f,        0.0f, 0.0f, 1.0f, 0.0f,      1.0f, 0.0f, // Vertex 3
		-3.0f,  0.0f,  3.0f,        0.0f, 0.0f, 1.0f, 0.0f,      0.0f, 0.0f  // Vertex 4
	};
	GLushort planeIndices[] = {
		0, 1, 2,    // Triangle 1
		2, 3, 0,    // Triangle 2
	};

	mesh.nVertices = sizeof(planeVerts) / (sizeof(planeVerts[0]) * 9);
	mesh.nIndices = sizeof(planeIndices) / sizeof(planeIndices[0]);

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(2, mesh.vbos);

	// Create VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW);

	// Specify the vertex attributes
	GLsizei stride = 9 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Create VBO for index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
}

void UCreateLongRectangle(GLMesh& mesh) {
	// Vertex positions for long pyramid
	GLfloat longRectangleVerts[] = {
		// Vertex positions             // Colors
		-0.25f, -1.0f, -0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Bottom left back Vertex 0 (Dark Grey)
		 0.25f, -1.0f, -0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Bottom right back Vertex 1 (Dark Grey)
		 0.25f, -1.0f,  0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Bottom right front Vertex 2 (Dark Grey)
		-0.25f, -1.0f,  0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Bottom left front Vertex 3 (Dark Grey)
		-0.25f,  1.0f, -0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Top left back Vertex 4 (Dark Grey)
		 0.25f,  1.0f, -0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Top right back Vertex 5 (Dark Grey)
		 0.25f,  1.0f,  0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Top right front Vertex 6 (Dark Grey)
		-0.25f,  1.0f,  0.25f,         0.2f,  0.2f,  0.2f,  1.0f, // Top left front Vertex 7 (Dark Grey)
	};
	GLushort longRectangleIndices[] = {
		// Bottom face
		0, 1, 2,
		0, 2, 3,

		// Back face
		4, 0, 3,
		4, 3, 7,

		// Right face
		1, 5, 6,
		1, 6, 2,

		// Front face
		5, 4, 7,
		5, 7, 6,

		// Left face
		4, 0, 1,
		4, 1, 5,

		// Top face
		3, 2, 6,
		3, 6, 7,
	};


	mesh.nVertices = sizeof(longRectangleVerts) / (sizeof(longRectangleVerts[0]) * 7);
	mesh.nIndices = sizeof(longRectangleIndices) / sizeof(longRectangleIndices[0]);

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(2, mesh.vbos);

	// Create VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(longRectangleVerts), longRectangleVerts, GL_STATIC_DRAW);

	// Specify the vertex attributes
	GLsizei stride = 7 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Create VBO for index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(longRectangleIndices), longRectangleIndices, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
}

bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}

void UDestroyMesh(GLMesh& mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(2, mesh.vbos);
}

bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{

	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);


	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}

void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}