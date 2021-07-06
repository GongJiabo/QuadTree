#ifndef QUADTREE_H
#define QUADTREE_H


#include <iostream>
#include <time.h>
#include <fstream>
#include <vector>
#include <queue>

using namespace std;


const int RAND_NUM = 500;
const int MAX_OBJECT = 1;
const int SEARCH_NUM = 15;			

const double LB_X = -180.0;
const double LB_Y = -90.0;
const double RT_X = 180.0;
const double RT_Y = 90.0;
const int TREE_DEPTH = 12;
const int CHILD_NUM = 4;


struct Point
{
    Point():x(0),y(0),z(0),depth(0) {}
    double x;
    double y;
    double z;
    double depth;
};

struct Rect
{
    Rect():lb_x(0), lb_y(0), rt_x(0), rt_y(0){}
    Rect(double lb_x, double lb_y, double rt_x, double rt_y): lb_x(lb_x), lb_y(lb_y), rt_x(rt_x), rt_y(rt_y){}
    Rect& operator=(Rect &rect)
    {
        lb_x = rect.lb_x;
        lb_y = rect.lb_y;
        rt_x = rect.rt_x;
        rt_y = rect.rt_y;
        return *this;
    }
    double lb_x;
    double lb_y;
    double rt_x;
    double rt_y;
};

struct PosInfo
{
    PosInfo():latitude(0), longtitude(0), user_id(""){}
    PosInfo(double la, double lo):latitude(la), longtitude(lo){}
    double longtitude;
    double latitude;
    string user_id;
};

struct QuadTreeNode
{
    QuadTreeNode():rect(), child_num(0), depth(0), number(0)
    {
        pos_array.clear();
        for(auto& p : child)
            p = NULL;
    }
    ~QuadTreeNode()
    {
        // std::cout << "~QuadTreeNode -- Depth = " << depth << std::endl;
        pos_array.clear();
        for (int i = 0; i < CHILD_NUM; ++i)
        {
            // c++ 中删除空指针没影响
            delete child[i];
            child[i] = NULL;
        }
    }
    
    Rect        rect;
    std::vector<PosInfo> pos_array;
    int            child_num;               // 当前区域包含的点位信息
    QuadTreeNode *child[CHILD_NUM];         // 子区域
    int            depth;                   // 深度
    unsigned int number;                    // 索引
};

class QuadTree 
{
public:

	/* 
	
	UL(1)   |    UR(0) 
	--------|----------- 
	LL(2)   |    LR(3) 

	*/  

	enum QuadrantEnum
	{
		INVALID = -1,
		UR = 0,
		UL = 1, 
		LL = 2,
		LR = 3,
	};
	
    void SetMdepth(const int d);
	void InitQuadTree(int depth, int max_objects);
	void InitQuadTreeNode(Rect rect);
	void CreateQuadTreeNode(int depth, Rect rect, QuadTreeNode *p_node);
	void Split(QuadTreeNode *p_node);
	void Insert(PosInfo pos, QuadTreeNode *p_node);
	int  GetIndex(PosInfo pos, QuadTreeNode *p_node);
	void Remove(PosInfo pos, QuadTreeNode *p_node);
	void Find(PosInfo pos, QuadTreeNode *p_start, QuadTreeNode *&p_target);
	void PrintAllQuadTreeLeafNode(QuadTreeNode *p_node);
    
    // 生成四叉树所有的子节点(满四叉树 深度为m_depth)
    void CreateAllNodes();
    void GenerateAllNodes(int curDepth, QuadTreeNode* pNode);
    
    // 根据指定MBR区域(minx, maxx, miny, maxy) 生成区域内的四叉树节点
    void CreateNodesByMBR(const double minx, const double maxx, const double miny, const double maxy, const double xcenter, const double ycenter);
    void GenerateNodesByMRB(const double minx, const double maxx, const double miny, const double maxy, const double xcenter, const double ycenter, QuadTreeNode* pNode);
    
    // 根据所查询的位置，返回查询到的该点所属的叶子节点，并生成该叶子节点的下一层(child)
    void GenerateMoreByPoint(PosInfo pos, vector<QuadTreeNode*>& vqnode, const int dstDepth);
    void GenerateMoreInNode(PosInfo pos, QuadTreeNode*& leaf_node, vector<QuadTreeNode*>& vqnode, const int dstDepth);
	//
	void Search(int num, PosInfo pos_source, std::vector<PosInfo> &pos_list, QuadTreeNode *p_node);
    
    void GetAllArea(std::vector<std::vector<double>>& area_list, QuadTreeNode *p_node);         // DFS
    vector<vector<QuadTreeNode*>> GetAllNodes_BFS(QuadTreeNode *p_node);                        // BFS

    
	QuadTree(int depth, int maxojects):m_depth(depth), m_maxobjects(maxojects), m_root(NULL){}
	~QuadTree();

	QuadTreeNode* GetTreeRoot() { return m_root; }
	int			  GetDepth() { return m_depth; }
	int			  GetMaxObjects() { return m_maxobjects; }
    
private:
	int m_depth;				
	int m_maxobjects;		
	QuadTreeNode *m_root;
};


// 随机生成四叉树
QuadTree* CreateTreeByRandom();

// 生成指定深度的depth满四叉树(所有孩子节点为满)
QuadTree* CreateTreeAllNodes(int depth);

// 根据PosInfo的范围(屏幕四个顶点)生成四叉树
QuadTree* CreateTreeByMBR(const double minx, const double maxx, const double miny, const double maxy, const double xcenter, const double ycenter, int depth);

// 查找四叉树节点，返回搜索路径
std::vector<PosInfo> SearchPoint(QuadTree* p_tree, int& pos_x, int& pos_y);

// BFS获取qtree中所有的矩形顶点，按顺序返回float数组
vector<float*> GetVertex_BFS(int& pnum, QuadTree* qtree,vector<int>& numberOfPoints);

// 获取四叉树根节点顶点组成的float数组
float* GetVertex_LeafNode(int& leafPointNum, QuadTreeNode* node);

// 根据QuadTreeNode返回保存四个角点坐标的数组
float* GetArrayByTreeNode(QuadTreeNode* node);


#endif
