#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <iostream>

#include "Renderer2D.h"
#include "Playfield.h"

static Playfield* g_Playfield = nullptr;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!g_Playfield) return;
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    if (winW > 0 && winH > 0) {
        float virtualX = (float)xpos * (1280.0f / (float)winW);
        float virtualY = (float)ypos * (720.0f / (float)winH);
        g_Playfield->UpdateMouse(virtualX, virtualY);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_Playfield) return;
    g_Playfield->OnMouseClick(button, action);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "DMET 502 - Kitchen Defense | Person 2: Game World & Progression", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context with GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, 1280, 720);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(w, GLFW_TRUE);
        // R = Restart game world (Person 2)
        if (g_Playfield)
            g_Playfield->OnKey(key, action);
    });

    // Enable V-Sync to cap to 60 FPS
    glfwSwapInterval(1);

    // Make window prominent and focused
    glfwShowWindow(window);
    glfwFocusWindow(window);

    // Enable Alpha Blending for smooth highlights, shadows and glows
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Renderer2D renderer;
    renderer.Init();

    Playfield playfield;
    playfield.Init();
    g_Playfield = &playfield;

    std::cout << "================================================\n";
    std::cout << " DMET 502 — Kitchen Defense                   \n";
    std::cout << " Person 2: Game World & Progression           \n";
    std::cout << "================================================\n";
    std::cout << "[INFO] Window opened (1280x720)\n";
    std::cout << "[INFO] Select a card in the top bar\n";
    std::cout << "[INFO] Click a lane to deploy a character\n";
    std::cout << "[INFO] Click dropped coins to collect them\n";
    std::cout << "[INFO] Press R to restart | ESC to exit\n";
    std::cout << "================================================\n" << std::flush;

    double lastTime = glfwGetTime();

    // Main Game Loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Update playfield animations and logic
        playfield.Update(deltaTime);

        // Clear screen
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render 2D Kitchen Playfield & Environment
        renderer.Begin(1280, 720);
        playfield.Render(renderer);
        renderer.End();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g_Playfield = nullptr;
    renderer.Cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}