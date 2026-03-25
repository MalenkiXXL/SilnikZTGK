//#include "glad/glad.h" 
//#include <GLFW/glfw3.h>
//#include <iostream>
//#include "Shader.h"
//#include "Camera.h"
////GLM
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#define PI 3.14159265
//
//#define STB_IMAGE_IMPLEMENTATION 
//#include "stb_image.h"
//
//using namespace std;
//
//
////zmiana rozimaru renderingu przy powiekszaniu/zmniejszaniu okna
//void framebuffer_size_callback(GLFWwindow* window, int width, int height);
////wykrywanie wcisniecia przycisku w tym przypadku esc
//void proccessInput(GLFWwindow* window);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//
////ustawienia co do wielkosci okna
//const unsigned int screen_width = 800;
//const unsigned int screen_height = 600;
//
//bool firstMouse = true;
//
//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
////glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
////glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
////glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
//
//GLfloat fov = 45.0f;
//
//GLfloat deltaTime = 0.0f;
//GLfloat lastFrame = 0.0f;
//
//GLfloat lastX = 800.0f / 2.0f;
//GLfloat lastY = 600.0f / 2.0f;
//
//
//int main()
//{
//	//specyfikacje co do uzwanego glfw
//	glfwInit();
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//
//
//	//tworzenie okna
//	GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "FirstWindow", NULL, NULL);
//	if (window == NULL)
//	{
//		cout << "Failed to create a window" << endl;
//		//funkcja ktora niszczy wszystkie istniejace okna
//		glfwTerminate();
//		return -1;
//	}
//
//	//ustawiamy glowny context jako nasze okno
//	glfwMakeContextCurrent(window);
//	//wykrywanie zmiany rozmiaru okna
//	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
//
//	//sprawdzanie czy poprawnie mamy zainiconowane glad
//	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
//	{
//		cout << "Failed to initilize GLAD" << endl;
//		return -1;
//	}
//
//	Shader ourShader("shader.vs", "shader.frag");
//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//	glEnable(GL_DEPTH_TEST);
//
//	////id dla vertex shadera
//	//unsigned int vertexShader;
//	////tworzy obiekt shadera
//	//vertexShader = glCreateShader(GL_VERTEX_SHADER);
//	////przyczepiamy shader source do obiektu
//	//glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
//	////kompiluje podany obiekt shadera
//	//glCompileShader(vertexShader);
//
//	////int dla sprawdzenia wgrania shaderow
//	//int success;
//	////char dla bledu przy nie wgraniu shaderow
//	//char infoLog[512];
//
//
//	////pobiramy parametry z obiektu shadera w tym przypadku "GL_COMPILE_STATUS"
//	//glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//
//	//if (!success)
//	//{
//	//	//zwraca information log z danego obiektu shadera
//	//	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
//	//	cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
//	//}
//
//	////id dla fragment Shadera
//	//unsigned int fragmentShader;
//	////analogicznie jak dla vertex shadera -> kompilujemy fragment shader
//	//fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//	//glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
//	//glCompileShader(fragmentShader);
//
//	//glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
//
//	//if (!success)
//	//{
//	//	glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
//	//	cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
//	//}
//	////id dla program shadera
//	//unsigned int shaderProgram;
//	////tworzy pusty Shader Program. Shader Program to ostateczny obiekt, który "zszywa" Vertex i Fragment shader w jedn¹ ca³oœæ
//	//shaderProgram = glCreateProgram();
//	////przylaczamy wczensniej utworzone obiekty shaderow 
//	//glAttachShader(shaderProgram, vertexShader);
//	//glAttachShader(shaderProgram, fragmentShader);
//	////linkujemy je
//	//glLinkProgram(shaderProgram);
//
//	//glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
//
//	//if (!success)
//	//{
//	//	glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
//	//	cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << infoLog << endl;
//	//}
//	////usuwamy obiekty shaderow skoro je juz zlaczylismy
//	//glDeleteShader(vertexShader);
//	//glDeleteShader(fragmentShader);
//
//	//punkty do wyswietlenia
//	float vertices[] =
//	{
//		//trojkat
//		//-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, //pozycja(3 floaty), kolor(3 floaty)
//		//0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
//		//0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
//
//		//prostokat
//		//pierwszy trojkat
//		//0.5f, 0.5f, 0.0f, // prawy gorny rog
//		//0.5f, -0.5f, 0.0f, //prawy dolny rog
//		//-0.5f, 0.5f, 0.0f, //lewy gorny rog
//		////drugi trojkat
//		//0.5f, -0.5f, 0.0f, //prway dolny rog
//		//-0.5f, -0.5f, 0.0f, //lewy dolny rog
//		//-0.5f, 0.5f, 0.0f, //lewy gorny rog
//
//		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,// prawy gorny rog
//		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //prawy dolny rog
//		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,//lewy dolny rog
//		-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,//lewy gorny rog
//	};
//
//	float cube[] =
//	{
//		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
//		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
//		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
//		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
//		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
//		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
//
//		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
//		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
//		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
//		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
//		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
//		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
//
//		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
//		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
//		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
//		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
//		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
//		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
//
//		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
//		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
//		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
//		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
//		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
//		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
//
//		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
//		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
//		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
//		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
//		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
//		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
//
//		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
//		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
//		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
//		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
//		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
//		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
//	};
//
//	glm::vec3 cubePositions[] = {
//		glm::vec3(0.0f,  0.0f,  0.0f),
//		glm::vec3(2.0f,  5.0f, -15.0f),
//		glm::vec3(-1.5f, -2.2f, -2.5f),
//		glm::vec3(-3.8f, -2.0f, -12.3f),
//		glm::vec3(2.4f, -0.4f, -3.5f),
//		glm::vec3(-1.7f,  3.0f, -7.5f),
//		glm::vec3(1.3f, -2.0f, -2.5f),
//		glm::vec3(1.5f,  2.0f, -2.5f),
//		glm::vec3(1.5f,  0.2f, -1.5f),
//		glm::vec3(-1.3f,  1.0f, -1.5f)
//	};
//
//	float texCords[] =
//	{
//		0.0f, 0.0f, //lewy dol tekstury
//		1.0f, 0.0f, // prawy dol tekstury
//		0.5f, 1.0f //gora tekstury
//	};
//
//	unsigned int indices[] =
//	{
//		0,1,3, //pierwszy trójk¹t
//		1,2,3, // drugi trójk¹t
//	};
//
//	//id dla vertex buffora 
//	unsigned int VBO;
//	//id dal vao
//	unsigned int VAO;
//	//id dal ebo
//	unsigned int EBO;
//
//	//generowanie unikalnych ID dla obiektów
//	glGenVertexArrays(1, &VAO);
//	glGenBuffers(1, &VBO);
//	glGenBuffers(1, &EBO);
//
//	glBindVertexArray(VAO);
//	//nadanie typu bufforowi
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	//przesylamy dane do buffora
//	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
//
//	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
//
//	//powiemy openglowi jak interpretowac dane
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
//
//	//pierwszy argument specyfikuje ktory atrybut vertexu chcemy konfigurowac
//	//drugi - wielkosc atrybuty vertexa
//	//trzeci - typ danych jaki bedziemy podawac
//	//czwarty - czy chcemy znormalizowac dane
//	//piaty - tak zwany "stride", mowi miejsce pomiedzy atrybutami vertexa
//	//szosty - typ void*, okresla gdzie pozycja danych zaczyna sie w bufferze
//	glEnableVertexAttribArray(0);
//
//	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
//	//glEnableVertexAttribArray(1);
//
//	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
//	glEnableVertexAttribArray(2);
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
//
//	glBindVertexArray(0);
//
//	unsigned int texture1;
//	unsigned int texture2;
//	glGenTextures(1, &texture1);
//	glBindTexture(GL_TEXTURE_2D, texture1);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	int width, height, nrChannels;
//	stbi_set_flip_vertically_on_load(true);
//	unsigned char* data = stbi_load("texture.png", &width, &height, &nrChannels, 0);
//	if (data)
//	{
//		GLenum format;
//		if (nrChannels == 3)
//			format = GL_RGB;
//		else if (nrChannels == 4)
//			format = GL_RGBA;
//		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
//		glGenerateMipmap(GL_TEXTURE_2D);
//	}
//	else
//	{
//		cout << "FAILED TO LOAD TEXTURE" << endl;
//	}
//	stbi_image_free(data);
//
//	glGenTextures(1, &texture2);
//	glBindTexture(GL_TEXTURE_2D, texture2);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	data = stbi_load("texture2.png", &width, &height, &nrChannels, 0);
//	if (data)
//	{
//		GLenum format;
//		if (nrChannels == 3)
//			format = GL_RGB;
//		else if (nrChannels == 4)
//			format = GL_RGBA;
//
//		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
//		glGenerateMipmap(GL_TEXTURE_2D);
//	}
//	else
//	{
//		cout << "FAILED TO LOAD TEXTURE" << endl;
//	}
//	stbi_image_free(data);
//
//
//
//	//tryb wireframe
//	//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
//
//	ourShader.use();
//	ourShader.setInt("texture1", 0);
//	ourShader.setInt("texture2", 1);
//
//	//na poczatku kazdej petli sprawdza czy glfw zostalo poinstruowane do zamkniecia
//	while (!glfwWindowShouldClose(window))
//	{
//		//wcisniecie przycisku esc
//		proccessInput(window);
//		GLfloat currentFrame = glfwGetTime();
//		deltaTime = currentFrame - lastFrame;
//		lastFrame = currentFrame;
//
//		glfwSetCursorPosCallback(window, mouse_callback);
//
//		//ustawiamy color "sceny"
//		glClearColor(0.5f, 0.6f, 0.1f, 1.0f);
//		//czyscimy buffor dotyczacy koloru i glebokosci
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//
//
//		////aktywujemy program shader
//		//glUseProgram(shaderProgram);
//
//		//GLfloat timeValue = glfwGetTime();
//		//GLfloat greenValue = (sin(timeValue) / 2) + 0.5f;
//		//GLint vertexColorLocation = glad_glGetUniformLocation(ourShader.ID, "ourColor");
//		//glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
//
//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_2D, texture1);
//		glActiveTexture(GL_TEXTURE1);
//		glBindTexture(GL_TEXTURE_2D, texture2);
//		ourShader.use();
//
//
//		//glm::vec3 cameraPos = glm::vec3(1.0f);
//		//cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
//
//		//glm::vec3 cameraTarget = glm::vec3(1.0f);
//		//cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
//
//
//		//glm::vec3 cameraDirection = glm::vec3(1.0f);
//		//cameraDirection = glm::vec3(cameraPos - cameraTarget);
//
//		//glm::vec3 up = glm::vec3(1.0f);
//		//up = glm::vec3(0.0f, 1.0f, 0.0f);
//
//		//glm::vec3 cameraRight = glm::vec3(1.0f);
//		//cameraRight = glm::vec3(glm::cross(up, cameraDirection));
//
//		//glm::vec3 cameraUp = glm::vec3(1.0f);
//		//cameraUp = glm::vec3(glm::cross(cameraDirection, cameraRight));
//
//
//
//
//
//		//glm::mat4 transform = glm::mat4(1.0f);
//		//transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.0f));
//		//transform = glm::rotate(transform, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
//		//transform = glm::scale(transform, glm::vec3(0.5f, 0.5f, 1.0f));
//
//		glm::mat4 model = glm::mat4(1.0f);
//		model = glm::rotate(model, float(glfwGetTime()) * glm::radians(50.0f), glm::vec3(0.5, 1.0f, 0.0f));
//		glm::mat4 view = glm::mat4(1.0f);
//		view = camera.GetViewMatrix();
//		glm::mat4 projection = glm::mat4(1.0f);
//		projection = glm::perspective(glm::radians(fov), (float)screen_width / (float)screen_height, 0.1f, 100.0f);
//
//		GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
//		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
//
//		GLint viewLoc = glGetUniformLocation(ourShader.ID, "view");
//		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
//
//		GLint projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
//		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
//
//
//		//unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
//		//glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
//		//bindujemy vao ponownie
//		glBindVertexArray(VAO);
//		for (unsigned int i = 0; i < 10; i++)
//		{
//			glm::mat4 model = glm::mat4(1.0f);
//			model = glm::translate(model, cubePositions[i]);
//			float rotationAngle = 20.0f * i;
//			float timeOffset = float(glfwGetTime()) * 50.0f;
//			if (i % 3 == 0 || i == 0)
//			{
//				model = glm::rotate(model, glm::radians(rotationAngle) * float(glfwGetTime()), glm::vec3(1.0f, 0.3f, 0.5f));
//			}
//			GLint modelLoc = glGetUniformLocation(ourShader.ID, "model");
//			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
//			glDrawArrays(GL_TRIANGLES, 0, 36);
//		}
//
//		////rysuejmy obiekt za pomoca DrawArrays ktore rysuje na podstawie obecnego program shadera
//		glDrawArrays(GL_TRIANGLES, 0, 36);
//
//		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//
//
//		//DRUGI OBIEKT
//		//transform = glm::mat4(1.0f);
//		//transform = glm::translate(transform, glm::vec3(-0.5f, 0.5f, 0.0f));
//		//double angle = sin(glfwGetTime());
//		//transform = glm::scale(transform, glm::vec3(angle, 1.0f, 1.0f));
//		//glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
//		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
//
//		glBindVertexArray(0);
//
//		glfwSetScrollCallback(window, scroll_callback);
//
//
//		//zamiast wyswietlac buffor na ktorym po ulamku sekundy rysowane sa obiekty, zamienia go na buffer na ktorym wszystko co powinno zostalo narysowane
//		glfwSwapBuffers(window);
//		//sprawdza czy zostal wywolany jakis event (np klikniecie myszy albo uzycie klawiatury), zmienia stan okna, i odwoluje sie do funkcji (mozna np zarejestrowac callback)
//		glfwPollEvents();
//
//
//	}
//	glDeleteVertexArrays(1, &VAO);
//	glDeleteBuffers(1, &VBO);
//	glDeleteBuffers(1, &EBO);
//	glDeleteProgram(ourShader.ID);
//	glfwTerminate();
//	return 0;
//}
//
//void framebuffer_size_callback(GLFWwindow* window, int width, int height)
//{
//	glViewport(0, 0, width, height);
//}
//
//void proccessInput(GLFWwindow* window)
//{
//	float cameraSpeed = 3.0f * deltaTime;
//	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//	{
//		glfwSetWindowShouldClose(window, true);
//	}
//	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//	{
//		camera.ProcessKeyboard(FORWARD, deltaTime);
//		;
//	}
//	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//	{
//		camera.ProcessKeyboard(BACKWARD, deltaTime);
//	}
//	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//	{
//		camera.ProcessKeyboard(LEFT, deltaTime);
//	}
//	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//	{
//		camera.ProcessKeyboard(RIGHT, deltaTime);
//	}
//}
//
//void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
//{
//	float xpos = static_cast<float>(xposIn);
//	float ypos = static_cast<float>(yposIn);
//
//	if (firstMouse)
//	{
//		lastX = xpos;
//		lastY = ypos;
//		firstMouse = false;
//	}
//
//	float xoffset = xpos - lastX;
//	float yoffset = lastY - ypos;
//
//	lastX = xpos;
//	lastY = ypos;
//
//	camera.ProcessMouseMovement(xoffset, yoffset);
//}
//
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
//{
//	camera.ProcessMouseScroll(static_cast<float>(yoffset));
//}


//#include "Shader.h"
//#include "Camera.h"
//#include "Model.h"
//#include "Window.h"
//
//// GLM do matematyki macierzy
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
//// Mówimy kompilatorowi, ¿eby w tym pliku utworzy³ cia³o biblioteki stb_image
//#define STB_IMAGE_IMPLEMENTATION 
//#include "stb_image.h"
//
//#include "Events/WindowEvent.h"
//
//
//using namespace std;
//
//// Deklaracje funkcji wywo³ywanych przez GLFW (tzw. callbacki)
//void proccessInput(GLFWwindow* window);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//
//// Ustawienia okna
//const unsigned int screen_width = 800;
//const unsigned int screen_height = 600;
//
//// Zmienne kamery i myszki
//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
//bool firstMouse = true;
//GLfloat lastX = screen_width / 2.0f;
//GLfloat lastY = screen_height / 2.0f;
//
//// Czas klatki (deltaTime to ró¿nica czasu miêdzy obecn¹ a poprzedni¹ klatk¹)
//GLfloat deltaTime = 0.0f;
//GLfloat lastFrame = 0.0f;
//
//glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);


// Informujemy stb_image, ¿eby odwraca³ tekstury na osi Y (OpenGL oczekuje punktu 0.0 na dole, a obrazki maj¹ go na górze)
//stbi_set_flip_vertically_on_load(true);

// W³¹czamy testowanie g³êbokoœci (¿eby obiekty z ty³u nie rysowa³y siê na obiektach z przodu)
//glEnable(GL_DEPTH_TEST);

// 5. Budowanie shaderów i wczytanie modelu
//Shader ourShader("shader.vs", "shader.frag");

// Wczytujemy model. Upewnij siê, ¿e plik char.obj le¿y w folderze Twojego projektu!
//Model ourModel("BR_Charizard.obj");

// Opcjonalnie: odkomentuj, by widzieæ siatkê (tryb wireframe)
// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

/*  myWindow.SetEventCallback(EventReciever);*/

//// 6. G£ÓWNA PÊTLA GRY
    //while (!myWindow.ShouldClose())
    //{
    //    // Obliczanie czasu klatki (deltaTime) do p³ynnego ruchu
    //    GLfloat currentFrame = glfwGetTime();
    //    deltaTime = currentFrame - lastFrame;
    //    lastFrame = currentFrame;

    //    //// Sprawdzanie klawiatury
    //    //proccessInput(window);

    //    // Czyszczenie ekranu i bufora g³êbokoœci na podany kolor (ciemnoszary)
    //    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //    // Uruchamiamy nasz shader dla modelu
    //    ourShader.use();

    //    // Obliczanie i wysy³anie macierzy Widoku i Projekcji
    //    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screen_width / (float)screen_height, 0.1f, 100.0f);
    //    glm::mat4 view = camera.GetViewMatrix();
    //    ourShader.setMat4("projection", projection);
    //    ourShader.setMat4("view", view);

    //    // Ustawianie macierzy Modelu (Pozycja, Obrót, Skala)
    //    glm::mat4 model = glm::mat4(1.0f);
    //    model = glm::translate(model, glm::vec3(0.0f, -1.0f, -3.0f)); // Przesuwamy model lekko w dó³ i w g³¹b ekranu
    //    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));       // Dostosuj tê skalê, jeœli twój char.obj jest gigantyczny lub mikroskopijny
    //    ourShader.setMat4("model", model);

    //    // FAKTYCZNE RYSOWANIE MODELU
    //    ourModel.Draw(ourShader);

    //    // Zamiana buforów i przetwarzanie zdarzeñ systemu Windows
    //    myWindow.OnUpdate();
    //}


// Odczytywanie wciœniêtych klawiszy i poruszanie kamer¹
//void proccessInput(GLFWwindow* window)
//{
//    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//        glfwSetWindowShouldClose(window, true);
//
//    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//        camera.ProcessKeyboard(FORWARD, deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//        camera.ProcessKeyboard(BACKWARD, deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//        camera.ProcessKeyboard(LEFT, deltaTime);
//    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//        camera.ProcessKeyboard(RIGHT, deltaTime);
//}
//
//// Obliczanie przesuniêcia myszy i obracanie kamery
//void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
//{
//    float xpos = static_cast<float>(xposIn);
//    float ypos = static_cast<float>(yposIn);
//
//    if (firstMouse)
//    {
//        lastX = xpos;
//        lastY = ypos;
//        firstMouse = false;
//    }
//
//    float xoffset = xpos - lastX;
//    float yoffset = lastY - ypos; // odwrócone, bo y roœnie od góry do do³u na monitorze
//
//    lastX = xpos;
//    lastY = ypos;
//
//    camera.ProcessMouseMovement(xoffset, yoffset);
//}
//
//// Przybli¿anie (Zoom) kó³kiem myszy
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
//{
//    camera.ProcessMouseScroll(static_cast<float>(yoffset));
//}