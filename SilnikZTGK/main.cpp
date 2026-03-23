#include <iostream>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Window.h"

// GLM do matematyki macierzy
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Mówimy kompilatorowi, ¿eby w tym pliku utworzy³ cia³o biblioteki stb_image
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#include "Events/WindowEvent.h"
#include "Application.h"

using namespace std;

// Deklaracje funkcji wywo³ywanych przez GLFW (tzw. callbacki)
void proccessInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Ustawienia okna
const unsigned int screen_width = 800;
const unsigned int screen_height = 600;

// Zmienne kamery i myszki
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool firstMouse = true;
GLfloat lastX = screen_width / 2.0f;
GLfloat lastY = screen_height / 2.0f;

// Czas klatki (deltaTime to ró¿nica czasu miêdzy obecn¹ a poprzedni¹ klatk¹)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);



int main()
{

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

    Application app;
    app.Run();
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

    return 0;
}


// Odczytywanie wciœniêtych klawiszy i poruszanie kamer¹
void proccessInput(GLFWwindow* window)
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

// Obliczanie przesuniêcia myszy i obracanie kamery
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
    float yoffset = lastY - ypos; // odwrócone, bo y roœnie od góry do do³u na monitorze

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Przybli¿anie (Zoom) kó³kiem myszy
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}