#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"

#include <iostream>
#include "control_utils.h"

#define PI 3.1415926

#include <vector>
#include <iostream>
using namespace std;

unsigned int loadTexture(char const * path);

float epsilon = PI * 10 / 180;         // 两极部分去掉10°
float mmax = PI / 2 - epsilon;
float mmaxvalue = log(tan(mmax) + 1.0 / cos(mmax));
float ts = (PI / 2 - epsilon) / (PI / 2);

void getSTTheta(float theta, float fai, float &s, float &t)
{
	// 墨卡托坐标
	s = 0.5 - (fai) / (2 * PI);
	// [-π/2, π/2] * ts
	float mtheta = -(theta - PI / 2) * ts;
	t = log(tan(mtheta) + 1.0 / cos(mtheta));
	t = 0.5 + 0.5 * t / mmaxvalue;
}


GLint statcky = 60;	// 横向向切成多少片
GLint stlicex = 60; // 纵向切多少片

Camera camera(glm::vec3(0.0f, 0.0f, 9.0f));
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 1200;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;


//构建球顶点及其纹理坐标
std::vector<float> drawglobeVBO() {
	vector<float> c;
	GLfloat R = 0.2f;	// 半径
	GLfloat angleHy = (GLfloat)(2 * PI) / statcky;	// 横向每份的角度		算出弧度值
	GLfloat angleZx = (GLfloat)(PI) / stlicex;	// 纵向每份的角度		算出弧度值
	GLfloat NumAngleHy = 0;	// 当前横向角度
	GLfloat NumAngleZx = 0;	// 当前纵向角度

	GLfloat x = 0;
	GLfloat y = 0;
	GLfloat z = 0;
	for (float j = 0; j <stlicex; j++) //纵向
	{
		for (float i = 0; i <statcky; i++)
		{
			float s, t;
			float r, theta, fai;

			theta = angleZx*j; 

			fai = angleHy*i;
			
			GLfloat x = R*cos(fai)*sin(theta);	
			GLfloat y = R*sin(fai)*sin(theta);
			GLfloat z = R*cos(theta);
			c.push_back(x);
			c.push_back(z);
			c.push_back(-y);

			//法向量
			c.push_back(x);
			c.push_back(z);
			c.push_back(-y);
			
			getSTTheta(theta, fai, s, t);

			c.push_back(s);
			c.push_back(t);
			
		}
	}
	return c;
}

std::vector<int> drawglobeEBO() {
	vector<int> ebo;
	vector<float> vbo = drawglobeVBO();
	int only = vbo.size();
	int num = (int)((only / (3 * statcky)) * 2);
	
	for (int y = 0; y<statcky; y++)
	{
		for (int x = 0; x<stlicex; x++)
		{
			ebo.push_back(x + y*stlicex);
			ebo.push_back(x + y*stlicex + 1);
			ebo.push_back(x + y*stlicex + stlicex);
			ebo.push_back(x + y*stlicex + stlicex + 1);
			ebo.push_back(x + y*stlicex + stlicex);
			ebo.push_back(x + y*stlicex + 1);
		}
	}
	return ebo;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(1200, 1200, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);
	Shader lampShader("./src/shader.vs", "./src/shader.fs");
	Shader lightShader("./src/light.vs", "./src/light.fs");

	vector<float> mmc = drawglobeVBO();
	vector<int> mfc = drawglobeEBO();

	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, mmc.size() * sizeof(float), &mmc[0], GL_STATIC_DRAW);
	// &vertor[0] 是指这个vector的地址。&vector不是指vector地址。

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//光源
	/*unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); */

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mfc.size() * sizeof(float), &mfc[0], GL_STATIC_DRAW);

	unsigned int EBO2;
	glGenBuffers(1, &EBO2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mfc.size() * sizeof(float), &mfc[0], GL_STATIC_DRAW);

	/*
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	*/
	
	unsigned int lightEBO;
	glGenBuffers(1, &lightEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mfc.size() * sizeof(float), &mfc[0], GL_STATIC_DRAW);

	unsigned int diffuseMap1 = loadTexture("./img/earth.jpg");
	unsigned int diffuseMap2 = loadTexture("./img/moon.jpg");
	unsigned int diffuseMap3 = loadTexture("./img/sun.jpg");

	while (!glfwWindowShouldClose(window)) 
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)1200 / (float)1200, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
		
		lampShader.use();
		lampShader.setInt("diffuse", 0);
		/*
		lampShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
		lampShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
		lampShader.setVec3("lightPos", lightPos);*/

		//材质变化
		lampShader.setVec3("viewPos", camera.Position);

		
		glm::vec3 lightColor;
		lightColor.x = 1.0f;//sin(glfwGetTime() * 2.0f);
		lightColor.y = 1.0f;//sin(glfwGetTime() * 0.7f);
		lightColor.z = 1.0f;//sin(glfwGetTime() * 1.3f);
		glm::vec3 diffuseColor = lightColor   * glm::vec3(0.5f); // decrease the influence
		glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f); // low influence
		lampShader.setVec3("light.ambient", ambientColor);
		lampShader.setVec3("light.diffuse", diffuseColor);
		lampShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);


		//------------------------------------------------------地球-----------------------------------------
		//---------------------------------------------------------------------------------------------------
		lampShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f); // specular lighting doesn't have full effect on this object's material
		lampShader.setFloat("material.shininess", 32.0f);
		
		lampShader.setMat4("projection", projection);
		lampShader.setMat4("view", view);

		glm::vec3 earthPos(3.0f, 0.0f, 0.0f);

		model = glm::rotate(model, (float)(currentFrame*0.1), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, earthPos);
		model = glm::rotate(model, (float)(currentFrame*0.3), glm::vec3(0.0f, 0.0f, 1.0f));
		lampShader.setVec3("light.position", lightPos);
		lampShader.setMat4("model", model);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap1);

		//glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(GL_TRIANGLES, mfc.size(), GL_UNSIGNED_INT, 0);


		//------------------------------------------------------月亮-----------------------------------------
		//---------------------------------------------------------------------------------------------------
		
		lampShader.setInt("diffuse", 0);
		glm::vec3 moonPos(3.2f, -0.3f, 0.0f);
		model = glm::mat4(1.0f);
		model = glm::rotate(model, (float)(currentFrame*0.1), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, moonPos);
		model = glm::rotate(model, (float)(currentFrame*0.3), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // a smaller cube

		lampShader.setMat4("model", model);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap2);

		//glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
		glDrawElements(GL_TRIANGLES, mfc.size(), GL_UNSIGNED_INT, 0);


		//------------------------------------------------------光源-----------------------------------------
		//---------------------------------------------------------------------------------------------------
		lightShader.use();
		lightShader.setInt("diffuse", 0);
		lightShader.setMat4("projection", projection);
		lightShader.setMat4("view", view);
		model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(3.0f)); // a smaller cube

		lightShader.setMat4("model", model);

		//glBindVertexArray(lightEBO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap3);

		//glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightEBO);

		glDrawElements(GL_TRIANGLES, mfc.size(), GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &VBO);

	glfwTerminate();
	return 0;
}

unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}
