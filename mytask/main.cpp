#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include "Quad/QuadTree.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);          // 回调函数，监听鼠标移动事件
void scroll_callback(GLFWwindow* windows, double xoffset, double yoffset);  // 回调函数，监听鼠标滚轮事件
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void printCameraInfo();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// camera settings
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

// render speed time
float deltaTime = 0.0f; // 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间

// Mouse Events
bool firstMouse = true;
float yaw   = -90.0f;    // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float lastX =  SCR_WIDTH / 2.0;
float lastY =  SCR_HEIGHT / 2.0;
float fov   =  40.0f;

// QuadTree
QuadTree* qtree;

// GLSL shader
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.8f, 0.2f, 1.0f);\n"
    "}\n\0";

int main()
{
    // Generate quadtree
    QuadTree* qtree = NULL;
//    qtree = CreateTreeByRandom();
    qtree = CreateTreeAllNodes();
    
    int allPoints = 0, allDepth = 0;                        // 所有顶点的个数
    vector<int> numberOfPoints;                             // 每一层的顶点个数
    allDepth = qtree->GetDepth();
    vector<float*> vv = GetVertex_BFS(allPoints, qtree, numberOfPoints);
    
    // 把之前定义的顶点数据复制到缓冲的内存中，用一个float*保存
    int level = 7, pnum = 0;                // pnum为指定depth后，所包含的顶点个数
    float* qtVertex = NULL;
    float* qtDetph = NULL;
    vector<float*> vinfo = GetArrayBylevel(level, vv, numberOfPoints, pnum);
    qtVertex = vinfo[0];
    qtDetph  = vinfo[1];
    
//    // 输出每个顶点qtVertex信息
//    for(int i = 0; i < pnum ; ++i)
//    {
//        if(i%4==0) std::cout <<std::endl;
//        // 2-LB 1-LU 0-RU 3-RB 从第三象限逆时针排布
////        std::cout<< qtVertex[3*i] << "   " << qtVertex[3*i+1] << "   " << qtVertex[3*i+2] << std::endl;
//        std::cout<< qtDetph[3*i] << "   " << qtDetph[3*i+1] << "   " << qtDetph[3*i+2] << std::endl;
//    }
//
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // 设置当前的窗口上下文
    glfwMakeContextCurrent(window);
    // 根据窗口大小动态改变
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // GLFW隐藏鼠标指针并监听鼠标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    // GLFW鼠标滚轮监听
    glfwSetScrollCallback(window, scroll_callback);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // build and compile our shader program
    // ------------------------------------
    // vertex shader 顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // fragment shader 片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VBO, VAO;
    
    // Move into the loop
    // 设置顶点缓冲对象VBO与顶点数组对象VAO的ID
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glBufferData(GL_ARRAY_BUFFER, pnum * 3 * sizeof(float), qtVertex, GL_DYNAMIC_DRAW);
    
//    获取指定level上的顶点
//    int level = 4;
//    int pnum = numberOfPoints[level];
//    float* vertex = vv[level];
//    glBufferData(GL_ARRAY_BUFFER, pnum * 3 * sizeof(float), vertex, GL_STATIC_DRAW);
    
    // 告诉OpenGL该如何解析顶点数据
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
    
    
    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // create transformations
        // change with time: (float)glfwGetTime() *
        // model 模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // view 观察矩阵
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        
        // projection 投影矩阵  (地图平面为二平面，n与f的大小无关)
        glm::mat4 projection = glm::mat4(1.0f);;
        projection = glm::perspective(glm::radians(fov), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);
        
        // 查看相机位置和方向
//        printCameraInfo();
        
        // draw our first triangle
        glUseProgram(shaderProgram);
        
        // 在把位置向量传给gl_Position之前，我们先添加一个uniform，并且将其与变换矩阵相乘
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        // 通过一致变量（uniform修饰的变量）引用将一致变量值传入渲染管线
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        
        // 节点评价
        // 设置需要显示的depth阈值，最大显示的深度
        float l = abs(cameraPos[2]);
        int showDepth = static_cast<int>(level * 0.8 / l);
        
        // 四个点一组绘制矩形
        for(int i = 0; i < pnum / 4; ++i)
        {
            // 2021/6/21: TO BE OPTIMIZED
            // 选择当前将要绘制的4个点，计算其中心点与camera的距离，以选择应该显示的四叉树深度
            // 每个矩形tile的中心点
            glm::vec4 centerPoint((qtVertex[12*i] + qtVertex[12*i+3]+qtVertex[12*i+6]+qtVertex[12*i+9])/4,
                         (qtVertex[12*i+1] + qtVertex[12*i+4] + qtVertex[12*i+7] + qtVertex[12*i+10])/4,
                         (qtVertex[12*i+2] + qtVertex[12*i+5] + qtVertex[12*i+8] + qtVertex[12*i+11])/4, 1.0f);
            glm::vec4 scrPoint = projection * view * model * centerPoint;
            int curDepth = qtDetph[i*4*3];
            //
            if((scrPoint.y > 0 && curDepth >= showDepth)) continue;
            if((scrPoint.y < 0 && curDepth > showDepth)) continue;
 
            glDrawArrays(GL_LINE_LOOP, i*4, 4);
        }
        
        
         glBindVertexArray(0); // no need to unbind it every time
 
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // 交换缓冲并查询IO事件
        // -----------------------------------------------------------------------  --------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // camera position change
    float cameraSpeed = 1.0f * deltaTime;   // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.04;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // 改变视角fov
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 80.0f)
        fov = 80.0f;
}


// 打印相机的位置以及方向
void printCameraInfo()
{
    std::cout <<"CAMERA POS: " <<cameraPos.x <<"  "<<cameraPos.y<<"  "<<cameraPos.z<<std::endl;
    std::cout <<"CAMERA DIR: " <<cameraFront.x <<"  "<<cameraFront.y<<"  "<<cameraFront.z<<std::endl;
    std::cout <<"-----------------------------"<< std::endl;
}
