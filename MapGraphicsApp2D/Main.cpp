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

bool measureMode = false;

std::vector<std::pair<float, float>> dots;
float measuredDistance = 0.0f;

const float DOT_HIT_PX = 10.0f;

const float iconX0 = 0.88f, iconX1 = 0.98f, iconY0 = 0.78f, iconY1 = 0.96f;


void updateMeasuredDistance() {
    measuredDistance = 0.0f;
    for (size_t i = 1; i < dots.size(); i++) {
        // pixels =  NDC diff / 2 * image dim 
        float dxPx = (dots[i].first - dots[i - 1].first) / 2.0f * imgW;
        float dyPx = (dots[i].second - dots[i - 1].second) / 2.0f * imgH;
        measuredDistance += sqrt(dxPx * dxPx + dyPx * dyPx) * METERS_PER_PIXEL;
    }
}

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


    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        measureMode = measureMode ? false : true;
    }
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);

        float x = (float)(xpos / winW * 2.0 - 1.0);
        float y = (float)(1.0 - ypos / winH * 2.0);

        if (x >= iconX0 && x <= iconX1 && y >= iconY0 && y <= iconY1) {
            measureMode = measureMode ? false : true;
            return;
        }

        if (!measureMode) return;

        for (int i = (int)dots.size() - 1; i >= 0; i--) {
            float dxS = (x - dots[i].first) * winW / 2.0f;
            float dyS = (y - dots[i].second) * winH / 2.0f;

            if (dxS * dxS + dyS * dyS <= DOT_HIT_PX * DOT_HIT_PX) {
                dots.erase(dots.begin() + i);
                updateMeasuredDistance();
                return;
            }
        }

        dots.push_back(std::make_pair(x, y));
        updateMeasuredDistance();
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

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Map 2D App", monitor, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);


    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetMouseButtonCallback(window, mouseCallback);


    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");


    unsigned int basicShader = createShader("basic.vert", "basic.frag");
    glUseProgram(basicShader);
    glUniform1i(glGetUniformLocation(basicShader, "uTex"), 0);
    glUniform1i(glGetUniformLocation(basicShader, "uUseTex"), 1);
    glUniform1i(glGetUniformLocation(basicShader, "uUseCol"), 0);
    glUniform2f(glGetUniformLocation(basicShader, "uPos"), 0.0f, 0.0f);
    glUniform2f(glGetUniformLocation(basicShader, "uScale"), 1.0f, 1.0f);

    unsigned int mapBcg;
    unsigned int pin;
    unsigned int digits;
    unsigned int dot;
    unsigned int walkIcon;
    unsigned int rulerIcon;
    unsigned int student;

    preprocessTexture(mapBcg, "res/map_bcg3.jpg");
    preprocessTexture(pin, "res/pin1.png");
    preprocessTexture(digits, "res/digits.png");
    preprocessTexture(dot, "res/dot.png");
    preprocessTexture(walkIcon, "res/walk.png");
    preprocessTexture(rulerIcon, "res/ruler.png");
    preprocessTexture(student, "res/student.png");

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
    float prevX = 0.0, prevY = 0.0;

    //dot
    const float dh = 0.012f, dw = dh * fbh / fbw;
    float dotVertices[] = {
        -dw,  dh,  0.0f, 1.0f,
        -dw, -dh,  0.0f, 0.0f,
         dw, -dh,  1.0f, 0.0f,
         dw,  dh,  1.0f, 1.0f,
    };

    unsigned int dotVAO;
    unsigned int dotVBO;
    glGenVertexArrays(1, &dotVAO);
    glGenBuffers(1, &dotVBO);

    glBindVertexArray(dotVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dotVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(dotVertices), dotVertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // duz
    float lineVertices[] = {
        0.0f, 0.0f,
        1.0f, 1.0f,
    };

    unsigned int lineVAO;
    unsigned int lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glLineWidth(3.0f);

    ////ime, prezime i indeks (donji levi ugao, providnost se zadaje kroz rgba temena)

    const float studentH = 0.12f;
    const float studentW = studentH * (560.0f / 100.0f) * ((float)fbh / fbw);
    float studentVertices[] = {
        //   x                   y                   r     g     b     a      s     t
        -0.98f,            -0.98f + studentH,  1.0f, 1.0f, 1.0f, 0.5f,  0.0f, 1.0f,
        -0.98f,            -0.98f,             1.0f, 1.0f, 1.0f, 0.5f,  0.0f, 0.0f,
        -0.98f + studentW, -0.98f,             1.0f, 1.0f, 1.0f, 0.5f,  1.0f, 0.0f,
        -0.98f + studentW, -0.98f + studentH,  1.0f, 1.0f, 1.0f, 0.5f,  1.0f, 1.0f,
    };
    unsigned int studentVAO;
    unsigned int studentVBO;
    glGenVertexArrays(1, &studentVAO);
    glGenBuffers(1, &studentVBO);

    glBindVertexArray(studentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, studentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(studentVertices), studentVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    ////oznaka rezima hodanja

    float iconVertices[] = {
        iconX0, iconY1,  0.0f, 1.0f,
        iconX0, iconY0,  0.0f, 0.0f,
        iconX1, iconY0,  1.0f, 0.0f,
        iconX1, iconY1,  1.0f, 1.0f,
    };
    unsigned int iconVAO;
    unsigned int iconVBO;
    glGenVertexArrays(1, &iconVAO);
    glGenBuffers(1, &iconVBO);

    glBindVertexArray(iconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, iconVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(iconVertices), iconVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);

    auto drawDistance = [&](float value) {
        snprintf(distanceText, sizeof(distanceText), "%.1f m", value);
        glActiveTexture(GL_TEXTURE0);
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
    };
    auto drawStudent = [&]() {
        glUniform2f(glGetUniformLocation(basicShader, "uCenter"), 0.5f, 0.5f);
        glUniform2f(glGetUniformLocation(basicShader, "uRange"), 1.0f, 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, student);
        glUniform1i(glGetUniformLocation(basicShader, "uUseCol"), 1);
        glBindVertexArray(studentVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glUniform1i(glGetUniformLocation(basicShader, "uUseCol"), 0);
        };
    while (!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();
        float dt = (float)(frameStart - lastFrame);
        lastFrame = frameStart;
        if (!measureMode) {
            prevX = mapX;
            prevY = mapY;

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
            drawDistance(distance);

            //student
            drawStudent();

            //oznaka rezima hodanja
            glUniform2f(glGetUniformLocation(basicShader, "uCenter"), 0.5f, 0.5f);
            glUniform2f(glGetUniformLocation(basicShader, "uRange"), 1.0f, 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, walkIcon);
            glBindVertexArray(iconVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);




            glfwSwapBuffers(window);
            glfwPollEvents();


            
        }
        else {
         

            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(basicShader);

            //map
            glUniform2f(glGetUniformLocation(basicShader, "uCenter"), 0.5, 0.5);
            glUniform2f(glGetUniformLocation(basicShader, "uRange"), 1.0, 1.0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mapBcg);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            //lines
            glUniform1i(glGetUniformLocation(basicShader, "uUseTex"), 0);
            glUniform1i(glGetUniformLocation(basicShader, "uUseCol"), 1);
            glVertexAttrib4f(1, 1.0f, 0.0f, 0.0f, 1.0f);
            glBindVertexArray(lineVAO);
            for (size_t i = 1; i < dots.size(); i++) {
                glUniform2f(glGetUniformLocation(basicShader, "uPos"), dots[i - 1].first, dots[i - 1].second);
                glUniform2f(glGetUniformLocation(basicShader, "uScale"),
                    dots[i].first - dots[i - 1].first, dots[i].second - dots[i - 1].second);
                glDrawArrays(GL_LINES, 0, 2);
            }
            glUniform2f(glGetUniformLocation(basicShader, "uScale"), 1.0f, 1.0f);
            glUniform1i(glGetUniformLocation(basicShader, "uUseTex"), 1);
            glUniform1i(glGetUniformLocation(basicShader, "uUseCol"), 0);

            //dot
            glUniform2f(glGetUniformLocation(basicShader, "uCenter"), 0.5f, 0.5f);
            glUniform2f(glGetUniformLocation(basicShader, "uRange"), 1.0f, 1.0f);
            glBindTexture(GL_TEXTURE_2D, dot);
            glBindVertexArray(dotVAO);
            for (size_t i = 0; i < dots.size(); i++) {
                glUniform2f(glGetUniformLocation(basicShader, "uPos"), dots[i].first, dots[i].second);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
            glUniform2f(glGetUniformLocation(basicShader, "uPos"), 0.0f, 0.0f);

            //oznaka rezima merenja
            glBindTexture(GL_TEXTURE_2D, rulerIcon);
            glBindVertexArray(iconVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            //izmerena razdaljina
            drawDistance(measuredDistance);

            //student
            drawStudent();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

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

