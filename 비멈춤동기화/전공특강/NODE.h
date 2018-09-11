#pragma once
#include <iostream>
#include <atomic>

using namespace std;

class NODE
{
public:
	int key;
	unsigned next;
	
	NODE();
	NODE(int key_value);
	NODE *GetNext();
	NODE *GetNextWithMark(bool *mark);
	
	void SetNext(NODE* ptr);
	bool CAS(int old_value, int new_value);
	bool CAS(NODE *old_next, NODE *new_next, bool old_mark, bool new_mark);
	bool TryMark();
	~NODE();
};

