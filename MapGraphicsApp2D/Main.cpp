#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include "Util.h"
#define _USE_MATH_DEFINES
#include <math.h>

#include <chrono>
#include <thread>

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "stb_image.h"
#include <cstdio>




int imgW, imgH, imgComp;

const double TARGET_FPS = 75.0;
const double FRAME_TIME = 1.0 / TARGET_FPS;

const float METERS_PER_PIXEL = 3.3f;

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

float mapX = 0.0f, mapY = 0.0f;


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}


void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);


    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void computeMapView(int screenW, int screenH, float area, float& rS, float& rT) {
    //rS*W / rT*H = Ws / Hs
    // eq. to rS / rT = Ws / Hs * (H / W)
    float k = ((float)screenW / screenH) * ((float)imgH / imgW);
    rT = sqrt(area / k);
    rS = k * rT;
    if (rS > 1.0f) { rS = 1.0f; rT = rS / k; }
    if (rT > 1.0f) { rT = 1.0f; rS = k * rT; }
}

int main()
{
    stbi_info("res/map_bcg3.jpg", &imgW, &imgH, &imgComp);
    timeBeginPeriod(1);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Vezba 4", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);


    glfwSetKeyCallback(window, keyboardCallback);


    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");


    unsigned int basicShader = createShader("basic.vert", "basic.frag");
    glUseProgram(basicShader);
    glUniform1i(glGetUniformLocation(basicShader, "uTex"), 0);


    unsigned int mapBcg;
    unsigned int pin;
    unsigned int digits;

    preprocessTexture(mapBcg, "res/map_bcg3.jpg");
    preprocessTexture(pin, "res/pin1.png");
    preprocessTexture(digits, "res/digits.png");

    //map
    float vertices[] = {
        //   x      y     s     t
           -1.0f,  1.0f,  0.0f, 1.0f,
           -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f,
    };
    unsigned int VAO;
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    ////pin

    float pinVertices[] = {
        //   x      y       s     t
           -0.03f,  0.045f,   0.0f, 1.0f,
           -0.03f, -0.045f,   0.0f, 0.0f,
            0.03f, -0.045f,   1.0f, 0.0f,
            0.03f,  0.045f,   1.0f, 1.0f,
    };
    unsigned int pinVAO;
    unsigned int pinVBO;
    glGenVertexArrays(1, &pinVAO);
    glGenBuffers(1, &pinVBO);

    glBindVertexArray(pinVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pinVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pinVertices), pinVertices, GL_STATIC_DRAW);


    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glClearColor(0.5f, 0.6f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double fpsTimer = glfwGetTime();
    int frameCount = 0;


    double lastFrame = glfwGetTime();
    const float panSpeed = 0.1f;

    float visibleArea = 0.1f;
    float mapX = 0.5f, mapY = 0.5f;

    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);

    float rS, rT;
    computeMapView(fbw, fbh, visibleArea, rS, rT);


    ////razdaljina (atlas sa ciframa "0123456789.m")

    const int ATLAS_CHARS = 12;
    const int MAX_CHARS = 10;
    const float charH = 0.08f;
    const float charW = charH * (56.0f / 80.0f) * ((float)fbh / fbw);
    const float startX = -0.97f, startY = 0.95f;

    float digitVertices[MAX_CHARS * 16];
    for (int i = 0; i < MAX_CHARS; i++) {
        float x0 = startX + i * charW, x1 = x0 + charW;
        float y0 = startY - charH, y1 = startY;
        float quad[16] = {
            x0, y1,  0.0f, 1.0f,
            x0, y0,  0.0f, 0.0f,
            x1, y0,  1.0f, 0.0f,
            x1, y1,  1.0f, 1.0f,
        };
        for (int j = 0; j < 16; j++) digitVertices[i * 16 + j] = quad[j];
    }
    unsigned int digitVAO;
    unsigned int digitVBO;
    glGenVertexArrays(1, &digitVAO);
    glGenBuffers(1, &digitVBO);

    glBindVertexArray(digitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, digitVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(digitVertices), digitVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float distance = 0.0f;
    char distanceText[32];
    while (!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();
        float dt = (float)(frameStart - lastFrame);
        lastFrame = frameStart;

        float prevX = mapX, prevY = mapY;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) mapY += panSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) mapY -= panSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) mapX += panSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) mapX -= panSpeed * dt;

        

        mapX = max(rS / 2, min(mapX, 1.0f - rS / 2));
        mapY = max(rT / 2, min(mapY, 1.0f - rT / 2));

        //predjena razdaljina (racuna se posle ogranicenja, pa se ne broji guranje u ivicu)
        float dxPx = (mapX - prevX) * imgW;
        float dyPx = (mapY - prevY) * imgH;
        distance += sqrt(dxPx * dxPx + dyPx * dyPx) * METERS_PER_PIXEL;


        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(basicShader);

        //map
        glUniform2f(glGetUniformLocation(basicShader, "uCenter"), mapX, mapY);
        glUniform2f(glGetUniformLocation(basicShader, "uRange"), rS, rT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapBcg);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        //pin
        glUniform2f(glGetUniformLocation(basicShader, "uCenter"), 0.5f, 0.5f);
        glUniform2f(glGetUniformLocation(basicShader, "uRange"), 1.0f, 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pin);
        
        glBindVertexArray(pinVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


        //razdaljina
        snprintf(distanceText, sizeof(distanceText), "%.1f m", distance);
        glBindTexture(GL_TEXTURE_2D, digits);
        glBindVertexArray(digitVAO);
        for (int i = 0; i < MAX_CHARS && distanceText[i] != '\0'; i++) {
            char c = distanceText[i];
            int idx;
            if (c >= '0' && c <= '9') idx = c - '0';
            else if (c == '.') idx = 10;
            else if (c == 'm') idx = 11;
            else continue; // razmak - samo preskoci mesto

            glUniform2f(glGetUniformLocation(basicShader, "uCenter"), (idx + 0.5f) / ATLAS_CHARS, 0.5f);
            glUniform2f(glGetUniformLocation(basicShader, "uRange"), 1.0f / ATLAS_CHARS, 1.0f);
            glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
        }




        glfwSwapBuffers(window);
        glfwPollEvents();


        double target = frameStart + FRAME_TIME;
        while (glfwGetTime() < target) {
            double left = target - glfwGetTime();
            if (left > 0.002)
                std::this_thread::sleep_for(std::chrono::duration<double>(left - 0.002));
        }



        //ispis fps-a
        frameCount++;
        double now = glfwGetTime();
        if (now - fpsTimer >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            fpsTimer = now;
        }
    }

    timeEndPeriod(1);
    glfwTerminate();
    return 0;
}

