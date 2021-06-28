#include "glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include "Quad/QuadTree.h"

#include "Shader.h"
#include "Camera.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);          // 回调函数，监听鼠标移动事件
void scroll_callback(GLFWwindow* windows, double xoffset, double yoffset);  // 回调函数，监听鼠标滚轮事件
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void printCameraInfo();

// 生成四叉树并绘制方案1: 生成满四叉树，根据节点评价部分现实子tile
// 引用传参 减少内存约6m
void drawTwoLayer(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader);
// 生成四叉树并绘制方案2: 生成倒数二层为满的四叉树，最后一层根据节点评价生成tile
void drawTwoLater_dynLast(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// camera settings
// front方向默认为(0.0f,0.0f,-1.0f)
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.5f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
Camera camera(cameraPos, cameraUp);

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
QuadTree* qtree = NULL;

// Draw type
enum DRAW_TYPE
{
    SCREEN,     // 屏幕划分，Depth(上半屏幕) = Depth(下班屏幕) - 1；
    CAMERA,     // 相机距离划分
    FOCUS       // 视角焦点划分
}dType;

int main()
{
    // 设置绘制类型
    dType = DRAW_TYPE::SCREEN;

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
    //
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
    Shader ourShader("/Users/jbgong/Desktop/mytask/mytask/glsl/shader.vs", "/Users/jbgong/Desktop/mytask/mytask/glsl/shader.fs");
    
    // ------------------------------------
    // 设置顶点缓冲对象VBO与顶点数组对象VAO的ID
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // input
        processInput(window);

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // create transformations
        // model 模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0/RT_X, 1.0/RT_Y, 1.0));
        // view 观察矩阵
        glm::mat4 view = camera.GetViewMatrix();
        // projection 投影矩阵  (地图平面为二平面，n与f的大小无关)
        glm::mat4 projection = glm::mat4(1.0f);;
        projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.5f, -100.0f);
        
        // 查看相机位置和方向
//        printCameraInfo();
        
        // draw our first triangle
        ourShader.use();
        
        // 通过一致变量（uniform修饰的变量）引用将一致变量值传入渲染管线
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        
        // 节点评价
        // 设置需要显示的depth阈值，父tile深度
        float l = abs(camera.Position[2]);
        int showDepth = static_cast<int>(TREE_DEPTH * 0.4 / l);
        showDepth = showDepth > TREE_DEPTH ? TREE_DEPTH : showDepth;
    
        // 生成四叉树并绘制
        drawTwoLayer(VBO, VAO, showDepth, projection, view, model, ourShader);
//        drawTwoLater_dynLast(VBO, VAO, showDepth, projection, view, model, ourShader);
        
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
    float cameraSpeed = 0.8f * deltaTime;   // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        dType = DRAW_TYPE::SCREEN;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        dType = DRAW_TYPE::CAMERA;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        dType = DRAW_TYPE::FOCUS;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    // 自动进行视口变换
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

    float sensitivity = 0.4;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

void drawTwoLayer(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader)
{
    // Generate quadtree
//    qtree = CreateTreeByRandom();
    qtree = CreateTreeAllNodes(showDepth + 1);
    
    int allPoints = 0, allDepth = 0;                        // 所有顶点的个数
    vector<int> numberOfPoints;                             // 每一层的顶点个数
    allDepth = qtree->GetDepth();
    vector<float*> vv = GetVertex_BFS(allPoints, qtree, numberOfPoints);
    
    // 输出当前绘制的四叉树层
    // std::cout << "Father's Tile Depth: " << showDepth <<"   Son's Tile Depth: " << showDepth + 1 << std::endl;
    
    // 绘制两层
    for(int renderDepth = showDepth, times = 0; times < 2; ++times, ++renderDepth)
    {
        // set shader color
        ourShader.setVec4("inColor", glm::vec4(0.15f * renderDepth, 0.8f - times/5.0f, cos(renderDepth*2), 1.0f));
        
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        //
        
        glBufferData(GL_ARRAY_BUFFER, numberOfPoints[renderDepth] * 3 * sizeof(float), vv[renderDepth], GL_STATIC_DRAW);
        
        // 告诉OpenGL该如何解析顶点数据
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // 父tile全部绘制
        if(times == 0)
        {
            for(int i = 0; i < numberOfPoints[renderDepth] / 4; ++i)
                glDrawArrays(GL_LINE_LOOP, i*4, 4);
        }
        // 子tile
        else if(times == 1)
        {
            for(int i = 0; i < numberOfPoints[renderDepth] / 4; ++i)
            {
                // 2021/6/21: TO BE OPTIMIZED
                // 选择当前将要绘制的4个点，计算其中心点与camera的距离，以选择应该显示的四叉树深度
                // 每个矩形tile的中心点
                glm::vec4 centerPoint((vv[renderDepth][12*i] +vv[renderDepth][12*i+3]+vv[renderDepth][12*i+6]+vv[renderDepth][12*i+9])/4.0,
                                        (vv[renderDepth][12*i+1] + vv[renderDepth][12*i+4] + vv[renderDepth][12*i+7] +vv[renderDepth][12*i+10])/4.0,
                                        (vv[renderDepth][12*i+2] + vv[renderDepth][12*i+5] + vv[renderDepth][12*i+8] +vv[renderDepth][12*i+11])/4.0, 1.0f);
                glm::vec4 scrPoint = view * model * centerPoint;
                if(dType == DRAW_TYPE::SCREEN)
                {
                   scrPoint = projection * scrPoint;
                    
                    if(scrPoint.y <= 0)
                        glDrawArrays(GL_LINE_LOOP, i*4, 4);
                }
                else if(dType == DRAW_TYPE::CAMERA)
                {
                    float dis = glm::distance(glm::vec3(0,0,0), glm::vec3(scrPoint[0], scrPoint[1], scrPoint[2]));
                    if(dis < 1.2f)
                        glDrawArrays(GL_LINE_LOOP, i*4, 4);
                }
                else if(dType == DRAW_TYPE::FOCUS)
                {
                    float angle = glm::dot(glm::vec3(0.0f,0.0f,-1.0f), glm::normalize(glm::vec3(scrPoint[0], scrPoint[1], scrPoint[2])));
                    if(angle > cos(glm::radians(15.0f)))
                        glDrawArrays(GL_LINE_LOOP, i*4, 4);
                }
            }
        }
    }
    // delete pointer
    delete qtree;
    qtree = NULL;
    for(auto& p:vv)
    {
        delete []p;
        p = NULL;
    }
    // 释放vector内存
    vector<float*>().swap(vv);
    //
    glBindVertexArray(0); // no need to unbind it every time
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //
}

void drawTwoLater_dynLast(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader)
{
//    showDepth -= 1;
    // Generate quadtree
//    qtree = CreateTreeByRandom();
    qtree = CreateTreeAllNodes(showDepth);
    
    int leafPointNum = 0;
    float* ptrLeaf = GetVertex_LeafNode(leafPointNum, qtree->GetTreeRoot());

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //
    
    glBufferData(GL_ARRAY_BUFFER, leafPointNum * 3 * sizeof(float), ptrLeaf, GL_STATIC_DRAW);
    
    // 告诉OpenGL该如何解析顶点数据
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 设置着色器颜色并绘制底图
    ourShader.setVec4("inColor", glm::vec4(0.1f, 0.8f, 0.6f, 1.0f));
    for(int i = 0; i < leafPointNum / 4; ++i)
            glDrawArrays(GL_LINE_LOOP, i*4, 4);
    //
    // 寻找需要细分的四叉树节点, 生成子树 depth > showDepth
    for(int i = 0; i < leafPointNum / 4; ++i)
    {
        // 2021/6/21: TO BE OPTIMIZED
        // 选择当前将要绘制的4个点，计算其中心点与camera的距离，以选择应该显示的四叉树深度
        // 每个矩形tile的中心点
        glm::vec4 centerPoint((ptrLeaf[12*i] +ptrLeaf[12*i+3]+ptrLeaf[12*i+6]+ptrLeaf[12*i+9])/4.0,
                                (ptrLeaf[12*i+1] + ptrLeaf[12*i+4] + ptrLeaf[12*i+7] +ptrLeaf[12*i+10])/4.0,
                                (ptrLeaf[12*i+2] + ptrLeaf[12*i+5] + ptrLeaf[12*i+8] +ptrLeaf[12*i+11])/4.0, 1.0f);

        glm::vec4 scrPoint = view * model * centerPoint;
        // 动态生成四叉树
        if(dType == DRAW_TYPE::SCREEN)
        {
            // TO DO ...
        }
        
        else if(dType == DRAW_TYPE::CAMERA)
        {
            // TO DO ...
        }
        
        else if(dType == DRAW_TYPE::FOCUS)
        {
            // 符合与视线方向（屏幕中央）点与rect中心点满足一定夹角的点
            float angle = glm::dot(glm::vec3(0.0f,0.0f,-1.0f), glm::normalize(glm::vec3(scrPoint[0], scrPoint[1], scrPoint[2])));
            
            // 如此判断并不科学, 可能所有的中心点夹角均不满足条件
            // 应该寻找夹角最小的中心点
            // TO DO ...
            if(angle < cos(glm::radians(3.0f)))
                continue;
            //
            vector<QuadTreeNode*> vqnode;
            qtree->GenerateMoreByPoint(PosInfo(centerPoint[1], centerPoint[0]), vqnode, 4);
            for(int i = 0; i < vqnode.size(); ++i)
            {
                
                ourShader.setVec4("inColor", glm::vec4(0.15f * vqnode[i]->depth, 1.0f - 0.1 * vqnode[i]->depth, cos(vqnode[i]->depth), 1.0f));
                float* ptrOneNode = GetArrayByTreeNode(vqnode[i]);
                glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), ptrOneNode , GL_STATIC_DRAW);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
            // free memory
            vqnode.clear();
            vector<QuadTreeNode*>().swap(vqnode);
            //
            break;
        }
    }
    
    // delete pointer
    delete qtree;
    qtree = NULL;
    delete ptrLeaf;
    ptrLeaf = NULL;

    glBindVertexArray(0); // no need to unbind it every time
    glBindBuffer(GL_ARRAY_BUFFER, 0);
//
    
}

// 打印相机的位置以及方向
void printCameraInfo()
{
    std::cout <<"CAMERA POS: " <<camera.Position[0] << "  " <<camera.Position[1]<<"  "<<camera.Position[2]<<std::endl;
    std::cout <<"CAMERA DIR: " <<camera.Front[0] << "  " <<camera.Front[1]<<"  "<<camera.Front[2] << std::endl;
    std::cout <<"-----------------------------"<< std::endl;
}
