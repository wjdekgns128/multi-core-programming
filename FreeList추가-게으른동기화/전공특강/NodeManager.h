#pragma once
#include "NODE.h"

class NodeManager
{
	NODE *first, *second;
	std::mutex f_m, s_m;

public:
	NodeManager();
	~NodeManager();

	NODE *GetNode(int key);
	void FreeNode(NODE *e);
	void Recycle();
};

