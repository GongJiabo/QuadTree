//
//  DrawMethod.cpp
//  mytask
//
//  Created by jbgong on 2021/7/8.
//

#include "DrawMethod.h"


DrawMethod::DrawMethod():VBO(0),VAO(0),showDepth(0),projection(0),view(0),model(0),qtree(NULL)
{
}

DrawMethod::DrawMethod(unsigned int VBO, unsigned int VAO, int showDepth,
           glm::mat4 projection, glm::mat4 view, glm::mat4 model)
{
    this->VBO = VBO;
    this->VAO = VAO;
    this->showDepth = showDepth;
    this->projection = projection;
    this->view = view;
    this->model = model;
}

DrawMethod::~DrawMethod()
{
//    std::cout<<"~DrawMethod --- delete qtree" << std::endl;
    delete qtree;
    qtree = NULL;
}

void DrawMethod::initQuadTree(const int& depth, const int& maxobj, Rect ret)
{
    qtree = new QuadTree(depth, maxobj);
    qtree->InitQuadTreeNode(ret);
}

void DrawMethod::setDepth(int depth)
{
    showDepth = depth;
    qtree->SetDepth(depth);
}

void DrawMethod::setMatrix(glm::mat4 projection, glm::mat4 view, glm::mat4 model)
{
    this->projection = projection;
    this->view = view;
    this->model = model;
}

void DrawMethod::glBind()
{
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);         // VBO赋予了GL_ARRAY_BUFFER的类型，告诉Opengl这个VBO变量是一个顶点缓冲对象
}

void DrawMethod::glUnbind()
{
    glBindVertexArray(0); // no need to unbind it every time
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// 引用传参 减少内存约6m
// 生成四叉树并绘制方案1:
// 生成满四叉树，根据节点评价部分现实子tile
void DrawMethod::drawTwoLayer(Shader& ourShader)
{
    // Generate quadtree
//    qtree = CreateTreeByRandom();
    qtree = CreateTreeAllNodes(showDepth + 1);
    
    int allPoints = 0, allDepth = 0;                        // 所有顶点的个数
    vector<int> numberOfPoints;                             // 每一层的顶点个数
    allDepth = qtree->GetDepth();
    vector<float*> vv = GetVertex_BFS(allPoints, qtree, numberOfPoints);
    
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBind();
    
    // 绘制两层
    for(int renderDepth = showDepth, times = 0; times < 2; ++times, ++renderDepth)
    {
        // set shader color
        ourShader.setVec4("inColor", glm::vec4(0.15f * renderDepth, 0.8f - times/5.0f, cos(renderDepth*2), 1.0f));
    
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
    //
    for(auto& p:vv)
    {
        delete []p;
        p = NULL;
    }
    // 释放vector内存
    vector<float*>().swap(vv);
    //
    glUnbind();
    //
}

// 生成四叉树并绘制方案2:
// 生成倒数二层为满的四叉树，最后一层根据节点评价生成tile
void DrawMethod::drawTwoLater_dynLast(Shader& ourShader)
{
    // Generate quadtree
//    qtree = CreateTreeByRandom();
    qtree = CreateTreeAllNodes(showDepth);
    
    int leafPointNum = 0;
    float* ptrLeaf = GetVertex_LeafNode(leafPointNum, qtree->GetTreeRoot());

    glBind();
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
            if(angle < cos(glm::radians(3.0f)))
                continue;
            //
            vector<QuadTreeNode*> vqnode;
            qtree->GenerateMoreByPoint(PosInfo(centerPoint[1], centerPoint[0]), vqnode, showDepth+4);
            for(int i = 0; i < vqnode.size(); ++i)
            {
                
                ourShader.setVec4("inColor", glm::vec4(0.15f * vqnode[i]->depth, 1.0f - 0.1 * vqnode[i]->depth, cos(vqnode[i]->depth), 1.0f));
                float* ptrOneNode = GetArrayByTreeNode(vqnode[i]);
                
                // void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
                // 第一个参数表示目标的缓冲类型，这里指当前绑定到GL_ARRAY_BUFFER上的顶点缓冲对象
                // 第二个参数表示数据大小（字节为单位）
                // 第三个参数表示我们实际发出的数据
                // 第四个参数GL_STATIC_DRAW表示Opengl如何处理上传的数据
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
    //
    delete ptrLeaf;
    ptrLeaf = NULL;

    glUnbind();
}

// 生成四叉树并绘制方案3:
// 1.根据屏幕范围生成底图节点
// 2.根据感兴趣的范围（上下屏幕/相机位置/视点）生成子四叉树
void DrawMethod::drawLayers_dynamic(Shader& ourShader)
{
    // 绑定VAO VBO
    glBind();
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
    // 当层数深厚了 采样间隔过大会显示不连续
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
    //
    glUnbind();
}


// 生成四叉树并绘制方案4:
// 1.根据屏幕四个顶点确定在地图底图的包围和矩形MBR
// 2.根据MBR生成四叉树 在四叉树内实现 不用对每个像素计算
// 3.TO DO... 对不同dType绘制
void DrawMethod::drawLayers_MBR(Shader& ourShader)
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
    float xcenter = 0.0, ycenter = 0.0;
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
    
    // 绑定VAO VBO
    glBind();
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
    //
    glUnbind();
}


// 生成四叉树并绘制方案5:
// 在方案4的基础上使用glut的库函数gluUnproject计算屏幕像素点坐标对应于世界坐标
void DrawMethod::drawLayers_MBR2(Shader& ourShader)
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
    
    // 确定地图平面的法向量(世界空间坐标系)
    glm::vec4 v0(LB_X, LB_Y, 0.0f, 1.0f);
    glm::vec4 v1(LB_X, RT_Y, 0.0f, 1.0f);
    glm::vec4 v2(RT_X, RT_Y, 0.0f, 1.0f);
    glm::vec3 nor = glm::normalize(glm::cross(glm::vec3(v1.x-v0.x, v1.y-v0.y, v1.z-v0.z), glm::vec3(v2.x-v0.x, v2.y-v0.y, v2.z-v0.z)));

    // 对屏幕四个顶点投影逆变换 确定包围和矩形MBR(Minimum Boundary Rect)
    // 注意此时原点为左下角 x轴向右 y轴向上
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
    
    // 绑定VAO VBO
    glBind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 每个QuadTreeNode叶子节点绘制一次矩形
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
    glUnbind();
}


// DrawMethod_OneTree
DrawMethod_OneTree::DrawMethod_OneTree():DrawMethod(),preMbr(Rect()),curMbr(Rect()),preDepth(0),curDepth(0)
{
    
}

DrawMethod_OneTree::DrawMethod_OneTree(unsigned int VBO, unsigned int VAO, int showDepth,
                   glm::mat4 projection, glm::mat4 view, glm::mat4 model, Rect preMbr, Rect curMbr, int preDepth, int curDepth)
:DrawMethod(VBO, VAO, showDepth, projection, view, model), preMbr(preMbr), curMbr(curMbr), preDepth(preDepth), curDepth(curDepth)
{
    
}

DrawMethod_OneTree::~DrawMethod_OneTree()
{
    
}

void DrawMethod_OneTree::SetPreDepth(int& preDepth)
{
    this->preDepth = preDepth;
}
void DrawMethod_OneTree::SetCurDepth(int& curDepth)
{
    this->curDepth = curDepth;
}

void DrawMethod_OneTree::SetPreMbr(Rect &preMbr)
{
    this->preMbr = preMbr;
}

void DrawMethod_OneTree::SetCurMbr(Rect &curMbr)
{
    this->curMbr = curMbr;
}


void DrawMethod_OneTree::drawLayers_MBR(Shader& ourShader)
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
    
    // 确定地图平面的法向量(世界空间坐标系)
    glm::vec4 v0(LB_X, LB_Y, 0.0f, 1.0f);
    glm::vec4 v1(LB_X, RT_Y, 0.0f, 1.0f);
    glm::vec4 v2(RT_X, RT_Y, 0.0f, 1.0f);
    glm::vec3 nor = glm::normalize(glm::cross(glm::vec3(v1.x-v0.x, v1.y-v0.y, v1.z-v0.z), glm::vec3(v2.x-v0.x, v2.y-v0.y, v2.z-v0.z)));

    // 对屏幕四个顶点投影逆变换 确定包围和矩形MBR(Minimum Boundary Rect)
    // 注意此时原点为左下角 x轴向右 y轴向上
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

    /* core method */
    // 维护一个根节点的同时将叶子节点保存下来
    vector<vector<float>> vvterices;
    qtree->MaintainNodesByMBR(minx, maxx, miny, maxy, xcenter, ycenter, vvterices);
    
    // 绑定VAO VBO
    glBind();
    
    // void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
    // 参数含义: 属性的location 属性大小 数据类型 是否希望数据标准化 步长(数据之间的内存间隔) 偏移量(在一段数据中, 指定的数据便宜多少位置开始)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // void glEnableVertexAttribArray(GLuint index);
    // 参数含义: 要使能的顶点属性索引，这里要说明，0表示使能顶点属性中的位置信息，也就是坐标值
    glEnableVertexAttribArray(0);
    
    
    // 两次缓冲 子父层分开绘制 在10层性能提高(fps: 9 -> 17)
    // 应该尽量减少glBufferdata调用的次数
    for(int i = 0; i < 2; ++i)
    {
        ourShader.setVec4("inColor", glm::vec4(0.1f * (i+showDepth/2.0), 1.0f - 0.2 * (i+showDepth/2.0), 1.0f-sin(i), 1.0f));
        float *pdraw = new float[vvterices[i].size()];
        copy(vvterices[i].begin(), vvterices[i].end(), pdraw);
        
        // 为什么buffer显存总在窗口关闭terminate后全部释放？？？
        glBufferData(GL_ARRAY_BUFFER, vvterices[i].size() * sizeof(float), pdraw , GL_DYNAMIC_COPY);
//        glBufferSubData(GL_ARRAY_BUFFER, 0, vvterices[i].size() * sizeof(float), pdraw);
        
//        // 获取缓冲区的映射指针ptr
//        void * ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
//        // 拷贝我们的数据到指针所指向的位置
//        memcpy(ptr,  static_cast<void*>(pdraw), vvterices[i].size() * sizeof(float));
//        // 使用完之后释放映射的指针
//        glUnmapBuffer(GL_ARRAY_BUFFER);

        // TO BE OPTIMIZED...
        for(int j = 0; j < vvterices[i].size()/12; ++j)
            glDrawArrays(GL_LINE_LOOP, j*4, 4);
    
        delete []pdraw;
    }
    vvterices.clear();
    glUnbind();
    // optional: de-allocate all resources once they've outlived their purpose:
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}
