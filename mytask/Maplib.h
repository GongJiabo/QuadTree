//
//  maplib.h
//  mytask
//
//  Created by jbgong on 2021/7/8.
//

#ifndef maplib_h
#define maplib_h

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <GLUT/GLUT.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// freetype
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>

#include "Quad/QuadTree.h"
#include "Shader.h"
#include "Camera.h"



// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// camera settings
// front方向默认为(0.0f,0.0f,-1.0f)
extern glm::vec3 cameraPos;
extern glm::vec3 cameraUp;
extern Camera camera;

// render speed time
extern float deltaTime; // 当前帧与上一帧的时间差
extern float lastFrame; // 上一帧的时间

// Mouse Events
extern bool firstMouse;
extern float yaw;    // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the extern right so we initially rotate a bit to the left.
extern float pitch;
extern float lastX;
extern float lastY;
extern float fov;

// QuadTree
extern QuadTree* qtree;

// Draw type
enum DRAW_TYPE
{
    SCREEN,     // 屏幕划分，Depth(上半屏幕) = Depth(下班屏幕) - 1；
    CAMERA,     // 相机距离划分
    FOCUS       // 视角焦点划分
};
extern DRAW_TYPE dType;

// Generate QuadTree Type
enum CREATE_TYPE
{
    TYPE1,          // 生成满四叉树 选择绘制底层
    TYPE2,          // 最后一层(底层)动态生成 上层为满四叉树
    TYPE3,          // 根据像素点对应的空间坐标系中map的坐标生成四叉树
    TYPE4,          // 根据包围和MBR生成四叉树
    TYPE5,          // gluUnproject
    OTHER
    // ...
};
extern CREATE_TYPE cType;

// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};
extern std::map<GLchar, Character> Characters;

extern unsigned int VBO, VAO;

// 调用freetype绘制文字函数
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);

#endif /* maplib_h */
