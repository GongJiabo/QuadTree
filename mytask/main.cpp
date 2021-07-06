#include "glad.h"
#include <GLFW/glfw3.h>
#include <GLUT/GLUT.h>

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

// 引用传参 减少内存约6m
// 生成四叉树并绘制方案1:
// 生成满四叉树，根据节点评价部分现实子tile
void drawTwoLayer(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader);

// 生成四叉树并绘制方案2:
// 生成倒数二层为满的四叉树，最后一层根据节点评价生成tile
void drawTwoLater_dynLast(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader);

// 生成四叉树并绘制方案3:
// 1.根据屏幕范围生成底图节点
// 2.根据感兴趣的范围（上下屏幕/相机位置/视点）生成子四叉树
void drawLayers_dynamic(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader);

// 生成四叉树并绘制方案4:
// 1.根据屏幕四个顶点确定在地图底图的包围和矩形MBR
// 2.根据MBR生成四叉树 在四叉树内实现 不用对每个像素计算
// 3.TO DO... 对不同dType绘制
void drawLayers_MBR(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                    glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader);
// 生成四叉树并绘制方案5:
// 在方案4的基础上使用glut的库函数gluUnproject计算屏幕像素点坐标对应于世界坐标
void drawLayers_MBR2(unsigned int& VBO, unsigned int& VAO, int& showDepth,
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

// Generate QuadTree Type
enum CREATE_TYPE
{
    TYPE1,          // 生成满四叉树 选择绘制底层
    TYPE2,          // 最后一层(底层)动态生成 上层为满四叉树
    TYPE3,          // 根据像素点对应的空间坐标系中map的坐标生成四叉树
    TYPE4,          // 根据包围和MBR生成四叉树
    TYPE5           // gluUnproject
    // ...
}cType;



int main()
{
    // 查看类对象所占内存大小
//    QuadTreeNode qqnode, qqtree;
    cout << "SIZE OF QUADTREE: "<< sizeof(QuadTree) << endl;
    cout << "SIZE OF QUADTREENODE: "<< sizeof(QuadTreeNode) << endl;
    
    // 设置绘制类型
    dType = DRAW_TYPE::SCREEN;
    // 设置四叉树生成类型
    cType = CREATE_TYPE::TYPE5;
    
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
        
//        glUtil.PrintString({ 0, -1 }, "FPS: %5.2f   ", 1.0f/deltaTime);
        
        // input
        processInput(window);

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // create transformations
        // model 模型矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0/RT_X, 1.0/RT_Y, 1.0));
        // view 观察矩阵
        glm::mat4 view = camera.GetViewMatrix();
        // projection 投影矩阵  (地图平面为二平面，n与f的大小无关)
        glm::mat4 projection = glm::mat4(1.0f);;
        projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);
        
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
        
       // cout<<"SHOWDETPH:  " << showDepth << endl;
        
        // 生成四叉树并绘制
        switch (cType)
        {
            case CREATE_TYPE::TYPE1:
            {
                drawTwoLayer(VBO, VAO, showDepth, projection, view, model, ourShader);
                break;
            }
            case CREATE_TYPE::TYPE2:
            {
                drawTwoLater_dynLast(VBO, VAO, showDepth, projection, view, model, ourShader);
                break;
            }
            case CREATE_TYPE::TYPE3:
            {
                drawLayers_dynamic(VBO, VAO, showDepth, projection, view, model, ourShader);
                break;
            }
            case CREATE_TYPE::TYPE4:
            {
                drawLayers_MBR(VBO, VAO, showDepth, projection, view, model, ourShader);
                break;
            }
            case CREATE_TYPE::TYPE5:
            {
                drawLayers_MBR2(VBO, VAO, showDepth, projection, view, model, ourShader);
                break;
            }
        }

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
    float cameraSpeed = 0.8f * deltaTime;
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
    //
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        switch (cType)
        {
            case CREATE_TYPE::TYPE1:
                cType = CREATE_TYPE::TYPE2;
                break;
            case CREATE_TYPE::TYPE2:
                cType = CREATE_TYPE::TYPE3;
                break;
            case CREATE_TYPE::TYPE3:
                cType = CREATE_TYPE::TYPE4;
                break;
            case CREATE_TYPE::TYPE4:
                cType = CREATE_TYPE::TYPE5;
                break;
            case CREATE_TYPE::TYPE5:
                cType = CREATE_TYPE::TYPE1;
            default:
                break;
        }
    }
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
                
                // cout << scrPoint.x << "  " << scrPoint.y << "  " << scrPoint.z << "  " << scrPoint.w <<endl;
                
                if(dType == DRAW_TYPE::SCREEN)
                {
                   scrPoint = projection * scrPoint;
                    if(scrPoint.y <= 0)
                        glDrawArrays(GL_LINE_LOOP, i*4, 4);
                }
                else if(dType == DRAW_TYPE::CAMERA)
                {
                    float dis = glm::distance(glm::vec3(0,0,0), glm::vec3(scrPoint[0], scrPoint[1], scrPoint[2]));
                    if(dis < 1.4f)
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
        // 动态生成四叉树深层次
        if(dType == DRAW_TYPE::SCREEN)
        {
            scrPoint = projection * scrPoint;
             
             if(scrPoint.y > 0)
                 continue;
            //
            vector<QuadTreeNode*> vqnode;
            qtree->GenerateMoreByPoint(PosInfo(centerPoint[1], centerPoint[0]), vqnode, showDepth+1);
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
        }
        
        else if(dType == DRAW_TYPE::CAMERA)
        {
            float dis = glm::distance(glm::vec3(0,0,0), glm::vec3(scrPoint[0], scrPoint[1], scrPoint[2]));
            if(dis > 1.0f)
                continue;
            //
            vector<QuadTreeNode*> vqnode;
            qtree->GenerateMoreByPoint(PosInfo(centerPoint[1], centerPoint[0]), vqnode, showDepth+1);
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
            qtree->GenerateMoreByPoint(PosInfo(centerPoint[1], centerPoint[0]), vqnode, showDepth+4);
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
}

// 生成四叉树并绘制方案3:
// 1.根据屏幕范围生成底图节点
// 2.根据感兴趣的范围（上下屏幕/相机位置/视点）生成子四叉树
void drawLayers_dynamic(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                  glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader)
{
//    std::cout << "Current Depth: " << showDepth     << std::endl;
    // 绑定VAO VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 设置四叉树最大深度为showDepth+2(感兴趣的地方深度更深, 底图深度为showDepth)
    qtree = new QuadTree(showDepth + 2, MAX_OBJECT);
    qtree->InitQuadTreeNode(Rect(LB_X, LB_Y, RT_X, RT_Y));
    
    glm::vec4 v0(qtree->GetTreeRoot()->rect.lb_x, qtree->GetTreeRoot()->rect.lb_y, 0.0f, 1.0f);
    glm::vec4 v1(qtree->GetTreeRoot()->rect.lb_x, qtree->GetTreeRoot()->rect.rt_y, 0.0f, 1.0f);
    glm::vec4 v2(qtree->GetTreeRoot()->rect.rt_x, qtree->GetTreeRoot()->rect.rt_y, 0.0f, 1.0f);
    
    v0 = view * model * v0;
    v1 = view * model * v1;
    v2 = view * model * v2;
    
    glm::vec3 nor = glm::normalize(glm::cross(glm::vec3(v1.x-v0.x, v1.y-v0.y, v1.z-v0.z), glm::vec3(v2.x-v0.x, v2.y-v0.y, v2.z-v0.z)));
//    glm::mat4 inv_proj = glm::inverse(projection);
    glm::mat4 inv_vm = glm::inverse(view * model);
    
    // 对所有像素点进行遍历 暂时将窗口长宽(像素点坐标范围)定死
    // TO DO... 当层数深厚了 采样间隔过大会显示不连续
    for(int x = 0; x < SCR_WIDTH; x += 20)
    {
        for(int y = 0; y < SCR_HEIGHT; y += 20)
        {
            // 将屏幕坐标逆变换回世界坐标系
            // 屏幕坐标 -> NDC坐标  viewport逆变换
            float nx = (x+0.5)*2.0/static_cast<float>(SCR_WIDTH) - 1.0;
            float ny = 1.0 - (y+0.5)*2.0/static_cast<float>(SCR_HEIGHT);
            
            // NDC -> Word Space
//             glm::vec4 p = inv_proj * glm::vec4(nx,ny, -1.0f, 1.0f);
            // 等价直接计算 -- z和w的值并不重要
            nx = nx * tan(glm::radians(camera.Zoom * 0.5f)) * static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
            ny = ny * tan(glm::radians(camera.Zoom * 0.5f));

            // 求从相机出发的dir射线与平面的交点  Z=-1没想明白？？？ 可能是因为在NDC中 成图的面可以视为z=-1的平面
            glm::vec3 dir = glm::normalize(glm::vec3(nx, ny, -1));
            float t = glm::dot((glm::vec3(v1.x,v1.y,v1.z)), nor) / glm::dot(dir, nor);
            glm::vec4 intersectPoint_ViewSpace = glm::vec4(t*dir.x,t*dir.y,t*dir.z,1.0f);
            glm::vec4 intersectPoint = inv_vm * intersectPoint_ViewSpace;
            
            PosInfo pos(intersectPoint.y, intersectPoint.x);
            vector<QuadTreeNode*> vqnode;
            
            // 对交点所在的区域插入四叉树
            // dType1 - 根据屏幕范围画
            if(dType == DRAW_TYPE::SCREEN)
            {
                // 对屏幕上半部分
                if(ny > 0)
                    qtree->GenerateMoreByPoint(pos, vqnode, showDepth);
                // 对屏幕下半部分
                else
                    qtree->GenerateMoreByPoint(pos, vqnode, showDepth + 1);
            }
            
            // dType2 - 根据相机距离画
            if(dType == DRAW_TYPE::CAMERA)
            {
                float dis = glm::distance(glm::vec3(0,0,0), glm::vec3(intersectPoint_ViewSpace[0], intersectPoint_ViewSpace[1], intersectPoint_ViewSpace[2]));
                //
                if(dis > 1.5f)
                    qtree->GenerateMoreByPoint(pos, vqnode, showDepth);
                else
                    qtree->GenerateMoreByPoint(pos, vqnode, showDepth + 1);

            }
            
            // dType3 - 根据视角中心绘制
            if(dType == DRAW_TYPE::FOCUS)
            {
                float angle = glm::dot(glm::vec3(0.0f,0.0f,-1.0f), glm::normalize(glm::vec3(intersectPoint_ViewSpace[0], intersectPoint_ViewSpace[1], intersectPoint_ViewSpace[2])));
                
                if(angle < cos(glm::radians(15.0f)))
                    qtree->GenerateMoreByPoint(pos, vqnode, showDepth);
                else
                    qtree->GenerateMoreByPoint(pos, vqnode, showDepth + 1);
            }
            
            // 绘制节点
            for(int i = 0; i < vqnode.size(); ++i)
            {
                ourShader.setVec4("inColor", glm::vec4(0.15f * vqnode[i]->depth, 1.0f - 0.1 * vqnode[i]->depth,cos(vqnode[i]->depth), 1.0f));
                float* ptrOneNode = GetArrayByTreeNode(vqnode[i]);
                glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), ptrOneNode , GL_STATIC_DRAW);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
                delete ptrOneNode;
            }
            
            // free memory
            vqnode.clear();
            vector<QuadTreeNode*>().swap(vqnode);
        }
    }
    // delete pointer
    delete qtree;
    qtree = NULL;

    glBindVertexArray(0); // no need to unbind it every time
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawLayers_MBR(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                    glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader)
{
    // 确定地图平面的法向量
    glm::vec4 v0(LB_X, LB_Y, 0.0f, 1.0f);
    glm::vec4 v1(LB_X, RT_Y, 0.0f, 1.0f);
    glm::vec4 v2(RT_X, RT_Y, 0.0f, 1.0f);
    
    v0 = view * model * v0;
    v1 = view * model * v1;
    v2 = view * model * v2;
    
    glm::vec3 nor = glm::normalize(glm::cross(glm::vec3(v1.x-v0.x, v1.y-v0.y, v1.z-v0.z), glm::vec3(v2.x-v0.x, v2.y-v0.y, v2.z-v0.z)));
//    glm::mat4 inv_proj = glm::inverse(projection);
    glm::mat4 inv_vm = glm::inverse(view * model);
    
    // 对屏幕四个顶点投影逆变换 确定包围和矩形MBR(Minimum Boundary Rect)
    float vx[5] = {1.0,-1.0,-1.0,1.0, 0.0};
    float vy[5] = {1.0,1.0,-1.0,-1.0, 0.0};
    float minx = FLT_MAX, maxx = FLT_MIN, miny = FLT_MAX, maxy = FLT_MIN;
    float xcenter, ycenter;
    for(int i = 0; i < 5; ++i)
    {
        float nx = vx[i] * tan(glm::radians(camera.Zoom * 0.5f)) * static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
        float ny = vy[i] * tan(glm::radians(camera.Zoom * 0.5f));
        
        // 求从相机出发的dir射线与平面的交点  Z=-1 成图的面可以视为z=-1的平面
        glm::vec3 dir = glm::normalize(glm::vec3(nx, ny, -1));
        float t = glm::dot((glm::vec3(v1.x, v1.y, v1.z)), nor) / glm::dot(dir, nor);
        glm::vec4 intersectPoint_ViewSpace = glm::vec4(t*dir.x, t*dir.y, t*dir.z,1.0f);
        glm::vec4 intersectPoint = inv_vm * intersectPoint_ViewSpace;

        if(i == 4)
        {
            xcenter = intersectPoint.x;
            ycenter = intersectPoint.y;
        }
        
        minx = min(minx, intersectPoint.x);
        maxx = max(maxx, intersectPoint.x);
        miny = min(miny, intersectPoint.y);
        maxy = max(maxy, intersectPoint.y);
    }
    //
    qtree = CreateTreeByMBR(minx, maxx, miny, maxy, xcenter, ycenter, showDepth);
//    int leafPointNum = 0;
//    float* pleafNode = GetVertex_LeafNode(leafPointNum, qtree->GetTreeRoot());
    
    // 绑定VAO VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 每个QuadTreeNode节点绘制一次矩形
    queue<QuadTreeNode*> q;
    q.push(qtree->GetTreeRoot());
    while(!q.empty())
    {
        QuadTreeNode* node = q.front();
        q.pop();
        // 绘制
        if(node->depth >= showDepth - 1)
        {
            ourShader.setVec4("inColor", glm::vec4(0.15f * node->depth, 1.0f - 0.1 * node->depth,cos(node->depth), 1.0f));
            float* pleafNode = GetArrayByTreeNode(node);
            glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), pleafNode , GL_STATIC_DRAW);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            delete []pleafNode;
        }
        if(node->child_num == 0)
            continue;
        for(int i = 0; i < 4; ++i)
            q.push(node->child[i]);
    }
    
    // delete pointer
    delete qtree;
    qtree = NULL;
//    delete [] pleafNode;
    //
    glBindVertexArray(0); // no need to unbind it every time
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawLayers_MBR2(unsigned int& VBO, unsigned int& VAO, int& showDepth,
                    glm::mat4& projection, glm::mat4& view, glm::mat4& model, Shader& ourShader)
{
    // 变换矩阵转数组
    double modelview[16], project[16];//模型投影矩阵
    glm::mat4 mulmv = view * model;
    for(int i = 0; i < 4; ++i)
    {
        modelview[4*i    ] = mulmv[i][0];
        modelview[4*i + 1] = mulmv[i][1];
        modelview[4*i + 2] = mulmv[i][2];
        modelview[4*i + 3] = mulmv[i][3];
        project[4*i    ] = projection[i][0];
        project[4*i + 1] = projection[i][1];
        project[4*i + 2] = projection[i][2];
        project[4*i + 3] = projection[i][3];
    }
    int viewport[4]={0,0,SCR_WIDTH,SCR_HEIGHT};//视口
    
//    double objx,objy,objz;//获得的世界坐标值
//    glGetDoublev( GL_PROJECTION_MATRIX, project );//获得投影矩阵
//    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );//获得模型矩阵
//    glGetIntegerv( GL_VIEWPORT, viewport );    //获得视口
//    glReadPixels( x, viewport[3]-y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &ScreenZ ); //获得屏幕像素对应的世界坐标深度值
//    gluUnProject( SCR_WIDTH, SCR_HEIGHT, 0.0 , modelview, project, viewport, &objx, &objy, &objz );//获得屏幕坐标对应的世界坐标
    
    // 确定地图平面的法向量(世界空间坐标系)
    glm::vec4 v0(LB_X, LB_Y, 0.0f, 1.0f);
    glm::vec4 v1(LB_X, RT_Y, 0.0f, 1.0f);
    glm::vec4 v2(RT_X, RT_Y, 0.0f, 1.0f);
    glm::vec3 nor = glm::normalize(glm::cross(glm::vec3(v1.x-v0.x, v1.y-v0.y, v1.z-v0.z), glm::vec3(v2.x-v0.x, v2.y-v0.y, v2.z-v0.z)));

    
    // 对屏幕四个顶点投影逆变换 确定包围和矩形MBR(Minimum Boundary Rect)
    float vx[5] = {1.0, 1.0, 0.0, 0.0, 0.5};
    float vy[5] = {1.0, 0.0, 1.0, 0.0, 0.5};
    float minx = FLT_MAX, maxx = FLT_MIN, miny = FLT_MAX, maxy = FLT_MIN;
    float xcenter = 0, ycenter = 0;
    for(int i = 0; i < 5; ++i)
    {
        // 使用gluUnproject
        double x0,x1,y0,y1,z0,z1;
        //获得屏幕坐标对应的世界坐标
        // vec3(x0,y0,z0) 与 vec3(x1,y1,z1) 与 camera.Postion必然在一条直线上！
        gluUnProject(vx[i]*SCR_WIDTH, vy[i]*SCR_HEIGHT, 0.0 , modelview, project, viewport, &x0, &y0, &z0);
        gluUnProject(vx[i]*SCR_WIDTH, vy[i]*SCR_HEIGHT, 1.0, modelview, project, viewport, &x1, &y1, &z1);
        glm::vec3 dir = glm::normalize(glm::vec3(x1-x0, y1-y0, z1-z0));
        
        // 求从相机出发的dir射线与平面的交点  Z=-1 成图的面可以视为z=-1的平面
        float t = glm::dot((glm::vec3(v1.x - x0, v1.y - y0, v1.z - z0)), nor) / glm::dot(dir, nor);
        glm::vec4 intersectPoint_wordSpace = glm::vec4(x0 + t*dir.x, y0 + t*dir.y, z0 + t*dir.z, 1.0f);
        
        // line0 line1 camera.Position 必然三点共线
//        glm::vec3 line0 = glm::normalize(glm::vec3(x0-camera.Position.x, y0-camera.Position.y, z0-camera.Position.z));
//        glm::vec3 line1 = glm::normalize(glm::vec3(x0-x1, y0-y1, z0-z1));
//        cout << "line0: " << line0.x << "  " << line0.y << "  " << line0.z << endl;
//        cout << "line1: " << line1.x << "  " << line1.y << "  " << line1.z << endl << endl;
        
        // 计算屏幕中心点
        if(i == 4)
        {
            xcenter = intersectPoint_wordSpace.x;
            ycenter = intersectPoint_wordSpace.y;
            break;
        }
        
        // min与max应该互为相反数
        minx = min(minx, intersectPoint_wordSpace.x);
        maxx = max(maxx, intersectPoint_wordSpace.x);
        miny = min(miny, intersectPoint_wordSpace.y);
        maxy = max(maxy, intersectPoint_wordSpace.y);
    }
    //
    qtree = CreateTreeByMBR(minx, maxx, miny, maxy, xcenter, ycenter, showDepth);
//    int leafPointNum = 0;
//    float* pleafNode = GetVertex_LeafNode(leafPointNum, qtree->GetTreeRoot());
    
    // 绑定VAO VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 绘制节点
//    ourShader.setVec4("inColor", glm::vec4(0.15f , 0.8f, 0.5f, 1.0f));
//    glBufferData(GL_ARRAY_BUFFER, leafPointNum * 3 * sizeof(float), pleafNode , GL_STATIC_DRAW);
//    for(int i = 0; i < leafPointNum/4; ++i)
//        glDrawArrays(GL_LINE_LOOP, i*4, 4);
    
    // 每个QuadTreeNode节点绘制一次矩形
    queue<QuadTreeNode*> q;
    q.push(qtree->GetTreeRoot());
    while(!q.empty())
    {
        QuadTreeNode* node = q.front();
        q.pop();
        // 绘制
        if(node->depth >= showDepth - 1)
        {
            ourShader.setVec4("inColor", glm::vec4(0.15f * node->depth, 1.0f - 0.1 * node->depth,cos(node->depth), 1.0f));
            float* pleafNode = GetArrayByTreeNode(node);
            glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), pleafNode , GL_STATIC_DRAW);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            delete []pleafNode;
        }
        if(node->child_num == 0)
            continue;
        for(int i = 0; i < 4; ++i)
            q.push(node->child[i]);
    }
    
    // delete pointer
    delete qtree;
    qtree = NULL;
//    delete [] pleafNode;
    //
    glBindVertexArray(0); // no need to unbind it every time
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// 打印相机的位置以及方向
void printCameraInfo()
{
    std::cout <<"CAMERA POS: " <<camera.Position[0] << "  " <<camera.Position[1]<<"  "<<camera.Position[2]<<std::endl;
    std::cout <<"CAMERA DIR: " <<camera.Front[0] << "  " <<camera.Front[1]<<"  "<<camera.Front[2] << std::endl;
    std::cout <<"-----------------------------"<< std::endl;
}
