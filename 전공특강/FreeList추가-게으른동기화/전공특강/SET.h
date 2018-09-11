#pragma once
#include "NodeManager.h"

extern NodeManager node_pool;

class CSET {
	NODE head, tail;

public:
	CSET();
	~CSET();

	void Init();

	bool Add(int key);
	bool Remove(int key);
	bool Contain(int key);

	bool validate(NODE* pred, NODE* curr);
	void Dump();
};
