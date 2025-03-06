#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textures.h"
#include <stdio.h>
#include "shaders.h"
#include <math.h>
#include <stdlib.h> // Добавлено для rand() и RAND_MAX

#define NUM_SEGMENTS 500 // Количество точек на кривой Безье
#define NUM_CURVES 20     // Количество отдельных кривых Безье

// Объявляем массив контрольных точек для кривых Безье
float controlPoints[NUM_CURVES][4][2];

// Объявляем массивы для хранения точек земли
float groundVertices[NUM_SEGMENTS * 2];
float fullGroundVertices[NUM_SEGMENTS * 2 * 2];

unsigned int groundVAO, groundVBO;

float tankX = -0.9f;  // Начальная позиция танка

// Объявления функций
void generateRandomTerrain();
void generateBezierGround();
void bezierCurve(float t, float* x, float* y, float points[4][2]);

void processInput(GLFWwindow* window) {
    float moveSpeed = 0.0002f; 

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        if (tankX > -1.0f) tankX -= moveSpeed; // Двигаем влево
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        if (tankX < 1.0f) tankX += moveSpeed; // Двигаем вправо
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        generateRandomTerrain();
        generateBezierGround();
        // Обновляем данные в буфере
        glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullGroundVertices), fullGroundVertices, GL_STATIC_DRAW);
    }
}

void updateTankPosition(float tankX, float* tankY, float* angle) {
    // Точки контакта танка (левая и правая)
    float leftX = tankX - 0.1f;
    float rightX = tankX + 0.1f;

    // Находим высоты для точек
    int curveIndexLeft = getCurveIndex(leftX);
    float segmentStartLeft = -1.0f + curveIndexLeft * (2.0f / NUM_CURVES);
    float tLeft = (leftX - segmentStartLeft) * (NUM_CURVES / 2.0f);
    tLeft = fmaxf(0.0f, fminf(1.0f, tLeft));
    float leftY;
    bezierCurve(tLeft, &leftX, &leftY, controlPoints[curveIndexLeft]);

    int curveIndexRight = getCurveIndex(rightX);
    float segmentStartRight = -1.0f + curveIndexRight * (2.0f / NUM_CURVES);
    float tRight = (rightX - segmentStartRight) * (NUM_CURVES / 2.0f);
    tRight = fmaxf(0.0f, fminf(1.0f, tRight));
    float rightY;
    bezierCurve(tRight, &rightX, &rightY, controlPoints[curveIndexRight]);

    // Средняя высота и угол
    *tankY = (leftY + rightY) / 2.0f;
    *angle = atan2(rightY - leftY, rightX - leftX);
}

void generateRandomTerrain() {
    srand((unsigned int)glfwGetTime());

    // Параметры для генерации ландшафта
    float maxHeight = 0.5f;       // Максимальная высота холмов
    float minHeight = -0.5f;      // Минимальная высота впадин
    float hillWidth = 0.4f;       // Ширина холмов
    float smoothness = 0.7f;      // Коэффициент сглаживания (0.0 - резко, 1.0 - плавно)

    for (int curve = 0; curve < NUM_CURVES; curve++) {
        if (curve == 0) {
            // Первая точка первой кривой
            controlPoints[curve][0][0] = -1.0f;
            controlPoints[curve][0][1] = -0.5f;
        }
        else {
            // Первая точка текущей кривой равна последней точке предыдущей
            controlPoints[curve][0][0] = controlPoints[curve - 1][3][0];
            controlPoints[curve][0][1] = controlPoints[curve - 1][3][1];
        }

        // Генерация средних контрольных точек
        for (int i = 1; i < 3; i++) {
            float x = controlPoints[curve][0][0] + (float)(i) * (2.0f / NUM_CURVES) * hillWidth;

            // Высота зависит от предыдущей точки и случайного отклонения
            float prevY = (i == 1) ? controlPoints[curve][0][1] : controlPoints[curve][i - 1][1];
            float y = prevY + ((float)rand() / RAND_MAX) * (maxHeight - minHeight) + minHeight;

            // Ограничиваем высоту
            y = fmaxf(minHeight, fminf(maxHeight, y));

            // Сглаживаем высоту
            if (i > 1) {
                y = smoothness * y + (1 - smoothness) * controlPoints[curve][i - 1][1];
            }

            controlPoints[curve][i][0] = x;
            controlPoints[curve][i][1] = y;
        }

        if (curve == NUM_CURVES - 1) {
            // Последняя точка последней кривой
            controlPoints[curve][3][0] = 1.0f;
            controlPoints[curve][3][1] = -0.5f;
        }
        else {
            // Последняя точка текущей кривой
            controlPoints[curve][3][0] = controlPoints[curve][0][0] + (2.0f / NUM_CURVES);

            // Высота зависит от предыдущей точки и случайного отклонения
            float y = controlPoints[curve][2][1] + ((float)rand() / RAND_MAX) * (maxHeight - minHeight) + minHeight;
            y = fmaxf(minHeight, fminf(maxHeight, y));

            // Сглаживаем высоту
            y = smoothness * y + (1 - smoothness) * controlPoints[curve][2][1];

            controlPoints[curve][3][1] = y;
        }
    }
}

void generateBezierGround() {
    int pointsPerCurve = NUM_SEGMENTS / NUM_CURVES;
    int index = 0;

    for (int curve = 0; curve < NUM_CURVES; curve++) {
        for (int i = 0; i < pointsPerCurve; i++) {
            float t = (float)i / (pointsPerCurve - 1);
            bezierCurve(t, &groundVertices[index * 2], &groundVertices[index * 2 + 1], controlPoints[curve]);
            index++;
        }
    }
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        fullGroundVertices[i * 4] = groundVertices[i * 2];
        fullGroundVertices[i * 4 + 1] = groundVertices[i * 2 + 1];
        fullGroundVertices[i * 4 + 2] = groundVertices[i * 2];
        fullGroundVertices[i * 4 + 3] = -1.0f;
    }
}

// Функция для вычисления точки на кривой Безье
void bezierCurve(float t, float* x, float* y, float points[4][2]) {
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    *x = uuu * points[0][0] + 3 * uu * t * points[1][0] +
        3 * u * tt * points[2][0] + ttt * points[3][0];

    *y = uuu * points[0][1] + 3 * uu * t * points[1][1] +
        3 * u * tt * points[2][1] + ttt * points[3][1];
}

// Остальной код остаётся без изменений...

int getCurveIndex(float x) {
    float segmentLength = 2.0f / NUM_CURVES;
    int index = (x + 1.0f) / segmentLength; // Правильный расчёт
    if (index >= NUM_CURVES) index = NUM_CURVES - 1;
    if (index < 0) index = 0;
    return index;
}

int main() {
    if (!glfwInit()) {
        printf("Ошибка инициализации GLFW\n");
        return -1;
    }

    // Настройки контекста OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Pocket Tanks", NULL, NULL);
    if (!window) {
        printf("Ошибка создания окна\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Ошибка инициализации GLAD\n");
        return -1;
    }

    // Включение блендинга и текстур
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Загрузка текстуры
    GLuint tankTexture = loadTexture("tank.png");
    if (!tankTexture) {
        printf("Ошибка загрузки текстуры танка\n");
        return -1;
    }

    float tankVertices[] = {
       -0.1f,  0.0f, 0.0f, 1.0f,  // Нижний левый
        0.1f,  0.0f, 1.0f, 1.0f,  // Нижний правый
        0.1f,  0.15f, 1.0f, 0.0f,  // Верхний правый
       -0.1f,  0.15f, 0.0f, 0.0f   // Верхний левый
    };


    unsigned int tankIndices[] = { 0, 1, 2, 2, 3, 0 };

    // Настройка VAO/VBO для танка
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tankVertices), tankVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tankIndices), tankIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Генерируем кривую Безье
    generateRandomTerrain(); 
    generateBezierGround();
    unsigned int groundVAO, groundVBO;
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);

    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Создание шейдеров
    GLuint shaderProgram = createShaderProgram();
    GLuint groundShaderProgram = createGroundShaderProgram();

 
    // Основной цикл
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // В основном цикле
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Обновляем позицию и угол наклона танка
        float tankY, angle;
        updateTankPosition(tankX, &tankY, &angle);

       
        // Очищаем экран
        glClear(GL_COLOR_BUFFER_BIT);

        // Рендерим землю
        glUseProgram(groundShaderProgram);
        glBindVertexArray(groundVAO);
        glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullGroundVertices), fullGroundVertices, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, NUM_SEGMENTS * 2);

        // Рендерим танк
        glUseProgram(shaderProgram);
        glUniform2f(glGetUniformLocation(shaderProgram, "tankPosition"), tankX, tankY);
        glUniform1f(glGetUniformLocation(shaderProgram, "tankAngle"), angle);
        glBindTexture(GL_TEXTURE_2D, tankTexture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Проверьте значения:
        printf("Tank Y: %f, Angle: %f\n", tankY, angle); // Добавьте отладочный вывод

        // Передача в шейдер:
        glUniform2f(glGetUniformLocation(shaderProgram, "tankPosition"), tankX, tankY);
        glUniform1f(glGetUniformLocation(shaderProgram, "tankAngle"), angle);


        // Обновляем экран
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
