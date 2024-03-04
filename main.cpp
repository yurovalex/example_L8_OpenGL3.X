#include <GL/glew.h>

#include "GLFW/glfw3.h"
#include <glm/glm.hpp>

#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>


class guiDialog
{
public:
    guiDialog(GLFWwindow *w);
    void CreateGuiDialog();
    void ShowGui();
    auto GetParam() const
    {
        return objPar;
    }
    virtual ~guiDialog();
private:
    GLFWwindow* window;

    struct PropertiesObject
    {
        PropertiesObject(float a, float b, float c, int d ):
            valueRed(a), valueGreen(b), valueBlue(c), valueThickness(d)
        {}
        float valueRed;
        float valueGreen;
        float valueBlue;
        int valueThickness;
    };
    PropertiesObject objPar;
};

guiDialog::guiDialog(GLFWwindow *w):window(w), objPar{0.1,0.5,0.0,2}
{
    //Инициализация интерфейса ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void guiDialog::CreateGuiDialog()
{
    //Запуск нового окна ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    //Элементы интерфейса в окне
    if (ImGui::Begin("Control propeties")) {
        ImGui::Text("Create a point's array");
        ImGui::SliderFloat("Red", &objPar.valueRed, 0.0f, 1.0f);
        ImGui::SliderFloat("Green", &objPar.valueGreen, 0.0f, 1.0f);
        ImGui::SliderFloat("Blue", &objPar.valueBlue, 0.0f, 1.0f);
        ImGui::SliderInt("Thickness", &objPar.valueThickness, 1, 50);
    }
    ImGui::End();

}

void guiDialog::ShowGui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

guiDialog::~guiDialog()
{
    //Освобождение ресурсов ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

class BaseOpenGL
{
//Шейдерная часть
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 position;
    uniform int vThickness;
    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
        gl_PointSize = vThickness;
        // if (position.x == 0.0f && position.y == 0.0f)
        // {
        //       gl_PointSize = 10;
        // }
    }
)";
    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform float vRed;
    uniform float vGreen;
    uniform float vBlue;
    void main()
    {
        FragColor = vec4(vRed, vGreen, vBlue, 1.0);
    }
)";

public:
    BaseOpenGL() = default;
    ~BaseOpenGL()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    //Инициализация шейдеров и создание шейдерной программмы
    void InitShaders()
    {
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
    }
    //Инициализация контейнера
    void InitDataPoints()
    {
        points = {0.0f,0.0f , 0.5f,0.5f, -0.5f,0.5f,  0.5f,-0.5f, -0.7f,-0.7f};
    }
    //Добавить точку в контейнер
    bool AddPointXY(float _x, float _y)
    {
        if (!(_x >= -1 && _x <= 1) || !(_y >= -1 && _y <= 1))
        {
            return false;
        }
        points.push_back(_x);
        points.push_back(_y);
        return true;
    }
    //Очистка контейнера с точками
    void ClearDataPoints()
    {
        points.clear();
    }

    //Получить адрес шейдерной программы
    GLuint&  GetShadersProgram()
    {
        return shaderProgram;
    }
    //Получить адрес VAO
    GLuint&  GetVAO()
    {
        return VAO;
    }

    //Получить количество точек
    int GetSize()
    {
        return points.size()/2;
    }

    //Инициализация буферов VAO и VBO
    void InitVBOVAO()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * points.size(), points.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glEnable(GL_PROGRAM_POINT_SIZE);
    }

private:
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int VBO, VAO;
    unsigned int shaderProgram;
    std::vector <float> points;
};


void reshapeWindows (GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


double normalize(double value, double min, double max) {
    return (2 * (value - min) / (max - min)) - 1;
}

//Line_Bresenham
void line_Bresenham(BaseOpenGL* obj, int& width, int& height, int x1,int y1,int x2,int y2)
{
    const int dx = abs(x2 - x1);
    const int dy = abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;
    int error = dx - dy;

    double x_norm = normalize(x2, 0, width); // Normalizing x to range [-1, 1]
    double y_norm = -1 * normalize(y2, 0, height);
    obj->AddPointXY(x_norm,y_norm);

    while(x1 != x2 || y1 != y2)
    {
        x_norm = normalize(x1, 0, width); // Normalizing x to range [-1, 1]
        y_norm = -1*normalize(y1, 0, height);

        obj->AddPointXY(x_norm,y_norm);
        int error2 = error * 2;
        if(error2 > - dy)
        {
            error -= dy;
            x1 += signX;
        }
        if(error2 < dx)
        {
            error += dx;
            y1 += signY;
        }
    }
}

//Circle_Bresenham
void drawCircle(BaseOpenGL* obj, int& width, int& height, int xc, int yc, int x, int y)
{
    //(xc+x, yc+y)
    double x_norm = normalize(xc+x, 0, width); // Normalizing x to range [-1, 1]
    double y_norm = -1* normalize(yc+y, 0, height);
    obj->AddPointXY(x_norm,y_norm);
    //(xc-x, yc+y)
    x_norm = normalize(xc-x, 0, width);
    y_norm = -1* normalize(yc+y, 0, height);
    obj->AddPointXY(x_norm,y_norm);
    //(xc+x, yc-y)
    x_norm = normalize(xc+x, 0, width);
    y_norm = -1* normalize(yc-y, 0, height);
    obj->AddPointXY(x_norm,y_norm);
    //(xc-x, yc-y)
    x_norm = normalize(xc-x, 0, width);
    y_norm = -1* normalize(yc-y, 0, height);
    obj->AddPointXY(x_norm,y_norm);
    //(xc+y, yc+x)
    x_norm = normalize(xc+y, 0, width);
    y_norm = -1* normalize(yc+x, 0, height);
    obj->AddPointXY(x_norm,y_norm);

    //(xc-y, yc+x)
    x_norm = normalize(xc-y, 0, width);
    y_norm = -1* normalize(yc+x, 0, height);
    obj->AddPointXY(x_norm,y_norm);

    //(xc+y, yc-x)
    x_norm = normalize(xc+y, 0, width);
    y_norm = -1* normalize(yc-x, 0, height);
    obj->AddPointXY(x_norm,y_norm);

    //(xc-y, yc-x)
    x_norm = normalize(xc-y, 0, width);
    y_norm = -1* normalize(yc-x, 0, height);
    obj->AddPointXY(x_norm,y_norm);
}

void circle_Bresenham(BaseOpenGL* obj, int& width, int& height, int xc,int yc, int R)
{
    int x = 0, y = R;
    int d = 3 - 2 * R;
    drawCircle(obj, width, height, xc, yc, x, y);
    while (y >= x)
    {
        // for each pixel we will
        // draw all eight pixels

        x++;

        // check for decision parameter
        // and correspondingly
        // update d, x, y
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        drawCircle(obj, width, height, xc, yc, x, y);
    }
}

int main()
{
    int width = 1024, height = 768;

    //Инициализация GLFW
    if (!glfwInit()) {
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(width, height, "L8 Example", NULL, NULL);
    glfwMakeContextCurrent(window);
    //Реакция на изменение размера окна
    glfwSetFramebufferSizeCallback(window, reshapeWindows);

    // Инициализация GLEW
    if (glewInit() != GLEW_OK) {
        return -1;
    }

    //Инициализация окна ImGui
    guiDialog objGui (window);

    //Работа с OpenGL 3.x
    BaseOpenGL obj;
    obj.InitShaders();


    //Отображение набора точек
    //obj.InitDataPoints();
    //Добавить еще точек
    //obj.AddPointXY(0.5f, 0.3f);
    //obj.AddPointXY(-0.8f, -0.3f);
    //Удалить все точки
    obj.ClearDataPoints();

    //Построить линию по точкам
    line_Bresenham(&obj, width, height, 100, 100, 900,100);
    line_Bresenham(&obj, width, height, 100, 2*100, 900,2*100);
    line_Bresenham(&obj, width, height, 100, 3*100, 900,3*100);
    //Построить окружность по точкам
    circle_Bresenham(&obj, width, height, width/2, 500, 100);

    obj.InitVBOVAO();

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(shaderProgram);
        glUseProgram(obj.GetShadersProgram());
        glBindVertexArray(obj.GetVAO());
        glDrawArrays(GL_POINTS, 0, obj.GetSize());

        //Создание окна ImGui
        objGui.CreateGuiDialog();
        //Отображение ImGui
        objGui.ShowGui();

        auto tParamObj =  objGui.GetParam();
        glUniform1f(glGetUniformLocation(obj.GetShadersProgram(), "vRed"), tParamObj.valueRed);
        glUniform1f(glGetUniformLocation(obj.GetShadersProgram(), "vGreen"), tParamObj.valueGreen);
        glUniform1f(glGetUniformLocation(obj.GetShadersProgram(), "vBlue"), tParamObj.valueBlue);
        glUniform1i(glGetUniformLocation(obj.GetShadersProgram(), "vThickness"), tParamObj.valueThickness);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
