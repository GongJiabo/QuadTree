
#include "DrawMethod.h"
#include "Maplib.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);          // 回调函数，监听鼠标移动事件
void scroll_callback(GLFWwindow* windows, double xoffset, double yoffset);  // 回调函数，监听鼠标滚轮事件
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);     // 回调函数，监听键盘事件，用来更换方法
void framebuffer_size_callback(GLFWwindow* window, int width, int height);              // 回调函数，窗口变换
void processInput(GLFWwindow *window);                                      // 处理键盘事件，根据键盘按键变化位置
void printCameraInfo();


int main()
{
    // 查看类对象所占内存大小
//    QuadTreeNode qqnode, qqtree;
    cout << "SIZE OF QUADTREE: "<< sizeof(QuadTree) << endl;
    cout << "SIZE OF QUADTREENODE: "<< sizeof(QuadTreeNode) << endl;
    
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "QuadTree Map", NULL, NULL);
    
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
    // GLFW键盘监听
    glfwSetKeyCallback(window, key_callback);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile our shader program
    Shader ourShader("/Users/jbgong/Desktop/mytask/mytask/glsl/shader.vs", "/Users/jbgong/Desktop/mytask/mytask/glsl/shader.fs");
    Shader textShader("/Users/jbgong/Desktop/mytask/mytask/glsl/text.vs", "/Users/jbgong/Desktop/mytask/mytask/glsl/text.fs");

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // find path to font
    std::string font_name = "/Users/jbgong/Desktop/mytask/fonts/Antonio-Bold.ttf";
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }
    
    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    
    // QuadTree
    DrawMethod_OneTree drawOneTree;
    drawOneTree.initQuadTree(TREE_DEPTH, MAX_OBJECT, Rect(LB_X, LB_Y, RT_X, RT_Y));
    
    // ------------------------------------
    // 设置顶点缓冲对象VBO与顶点数组对象VAO的ID
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        
        // 节点评价
        // 设置需要显示的depth阈值，父tile深度
        float l = abs(camera.Position[2]);
        int showDepth = static_cast<int>(TREE_DEPTH * 0.4 / l);
        showDepth = showDepth > TREE_DEPTH ? TREE_DEPTH : showDepth;
        
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        string str_fps = "FPS: " + to_string(static_cast<int>(1/deltaTime));
        string str_type = "TYPE OF METHOD: " + to_string(cType+1);
        
        // input
        processInput(window);

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // 绘制文字
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        RenderText(textShader, str_fps, 10.0f, 10.0f, 0.50f, glm::vec3(0.5, 0.8f, 0.2f));
        RenderText(textShader, str_type, 10.0f, 50.0f, 0.50f, glm::vec3(0.5, 0.8f, 0.2f));
        RenderText(textShader, "Depth: "+to_string(showDepth), 10.0f, 90.0f, 0.50f, glm::vec3(0.5, 0.8f, 0.2f));
        
        
        // 创建变换矩阵
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
        //  printCameraInfo();
        
        // 使用着色器
        ourShader.use();
        // 通过一致变量（uniform修饰的变量）引用将一致变量值传入渲染管线
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        
        switch (cType)
        {
            case CREATE_TYPE::TYPE1:
            {
                DrawMethod draw(VBO, VAO, showDepth, projection, view, model);
                draw.drawTwoLayer(ourShader);
                break;
            }
            case CREATE_TYPE::TYPE2:
            {
                DrawMethod draw(VBO, VAO, showDepth, projection, view, model);
                draw.drawTwoLater_dynLast(ourShader);
                break;
            }
            case CREATE_TYPE::TYPE3:
            {
                DrawMethod draw(VBO, VAO, showDepth, projection, view, model);
                draw.drawLayers_dynamic(ourShader);
                break;
            }
            case CREATE_TYPE::TYPE4:
            {
                DrawMethod draw(VBO, VAO, showDepth, projection, view, model);
                draw.drawLayers_MBR(ourShader);
                break;
            }
            case CREATE_TYPE::TYPE5:
            {
                DrawMethod draw(VBO, VAO, showDepth, projection, view, model);
                draw.drawLayers_MBR2(ourShader);
                break;
            }
            case CREATE_TYPE::TYPE6:
            {
                drawOneTree.setGlObj(VBO, VAO);
                drawOneTree.setDepth(showDepth);
                drawOneTree.setMatrix(projection, view, model);
                drawOneTree.drawLayers_MBR(ourShader);
            }
            default:
                break;
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
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
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
                cType = CREATE_TYPE::TYPE6;
                break;
            case CREATE_TYPE::TYPE6:
                cType = CREATE_TYPE::TYPE1;
                break;
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


// 打印相机的位置以及方向
void printCameraInfo()
{
    std::cout <<"CAMERA POS: " <<camera.Position[0] << "  " <<camera.Position[1]<<"  "<<camera.Position[2]<<std::endl;
    std::cout <<"CAMERA DIR: " <<camera.Front[0] << "  " <<camera.Front[1]<<"  "<<camera.Front[2] << std::endl;
    std::cout <<"-----------------------------"<< std::endl;
}
