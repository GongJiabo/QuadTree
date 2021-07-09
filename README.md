### QuadTree
使用opengl，绘制四叉树矩形，根据场景（视角）MIX。效果见demo.mp4

平台：MACOS
IDE：XCODE 13.0 beta

##### 使用到的库：

- freetype： 跨平台字体引擎

- glew:：*GLEW*是一个跨平台的C++扩展库，基于OpenGL图形接口
- glfw：*GLFW*一个轻量级的，开源的，跨平台的library。支持OpenGL及OpenGL ES，用来管理窗口，读取输入，处理事件等。
- glad：由于*OpenGL*驱动版本众多，它大多数函数的位置都无法在编译时确定下来，需要在运行时查询。所以任务就落在了开发者身上，开发者需要在运行时获取函数地址并将其保存在一个函数指针中供以后使用。

##### Xcode中加入的Frameworks：

- libfreetype.a
- libGELW.2.2.0
- libglfw.3.3
- OpenGL
- GLUT

PS：一定注意引用库的include路径包含。。。

