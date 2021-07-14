#include "QuadTree.h"

bool isIntersect(const Rect& ret, const Rect& mbr)
{
    double wa = ret.rt_x - ret.lb_x;
    double ha = ret.rt_y - ret.lb_y;
    double wb = mbr.rt_x - mbr.lb_x;
    double hb = mbr.rt_y - mbr.lb_y;
    double midXa = (ret.lb_x + ret.rt_x)/2.0;
    double midYa = (ret.lb_y + ret.rt_y)/2.0;
    double midXb = (mbr.lb_x + mbr.rt_x)/2.0;
    double midYb = (mbr.lb_y + mbr.rt_y)/2.0;
    if(abs(midXa-midXb) <= (wa+wb)/2.0 && abs(midYa-midYb) <= (ha+hb)/2.0)
        return true;
    return false;

}

QuadTree::~QuadTree()
{
//    std::cout<<"~QuadTree\n";
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

void QuadTree::CreateQuadTreeNode(QuadTreeNode* fatherNode, int depth, Rect rect, QuadTreeNode *p_node)
{
    p_node->father = fatherNode;
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
    // 均分四块
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

	CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
	CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
	CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), p_node2);
	CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);

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
            p_node->child[index]->number = ( p_node->number + to_string(index));
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

void QuadTree::SetMdepth(const int d)
{
    m_depth = d;
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

 	for (std::vector<PosInfo>::iterator it = p_node->pos_array.begin(); it != p_node->pos_array.end(); ++it)
 	{
 		if (pos.user_id == (*it).user_id)
 		{
 			p_node->pos_array.erase(it);
 			break;
 		}
 	}
}

void QuadTree::Find(PosInfo pos, QuadTreeNode *p_start, QuadTreeNode *&p_target)
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

// 删除指定的四叉树节点
void QuadTree::DeleteQuadTreeNode(QuadTreeNode*& p_node)
{
    if(p_node->father==NULL)
        return;
    // 输出日志
    flog.W(__FILE__, __LINE__, YLog::INFO, "Delete Node", p_node->number, p_node->depth);
    // 处理父节点
    --p_node->father->child_num;
    // 处理子节点
    delete p_node;
    p_node = NULL;
}

void QuadTree::DeleteQuadTreeNodeChild(QuadTreeNode *&p_node)
{
    if(p_node->child_num == 0)
        return;
    // 输出日志
    
    flog.W(__FILE__, __LINE__, YLog::INFO, "Current Node ", p_node->number, p_node->depth);
    // 删除当前节点的孩子节点
    for(int i = 0; i < CHILD_NUM; ++i)
    {
        if(p_node->child[i]!=NULL)
        {
            flog.W(__FILE__, __LINE__, YLog::INFO, "Delete Child Node["+to_string(i)+"]", p_node->child[i]->number, p_node->child[i]->depth);
            delete p_node->child[i];
            p_node->child[i] = NULL;
        }
    }
    p_node->child_num = 0;
    return;
}

// 根据指定深度depth，生成四叉树所有的子节点
void QuadTree::CreateAllNodes()
{
    GenerateAllNodes(0, m_root);
}
// 递归实现生成所有的子节点
void QuadTree::GenerateAllNodes(int curDepth, QuadTreeNode* pNode)
{
    if(curDepth >= m_depth)
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
    //
    p_node0->number = (pNode->number + "0");
    p_node1->number = (pNode->number + "1");
    p_node2->number = (pNode->number + "2");
    p_node3->number = (pNode->number + "3");
    //
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), p_node2);
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);

    pNode->child[0] = p_node0;
    pNode->child[1] = p_node1;
    pNode->child[2] = p_node2;
    pNode->child[3] = p_node3;
    pNode->child_num = 4;
    for(int i = 0; i < CHILD_NUM; ++i)
        GenerateAllNodes(curDepth + 1, pNode->child[i]);
}


void QuadTree::GenerateMoreByPoint(PosInfo pos, vector<QuadTreeNode*>& vqnode, const int dstDepth)
{
    // 判断pos的范围
    if(abs(pos.latitude) > RT_Y || abs(pos.longtitude) > RT_X)
        return;
    
    // 查找pos所在的叶子节点
    QuadTreeNode* leaf_node = NULL;
    Find(pos, m_root, leaf_node);
    
    // 对找到的leaf_node生成到指定深度dstDepth的子树
    GenerateMoreInNode(pos, leaf_node, vqnode, dstDepth);

}

void QuadTree::GenerateMoreInNode(PosInfo pos, QuadTreeNode*& leaf_node, vector<QuadTreeNode*>& vqnode, const int dstDepth)
{
    if(!leaf_node || leaf_node->depth >= dstDepth )
        return;
    double start_x = leaf_node->rect.lb_x;
    double start_y = leaf_node->rect.lb_y;
    double sub_width = (leaf_node->rect.rt_x - leaf_node->rect.lb_x) / 2;
    double sub_height = (leaf_node->rect.rt_y - leaf_node->rect.lb_y) / 2;
    double end_x = leaf_node->rect.rt_x;
    double end_y = leaf_node->rect.rt_y;
   //
    QuadTreeNode *p_node0 = new QuadTreeNode;
    QuadTreeNode *p_node1 = new QuadTreeNode;
    QuadTreeNode *p_node2 = new QuadTreeNode;
    QuadTreeNode *p_node3 = new QuadTreeNode;
    //
    p_node0->number = (p_node0->number + "0");
    p_node1->number = (p_node1->number + "1");
    p_node2->number = (p_node2->number + "2");
    p_node3->number = (p_node3->number + "3");
    //
    vqnode.push_back(p_node0);
    vqnode.push_back(p_node1);
    vqnode.push_back(p_node2);
    vqnode.push_back(p_node3);
    
    CreateQuadTreeNode(leaf_node, leaf_node->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
    CreateQuadTreeNode(leaf_node, leaf_node->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
    CreateQuadTreeNode(leaf_node, leaf_node->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), p_node2);
    CreateQuadTreeNode(leaf_node, leaf_node->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);
    
    leaf_node->child[0] = p_node0;
    leaf_node->child[1] = p_node1;
    leaf_node->child[2] = p_node2;
    leaf_node->child[3] = p_node3;
    leaf_node->child_num = 4;
    //
    int index = GetIndex(pos, leaf_node);
    GenerateMoreInNode(pos, leaf_node->child[index], vqnode, dstDepth);
}

void QuadTree::CreateNodesByMBR(const double& minx, const double& maxx, const double& miny, const double& maxy, const double& xcenter, const double& ycenter)
{
    double lbx = minx < -180.0 ? -180.0 : minx;
    double rtx = maxx > 180.0 ? 180.0 : maxx;
    double lby = miny < -90.0 ? -90.0 : miny;
    double rty = maxy > 90.0 ? 90.0 : maxy;
    //
    CreateNodesByMBR_Recursion(lbx, rtx, lby, rty, xcenter, ycenter, m_root);
}

void QuadTree::CreateNodesByMBR_Recursion(const double& minx, const double& maxx, const double& miny, const double& maxy, const double& xcenter, const double& ycenter, QuadTreeNode* pNode)
{
    // 当前pNode的rect 在 MBR 不相交/外部
    if(!pNode && !isIntersect(pNode->rect, Rect(minx, miny, maxx, maxy)))
        return;
    
    bool up = ((pNode->rect.lb_y + pNode->rect.rt_y) >= (2*ycenter));
    // 下半平面 最大深度
    if(!up && pNode->depth >= m_depth)
        return;
    // 上半平面 最大深度-1
    if(up && pNode->depth >= m_depth-1)
        return;
    
    
    // 生成子节点并递归
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
    
    p_node0->number = (pNode->number + "0");
    p_node1->number = (pNode->number + "1");
    p_node2->number = (pNode->number + "2");
    p_node3->number = (pNode->number + "3");
    
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), p_node2);
    CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);

    pNode->child[0] = p_node0;
    pNode->child[1] = p_node1;
    pNode->child[2] = p_node2;
    pNode->child[3] = p_node3;
    pNode->child_num = 4;
    for(int i = 0; i < CHILD_NUM; ++i)
        CreateNodesByMBR_Recursion(minx, maxx, miny, maxy, xcenter, ycenter, pNode->child[i]);
}

// 根据指定MBR区域(minx, maxx, miny, maxy) 生成 区域内的四叉树节点
void QuadTree::MaintainNodesByMBR(const double& minx, const double& maxx, const double& miny, const double& maxy, const double& xcenter, const double& ycenter)
{
    double lbx = minx < -180.0 ? -180.0 : minx;
    double rtx = maxx > 180.0 ? 180.0 : maxx;
    double lby = miny < -90.0 ? -90.0 : miny;
    double rty = maxy > 90.0 ? 90.0 : maxy;
    //
//    MaintainNodesByMBR_Recursion(lbx, rtx, lby, rty, xcenter, ycenter, m_root);
    MaintainNodesByMBR_Recursion(minx, maxx, miny, maxy, xcenter, ycenter, m_root);

}

void QuadTree::MaintainNodesByMBR_Recursion(const double& minx, const double& maxx, const double& miny, const double& maxy, const double& xcenter, const double& ycenter, QuadTreeNode*& pNode)
{
    if(pNode==NULL)
        return;
    
    // 当前pNode的节点是否与MBR相交
    // (如果不相交删除当前节点 删除后会出现底图缺失的情况 因此删除孩子节点) 并返回
    if(!isIntersect(pNode->rect, Rect(minx, miny, maxx, maxy)))
//    if(pNode->rect.lb_x > maxx || pNode->rect.rt_x < minx || pNode->rect.lb_y > maxy || pNode->rect.rt_y < miny )
    {
        DeleteQuadTreeNode(pNode);
        return;
    }

    // 判断所需的深度 对过深的节点进行释放
    bool up = ((pNode->rect.lb_y + pNode->rect.rt_y) >= (2*ycenter));
    // 下半平面 最大深度
    if(!up && pNode->depth >= m_depth)
    {
        DeleteQuadTreeNodeChild(pNode);
        return;
    }
    // 上半平面 最大深度-1
    if(up && pNode->depth >= m_depth-1)
    {
        DeleteQuadTreeNodeChild(pNode);
        return;
    }

    // 如果当前节点与MBR相交 递归下一层
    // 孩子节点不足且深度不足 生成孩子节点
    if(pNode->child_num < CHILD_NUM)
    {
        // 生成子节点并递归
        double start_x = pNode->rect.lb_x;
        double start_y = pNode->rect.lb_y;
        double sub_width = (pNode->rect.rt_x - pNode->rect.lb_x) / 2;
        double sub_height = (pNode->rect.rt_y - pNode->rect.lb_y) / 2;
        double end_x = pNode->rect.rt_x;
        double end_y = pNode->rect.rt_y;
        // 创建pNode的孩子节点
        if(!pNode->child[0] &&
           isIntersect(Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), Rect(minx, miny, maxx, maxy)))
        {
            QuadTreeNode *p_node0 = new QuadTreeNode;
            p_node0->number = (pNode->number + "0");
            CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y + sub_height, end_x, end_y), p_node0);
            pNode->child[0] = p_node0;
            ++pNode->child_num;
            flog.W(__FILE__, __LINE__, YLog::INFO, "Create Child Node[0]", p_node0->number, p_node0->depth);
        }
        if(!pNode->child[1] &&
           isIntersect(Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), Rect(minx, miny, maxx, maxy)))
        {
            QuadTreeNode *p_node1 = new QuadTreeNode;
            p_node1->number = (pNode->number + "1");
            CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y + sub_height, start_x + sub_width, end_y), p_node1);
            pNode->child[1] = p_node1;
            ++pNode->child_num;
            flog.W(__FILE__, __LINE__, YLog::INFO, "Create Child Node[1]", p_node1->number, p_node1->depth);

        }
        if(!pNode->child[2] &&
           isIntersect(Rect(start_x, start_y, start_x + sub_width, start_y + sub_height), Rect(minx, miny, maxx, maxy)))
        {
            QuadTreeNode *p_node2 = new QuadTreeNode;
            p_node2->number = (pNode->number + "2");
            CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x, start_y, start_x + sub_width, start_y + sub_height),p_node2);
            pNode->child[2] = p_node2;
            ++pNode->child_num;
            flog.W(__FILE__, __LINE__, YLog::INFO, "Create Child Node[2]", p_node2->number, p_node2->depth);
        }
        if(!pNode->child[3] &&
           isIntersect(Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), Rect(minx, miny, maxx, maxy)))
        {
            QuadTreeNode *p_node3 = new QuadTreeNode;
            p_node3->number = (pNode->number + "3");
            CreateQuadTreeNode(pNode, pNode->depth + 1, Rect(start_x + sub_width, start_y, end_x, start_y + sub_height), p_node3);
            pNode->child[3] = p_node3;
            ++pNode->child_num;
            flog.W(__FILE__, __LINE__, YLog::INFO, "Create Child Node[3]", p_node3->number, p_node3->depth);
        }
    }
    for(int i = 0; i < CHILD_NUM; ++i)
        MaintainNodesByMBR_Recursion(minx, maxx, miny, maxy, xcenter, ycenter, pNode->child[i]);
}


/*----------------------------*/

QuadTree* CreateTreeByRandom()
{
    QuadTree* qtree = new QuadTree(TREE_DEPTH, MAX_OBJECT);    // 8  5
    qtree->InitQuadTreeNode(Rect(LB_X, LB_Y, RT_X, RT_Y));


    srand((unsigned)time(NULL));
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

QuadTree* CreateTreeAllNodes(int depth)
{
    QuadTree* qtree = new QuadTree(depth, MAX_OBJECT);
    qtree->InitQuadTreeNode(Rect(LB_X, LB_Y, RT_X, RT_Y));
    qtree->CreateAllNodes();
    return qtree;
}

QuadTree* CreateTreeByMBR(const double minx, const double maxx, const double miny, const double maxy, const double xcenter, const double ycenter, int depth)
{
    QuadTree* qtree = new QuadTree(depth, MAX_OBJECT);
    qtree->InitQuadTreeNode(Rect(LB_X, LB_Y, RT_X, RT_Y));
    qtree->CreateNodesByMBR(minx, maxx, miny, maxy, xcenter, ycenter);
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
            ptr[12*j  ] = vvqtn[i][j]->rect.lb_x;
            ptr[12*j+1] = vvqtn[i][j]->rect.lb_y;
            ptr[12*j+2] = 0.0;
//            ptr[12*j+2] = static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
            // LU
            ptr[12*j+3] = vvqtn[i][j]->rect.lb_x;
            ptr[12*j+4] = vvqtn[i][j]->rect.rt_y;
            ptr[12*j+5] = 0.0;
//            ptr[12*j+5] =  static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
            // RU
            ptr[12*j+6] = vvqtn[i][j]->rect.rt_x;
            ptr[12*j+7] = vvqtn[i][j]->rect.rt_y;
            ptr[12*j+8] = 0.0;
//            ptr[12*j+8] = static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
            // RB
            ptr[12*j+9 ] = vvqtn[i][j]->rect.rt_x;
            ptr[12*j+10] = vvqtn[i][j]->rect.lb_y;
            ptr[12*j+11] = 0.0;
//            ptr[12*j+11] = static_cast<float>(vvqtn[i][j]->depth)/TREE_DEPTH;
        }
        res.push_back(ptr);
    }
    //free memory
    vector<vector<QuadTreeNode*>>().swap(vvqtn);
    //
    return res;
}

float* GetVertex_LeafNode(int& leafPointNum, QuadTreeNode* node)
{
    vector<float*> vres;
    leafPointNum = 0;
    //
    if(!node)
        return NULL;
    queue<QuadTreeNode*> q;
    q.push(node);
    //
    while(!q.empty())
    {
        QuadTreeNode* tmp = q.front();
        q.pop();
        // 判断是否是叶子节点
        if(tmp->child_num == 0)
            vres.push_back(GetArrayByTreeNode(tmp));
        //
        for(int i = 0; i < tmp->child_num; ++i)
            q.push(tmp->child[i]);
    }
    // trans vres -> float*
    leafPointNum = static_cast<int>(vres.size()) * 4;
    float* res = new float[leafPointNum*3];
    // 每个叶子节点保存4个顶点，12个坐标值
    for(int i = 0; i < static_cast<int>(vres.size()); ++i)
    {
        res[12*i + 0] = vres[i][0];
        res[12*i + 1] = vres[i][1];
        res[12*i + 2] = vres[i][2];
        res[12*i + 3] = vres[i][3];
        res[12*i + 4] = vres[i][4];
        res[12*i + 5] = vres[i][5];
        res[12*i + 6] = vres[i][6];
        res[12*i + 7] = vres[i][7];
        res[12*i + 8] = vres[i][8];
        res[12*i + 9] = vres[i][9];
        res[12*i + 10] = vres[i][10];
        res[12*i + 11] = vres[i][11];
        delete[] vres[i];
        vres[i] = NULL;
    }
    // 释放vector内存
    vres.clear();
    vector<float*>().swap(vres);
    return res;
}

// 根据QuadTreeNode返回保存四个角点坐标的数组
float* GetArrayByTreeNode(QuadTreeNode* node)
{
    float* ptr = new float[12];
    // LB
    ptr[0] = node->rect.lb_x;
    ptr[1] = node->rect.lb_y;
    ptr[2] = 0.0;
    // LU
    ptr[3] = node->rect.lb_x;
    ptr[4] = node->rect.rt_y;
    ptr[5] = 0.0;
    // RU
    ptr[6] = node->rect.rt_x;
    ptr[7] = node->rect.rt_y;
    ptr[8] = 0.0;
    // RB
    ptr[9] = node->rect.rt_x;
    ptr[10] = node->rect.lb_y;
    ptr[11] = 0.0;
    //
    return ptr;
}
