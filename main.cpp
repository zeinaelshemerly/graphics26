#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <iostream>

#include "Renderer2D.h"
#include "GameUI.h"

static GameUI* g_Playfield = nullptr;

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
        g_Playfield->Mouse(virtualX, virtualY);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_Playfield) return;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    cursor_position_callback(window, x, y);
    g_Playfield->Click(button, action);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Kitchen Defense", nullptr, nullptr);
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

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int) {
        if (g_Playfield) g_Playfield->Key(key, action);
    });
    glfwSetWindowFocusCallback(window, [](GLFWwindow*, int focused) {
        if (!focused && g_Playfield) g_Playfield->FocusLost();
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

    GameUI playfield;
    playfield.Init();
    g_Playfield = &playfield;

    std::cout << "Kitchen Defense: Select lanes, then click a card. W: jump, S: duck, ESC: pause.\n";

    double lastTime = glfwGetTime();

    // Main Game Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Update playfield animations and logic
        // Refresh coordinates even when a resize occurs without mouse movement.
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        cursor_position_callback(window, mouseX, mouseY);
        playfield.Update(deltaTime);
        if (playfield.ShouldQuit()) glfwSetWindowShouldClose(window, GLFW_TRUE);

        // Clear screen
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render 2D Kitchen Playfield & Environment
        renderer.Begin(1280, 720);
        playfield.Render(renderer);
        renderer.End();

        glfwSwapBuffers(window);
    }

    g_Playfield = nullptr;
    renderer.Cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}