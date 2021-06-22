#include "QuadTree.h"

QuadTree::~QuadTree()
{
    delete m_root;
}

void QuadTree::InitQuadTreeNode(Rect rect)
{
	m_root = new QuadTreeNode;
	m_root->rect = rect;

	for (int i = 0; i < CHILD_NUM; ++i) 
	{  
		m_root->child[i] = NULL; 
	}  
	m_root->depth = 0;
	m_root->child_num = 0;
	m_root->pos_array.clear();
}

void QuadTree::CreateQuadTreeNode(int depth, Rect rect, QuadTreeNode *p_node)
{
	p_node->depth = depth;
	p_node->child_num = 0;
	p_node->pos_array.clear();
	p_node->rect = rect;
	for (int i = 0; i < CHILD_NUM; ++i)  
	{  
		p_node->child[i] = NULL; 
	}
}

void QuadTree::Split(QuadTreeNode *pNode)
{
	if (pNode == NULL)
	{
		return;
	}

	double start_x = pNode->rect.lb_x;
	double start_y = pNode->rect.lb_y;
    //   均分四块
	double sub_width = (pNode->rect.rt_x - pNode->rect.lb_x) / 2;
	double sub_height = (pNode->rect.rt_y - pNode->rect.lb_y) / 2;
    
    // 根据散点距离区域左下角点的坐标差划分
//	double la_sum = 0, lo_sum = 0;
//	for (std::vector<PosInfo>::iterator it = pNode->pos_array.begin(); it != pNode->pos_array.end(); ++it)
//	{
//		la_sum += (*it).latitude - start_x;
//		lo_sum += (*it).longitude - start_y;
//	}
//	double sub_width = (la_sum * 1.0 / (int)pNode->pos_array.size());
//	double sub_height = (lo_sum * 1.0 / (int)pNode->pos_array.size());
    
	double end_x = pNode->rect.rt_x;
	double end_y = pNode->rect.rt_y;

	QuadTreeNode *p_node0 = new QuadTreeNode;
	QuadTreeNode *p_node1 = new QuadTreeNode;
	QuadTreeNode *p_node2 = new QuadTreeNode;
	QuadTreeNode *p_node3 = new QuadTreeNode;

	CreateQuadTreeNode(pNode->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
	CreateQuadTreeNode(pNode->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
	CreateQuadTreeNode(pNode->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), p_node2);
	CreateQuadTreeNode(pNode->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);

	pNode->child[0] = p_node0;
	pNode->child[1] = p_node1;
	pNode->child[2] = p_node2;
	pNode->child[3] = p_node3;

	pNode->child_num = 4;
}

void QuadTree::Insert(PosInfo pos, QuadTreeNode *p_node)
{
	if (p_node == NULL)
	{
		return;
	}

	if (p_node->child_num != 0)
	{
		// index - QuadrantEnum(表示方向)
		int index = GetIndex(pos, p_node);
		if (index >= UR && index <= LR)
		{
            p_node->child[index]->number = ((p_node->number << 2) | index);
			Insert(pos, p_node->child[index]);
		}
	}
	// 是叶子节点
	else
	{
		if ((int)p_node->pos_array.size() >= m_maxobjects && p_node->depth < m_depth)
		{
			Split(p_node);
			for (std::vector<PosInfo>::iterator it = p_node->pos_array.begin(); it != p_node->pos_array.end(); ++it)
			{
				Insert(*it, p_node);
			}
			p_node->pos_array.clear();
			Insert(pos, p_node);
		}
		else
		{
			p_node->pos_array.push_back(pos);
		}
	}
}

void QuadTree::Search(int num, PosInfo pos_source, std::vector<PosInfo> &pos_list, QuadTreeNode *p_node)
{
	if (p_node == NULL)
	{
		return;
	}

	if ((int)pos_list.size() >= num)
	{
		return;
	}

	if (p_node->child_num == 0)
	{
		for (std::vector<PosInfo>::iterator it = p_node->pos_array.begin(); it != p_node->pos_array.end(); ++it)
		{
			if ((int)pos_list.size() >= num)
			{
				return;
			}
			pos_list.push_back(*it);
		}
		return;
	}

	//
	int index = GetIndex(pos_source, p_node);
	if (index >= UR && index <= LR)
	{
		if (p_node->child[index] != NULL)
		{
			Search(num, pos_source, pos_list, p_node->child[index]);
		}
	}

	//
	for (int i = 0; i < CHILD_NUM; ++i)
	{
		if (index != i && p_node->child[i] != NULL)
		{
			Search(num, pos_source, pos_list, p_node->child[i]);
		}
	}
}

void QuadTree::GetAllArea(std::vector<std::vector<double>>& area_list, QuadTreeNode *p_node)
{
    // 如果没有子区域则返回
    if(p_node == NULL)
        return;
    
    // 存储当前区域
    vector<double> tmp{p_node->rect.lb_x, p_node->rect.lb_y, p_node->rect.rt_x, p_node->rect.rt_y, static_cast<double>(p_node->depth)};
    area_list.push_back(tmp);

    // 递归便利子区域
    for(int i=0; i<p_node->child_num; ++i)
        GetAllArea(area_list, p_node->child[i]);
        
}

// BFS生成所有区域
vector<vector<QuadTreeNode*>> QuadTree::GetAllNodes_BFS(QuadTreeNode *p_node)
{
    vector<vector<QuadTreeNode*>> res;
    // 如果没有子区域则返回
    if(p_node == NULL)
        return res;
    
    int curDepth = p_node->depth - 1;
    queue<QuadTreeNode* > q;
    q.push(p_node);
    //
    while(!q.empty())
    {
        QuadTreeNode* node = q.front();
        q.pop();
        //
        if(node->depth == 0 || node->depth != curDepth)
        {
            vector<QuadTreeNode*> vqtn;
            vqtn.push_back(node);
            res.push_back(vqtn);
            ++curDepth;
        }
        else
            res[curDepth].push_back(node);
        //
        for(int i = 0; i < node->child_num; ++i)
        {
            if(node->child[i] != NULL)
                q.push(node->child[i]);
        }
    }
    return res;
}
                             
int QuadTree::GetIndex(PosInfo pos, QuadTreeNode *pNode)
{
	double start_x = pNode->rect.lb_x;
	double start_y = pNode->rect.lb_y;
	double sub_width = (pNode->rect.rt_x - pNode->rect.lb_x) / 2;
	double sub_height = (pNode->rect.rt_y - pNode->rect.lb_y) / 2;
	double end_x = pNode->rect.rt_x;
	double end_y = pNode->rect.rt_y;
	
	// 0 - Up + Right
	if ((pos.longtitude >= start_x + sub_width)
	&& (pos.longtitude <= end_x)
	&& (pos.latitude >= start_y + sub_height)
	&& (pos.latitude <= end_y))
	{
		return UR;
	}
	// 1 - Up + Left
	else if ((pos.longtitude >= start_x)
	&& (pos.longtitude < start_x + sub_width)
	&& (pos.latitude >= start_y + sub_height)
	&& (pos.latitude <= end_y))
	{
		return UL;
	}
	// 2 - Low + Left
	else if ((pos.longtitude >= start_x)
	&& (pos.longtitude < start_x + sub_width)
	&& (pos.latitude >= start_y)
	&& (pos.latitude < start_y + sub_height))
	{
		return LL;
	}
	// 3 - Low + Right
	else if ((pos.longtitude >= start_x + sub_width)
	&& (pos.longtitude <= end_x)
	&& (pos.latitude >= start_y)
	&& (pos.latitude < start_y + sub_height))
	{
		return LR;
	}
	else
	{
		return INVALID;
	}
}

void QuadTree::Remove(PosInfo pos, QuadTreeNode* p_node)
{
	if (p_node == NULL)
	{
		return;
	}

// 	for (std::vector<PosInfo>::iterator it = p_node->pos_array.begin(); it != p_node->pos_array.end(); ++it)
// 	{
// 		if (pos.user_id == (*it).user_id)
// 		{
// 			p_node->pos_array.erase(it);
// 			break;
// 		}
// 	}
}

void QuadTree::Find(PosInfo pos, QuadTreeNode *p_start, QuadTreeNode *p_target)
{
	if (p_start == NULL)
	{
		return;
	}

	p_target = p_start;

	int index = GetIndex(pos, p_start);
	if (index >= UR && index <= LR)
	{
		if (p_start->child[index] != NULL)
		{
			Find(pos, p_start->child[index], p_target);
		}
	}
}

void QuadTree::PrintAllQuadTreeLeafNode(QuadTreeNode *p_node)
{
	if (p_node == NULL)
	{
		return;
	}
	
	ofstream myfile;
	myfile.open("Node.txt", ios::out|ios::app);
	if(!myfile)
	{ 
		return;
	}

	if (p_node->child_num == 0)
	{
		myfile<<"Node:["<<p_node->rect.lb_x<<", "<<p_node->rect.lb_y<<", "<<p_node->rect.rt_x<<", "<<p_node->rect.rt_y<<"], Depth:"<<p_node->depth<<", PosNum:"<<(int)p_node->pos_array.size()<<endl;
		myfile<<"{"<<endl;
		for (std::vector<PosInfo>::iterator it = p_node->pos_array.begin(); it != p_node->pos_array.end(); ++it)
		{
			myfile<<"["<<(*it).latitude<<", "<<(*it).longtitude<<"]"<<std::endl;
		}
		myfile<<"}"<<endl;
		return;
	}
	else
	{
		for (int i = 0; i < CHILD_NUM; ++i)
		{
			PrintAllQuadTreeLeafNode(p_node->child[i]);
		}
	}
}

// 根据指定深度depth，生成四叉树所有的子节点
void QuadTree::CreateAllNodes()
{
    GenerateAllNodes(0, m_root);
}
// 递归实现生成所有的子节点
void QuadTree::GenerateAllNodes(int curDepth, QuadTreeNode* pNode)
{
    if(curDepth == TREE_DEPTH)
        return;
    //
    double start_x = pNode->rect.lb_x;
    double start_y = pNode->rect.lb_y;
    double sub_width = (pNode->rect.rt_x - pNode->rect.lb_x) / 2;
    double sub_height = (pNode->rect.rt_y - pNode->rect.lb_y) / 2;
    double end_x = pNode->rect.rt_x;
    double end_y = pNode->rect.rt_y;
   //
    QuadTreeNode *p_node0 = new QuadTreeNode;
    QuadTreeNode *p_node1 = new QuadTreeNode;
    QuadTreeNode *p_node2 = new QuadTreeNode;
    QuadTreeNode *p_node3 = new QuadTreeNode;
    
    CreateQuadTreeNode(pNode->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
    CreateQuadTreeNode(pNode->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
    CreateQuadTreeNode(pNode->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), p_node2);
    CreateQuadTreeNode(pNode->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);

    pNode->child[0] = p_node0;
    pNode->child[1] = p_node1;
    pNode->child[2] = p_node2;
    pNode->child[3] = p_node3;
    pNode->child_num = 4;
    for(int i = 0; i < CHILD_NUM; ++i)
        GenerateAllNodes(curDepth + 1, pNode->child[i]);
}

/*----------------------------*/

QuadTree* CreateTreeByRandom()
{
    QuadTree* qtree = new QuadTree(TREE_DEPTH, MAX_OBJECT);    // 8  5
    qtree->InitQuadTreeNode(Rect(LB_X, LB_Y, RT_X, RT_Y));


//    srand((unsigned)time(NULL));
    for (int i = 0; i < RAND_NUM; ++i)
    {
        PosInfo pos;
        pos.longtitude = rand() % static_cast<int>(RT_X - LB_X) + static_cast<int>(LB_X) + 1;
        pos.latitude = rand() % static_cast<int>(RT_Y - LB_Y) + static_cast<int>(LB_Y) + 1;
        //
        qtree->Insert(pos, qtree->GetTreeRoot());
    }
    qtree->PrintAllQuadTreeLeafNode(qtree->GetTreeRoot());

    return qtree;
}

QuadTree* CreateTreeAllNodes()
{
    QuadTree* qtree = new QuadTree(TREE_DEPTH, MAX_OBJECT);
    qtree->InitQuadTreeNode(Rect(LB_X, LB_Y, RT_X, RT_Y));
    qtree->CreateAllNodes();
    return qtree;
}

std::vector<PosInfo> SearchPoint(QuadTree* p_tree, int& pos_x, int& pos_y)
{
    std::ofstream myfile;
    myfile.open("QuadTree.txt", ios::out);
    if(!myfile)
    {
        std::cout<<"create file failed!";
        exit(1);
    }
    
    PosInfo pos_source(pos_x, pos_y);
    std::vector<PosInfo> pos_list;

    clock_t start_time = clock();
    myfile<<endl<<"start time: "<<(double)start_time<<std::endl<<std::endl;
    p_tree->Search(SEARCH_NUM, pos_source, pos_list, p_tree->GetTreeRoot());
    myfile<<"pos_list:"<<std::endl;
    for (std::vector<PosInfo>::iterator it = pos_list.begin(); it != pos_list.end(); ++it)
    {
        myfile<<"["<<(*it).latitude<<", "<<(*it).longtitude<<"]"<<std::endl;
    }
    clock_t end_time = clock();
    myfile<<std::endl<<"end time: "<<(double)end_time<<std::endl;
    myfile<<"duration:"<<(double)(end_time - start_time) / CLOCKS_PER_SEC<<std::endl;
    
    myfile.close();
    
    return pos_list;
}


vector<float*> GetVertex_BFS(int& pnum, QuadTree* qtree,vector<int>& numberOfPoints)
{
    vector<vector<QuadTreeNode*>> vvqtn;
    vvqtn = qtree->GetAllNodes_BFS(qtree->GetTreeRoot());
    //
    vector<float*> res;
    //
    for(int i = 0; i<vvqtn.size();++i)
    {
        int curPoints = static_cast<int>(vvqtn[i].size()) * 4;
        // 保存每层的顶点个数(矩形个数*4)
        numberOfPoints.push_back(curPoints);
        pnum += curPoints;
        //
        float* ptr = new float[curPoints * 3];
        for(int j = 0; j < vvqtn[i].size(); ++j)
        {
            // LB
            ptr[12*j  ] = vvqtn[i][j]->rect.lb_x/abs(RT_X);
            ptr[12*j+1] = vvqtn[i][j]->rect.lb_y/abs(RT_Y);
            ptr[12*j+2] = 0.0;
//            ptr[12*j+2] = static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
            // LU
            ptr[12*j+3] = vvqtn[i][j]->rect.lb_x/abs(RT_X);
            ptr[12*j+4] = vvqtn[i][j]->rect.rt_y/abs(RT_Y);
            ptr[12*j+5] = 0.0;
//            ptr[12*j+5] =  static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
            // RU
            ptr[12*j+6] = vvqtn[i][j]->rect.rt_x/abs(RT_X);
            ptr[12*j+7] = vvqtn[i][j]->rect.rt_y/abs(RT_Y);
            ptr[12*j+8] = 0.0;
//            ptr[12*j+8] = static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
            // RB
            ptr[12*j+9 ] = vvqtn[i][j]->rect.rt_x/abs(RT_X);
            ptr[12*j+10] = vvqtn[i][j]->rect.lb_y/abs(RT_Y);
            ptr[12*j+11] = 0.0;
//            ptr[12*j+11] = static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
        }
        res.push_back(ptr);
    }
    //
    return res;
}

// 根据需要显示的depth返回float数组
vector<float*> GetArrayBylevel(const int& level, const vector<float*>& vv, const vector<int>& numberOfPoints, int& pnum)
{
    vector<float> vvertex;
    vector<float> vdepth;
    for(int i = 0; i <= level && i < vv.size(); ++i)
    {
        pnum += numberOfPoints[i];
        for(int j = 0; j < numberOfPoints[i] * 3; ++j)
        {
            vvertex.push_back(vv[i][j]);
            vdepth.push_back(i);
        }
        
    }
    float* vertex = new float[pnum * 3];
    memcpy(vertex, &vvertex[0], vvertex.size()*sizeof(vvertex[0]));
    float* depth = new float[pnum * 3];
    memcpy(depth, &vdepth[0], vdepth.size()*sizeof(vdepth[0]));
    return {vertex, depth};
}

