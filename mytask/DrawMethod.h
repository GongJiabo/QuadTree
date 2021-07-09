//
//  DrawMethod.hpp
//  mytask
//
//  Created by jbgong on 2021/7/8.
//

#ifndef DrawMethod_hpp
#define DrawMethod_hpp

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// shader
#include "Shader.h"
// para
#include "Maplib.h"

class DrawMethod
{
private:
    unsigned int VBO;
    unsigned int VAO;
    int showDepth;
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    QuadTree* qtree;
    
public:
    DrawMethod();
    DrawMethod(unsigned int VBO, unsigned int VAO, int showDepth,
               glm::mat4 projection, glm::mat4 view, glm::mat4 model);
    ~DrawMethod();
    
    void setDepth(int depth);
    void setMatrix(glm::mat4 projection, glm::mat4 view, glm::mat4 model);
    
    void glBind();
    void glUnbind();
    
    // 引用传参 减少内存约6m
    // 生成四叉树并绘制方案1:
    // 生成满四叉树，根据节点评价部分现实子tile
    void drawTwoLayer(Shader& ourShader);

    // 生成四叉树并绘制方案2:
    // 生成倒数二层为满的四叉树，最后一层根据节点评价生成tile
    void drawTwoLater_dynLast(Shader& ourShader);

    // 生成四叉树并绘制方案3:
    // 1.根据屏幕范围生成底图节点
    // 2.根据感兴趣的范围（上下屏幕/相机位置/视点）生成子四叉树
    void drawLayers_dynamic(Shader& ourShader);

    // 生成四叉树并绘制方案4:
    // 1.根据屏幕四个顶点确定在地图底图的包围和矩形MBR
    // 2.根据MBR生成四叉树 在四叉树内实现 不用对每个像素计算
    // 3.TO DO... 对不同dType绘制
    void drawLayers_MBR(Shader& ourShader);
    // 生成四叉树并绘制方案5:
    // 在方案4的基础上使用glut的库函数gluUnproject计算屏幕像素点坐标对应于世界坐标
    void drawLayers_MBR2(Shader& ourShader);
};

#endif /* DrawMethod_hpp */
