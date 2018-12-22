#include <glfw3.h>
#include "Model\Model.h"
#include "Utility\GLCamera.h"

struct PointLight
{
	glm::vec3 Position;
	glm::vec3 Color;

	float Linear;
	float Quadratic;

};

std::vector<PointLight> lights;

int SCREEN_WIDTH = 1280, SCREEN_HEIGHT = 720;

Shader geometryPass;
Shader lightingPass;

Shader forwardRender;
int whattodraw = 0;

GLCamera camera;
GLFWwindow * window;
void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void handleCameraKeyInput();
bool initDefferredShading();
void pLightToShader(Shader s);

float deltatime = 0;
Timer timer;
float ticks = 0;



unsigned int gBuffer;
unsigned int depthBuffer;
unsigned int posTex, normTex, colorTex;

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad();


int initOpenGL()
{
	//Init glfw
	glfwInit();
	//Set version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	//Creating window
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Deferred Shading", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create window";
		return -1;
	}

	//Set context to current window
	glfwMakeContextCurrent(window);



	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetKeyCallback(window, keyboardCallback);
	glfwSwapInterval(0);


	//initialize GLAD function pointers.
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	return 1;


}

bool initDefferredShading()
{

	//Gen framebuffer.
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);


	//Configure textures we're writing to in geometry pass.
	glGenTextures(1, &posTex);
	glBindTexture(GL_TEXTURE_2D, posTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posTex, 0);


	glGenTextures(1, &normTex);
	glBindTexture(GL_TEXTURE_2D, normTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normTex, 0);


	glGenTextures(1, &colorTex);
	glBindTexture(GL_TEXTURE_2D, colorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, colorTex, 0);


	//Tell gl we're using these when rendering.
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//If we want to do forward rendering, we need to store depth of geometry pass.
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);


	//Making sure the framebuffer is running.
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer BROKEN!" << std::endl;
		return 1;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	lightingPass.use();
	lightingPass.setInt("_Position", 0);
	lightingPass.setInt("_Normal", 1);
	lightingPass.setInt("_AlbedoSpec", 2);

	glActiveTexture(GL_TEXTURE0);
}


void pLightToShader(Shader s)
{
	for (unsigned int i = 0; i < lights.size(); i++)
	{
		s.setVec3("lights[" + std::to_string(i) + "].Position", lights[i].Position);
		s.setVec3("lights[" + std::to_string(i) + "].Color", lights[i].Color);

		const float linear = 0.1;
		const float quadratic = 0.1;
		s.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
		s.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
	}
}

void initLights()
{
	// lighting info
	// -------------
	const unsigned int NR_LIGHTS = 32;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(255);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = ((rand() % 100) / 100.0) * 6.0 - 16.0;
		float yPos = ((rand() % 100) / 100.0) * 6.0 + 4;
		float zPos = ((rand() % 100) / 100.0) * 6.0 - 15.0;
		glm::vec3 pos(xPos, yPos, zPos);
		// also calculate random color
		float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
		float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
		float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
		glm::vec3 color(rColor, gColor, bColor);

		PointLight p;
		p.Color = color;
		p.Position = pos;
		lights.push_back(p);
	}
}
int animID = 0;
int main()
{
	initOpenGL();
	geometryPass.init("Shaders/geoPass.vshader", "Shaders/geoPass.fshader");
	lightingPass.init("Shaders/shadePass.vshader", "Shaders/shadePass.fshader");
	forwardRender.init("Shaders/animated.vshader", "Shaders/animated.fshader");
	glEnable(GL_DEPTH_TEST);
	//Hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	initDefferredShading();
	unsigned int t;
	glGenTextures(1, &t);
	initLights();
	lights[0].Position = glm::vec3(0,4,4);

	std::vector<glm::vec3> positions =
	{
		glm::vec3(0,0,0),
		glm::vec3(4,0,12),
		glm::vec3(-1,17,6),
		glm::vec3(2,20,-6),
		glm::vec3(-20,1, 40),
		glm::vec3(14,15, 8),
		glm::vec3(8,14,4),
		glm::vec3(40,40,-4),
		glm::vec3(-5,-20, 0),
		glm::vec3(-12,2,-20)

	};
	std::vector<AnimatedModel> models;
	models.resize(1);

	for (unsigned int i = 0; i < models.size(); i++)
	{
		models[i].Init("data/Eletric Fan Animation.fbx");
		models[i].position = positions[i];
		models[i].scale_vector = glm::vec3(0.1);
		models[i].eulerAngles.y = 0;
		models[i].eulerAngles.z = 0;
		models[i].eulerAngles.x = 0;
		AnimInfo xd;
		xd.id = animID;
		xd.anim_rate = 0.8;
		models[i].AddTextureToMesh("data/fan_BaseColor.png", "texture_diffuse", 0);
		models[i].setAnimation(xd);
	
	}
	

	timer.start();
	timer.unpause();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	while (!glfwWindowShouldClose(window))
	{
		float t = timer.getTicks();
		deltatime = (timer.getTicks() - ticks) / 1000.0f;
		ticks = timer.getTicks();

		AnimInfo xd;
		xd.id = animID;
		xd.anim_rate = 0.4;

		models[0].setAnimation(xd);
		handleCameraKeyInput();

		//Geometry Pass

		glm::mat4 projection = glm::perspective(glm::radians(50.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 5000.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		geometryPass.use();
		geometryPass.setBool("hasNormal", false);
		geometryPass.setMat4("view", view);
		geometryPass.setMat4("projection", projection);
		for (unsigned int i = 0; i < models.size(); i++)
		{
		
			models[i].Draw(geometryPass);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		lightingPass.use();
		//Lighting pass
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, posTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, colorTex);

		pLightToShader(lightingPass);
		lightingPass.setInt("whattodraw", whattodraw);
		lightingPass.setVec3("viewPos", camera.Position);
		lightingPass.setInt("numLights", lights.size());
		renderQuad();

		// ----------------------------------------------------------------------------------
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
												   // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
												   // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
												   // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
		glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//Swap Buffers.
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;

}


void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	glViewport(0, 0, width, height);
}


//Members used for mouse input.
double lastX = 0, lastY = 0;
bool firstMouse = true;
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
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


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_1)
		{
			whattodraw = 0;
		}
		if (key == GLFW_KEY_2)
		{
			whattodraw = 1;
		}
		if (key == GLFW_KEY_3)
		{
			whattodraw = 2;
		}
		if (key == GLFW_KEY_4)
		{
			whattodraw = 3;
		}

		//if (key == GLFW_KEY_P)
		//	animID++;

	}
	if (key == GLFW_KEY_ESCAPE)
	{
		//Close
		glfwSetWindowShouldClose(window, true);
	}

}

void handleCameraKeyInput()
{

	//Camera movement 
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltatime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltatime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		float velocity = camera.MovementSpeed * deltatime;
		camera.Position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		float velocity = camera.MovementSpeed * deltatime;
		camera.Position += glm::vec3(0.0f, -1.0f, 0.0f) * velocity;
	}

}



void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}