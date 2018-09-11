#pragma once
#include <iostream>

class NODE
{
public:
	int key;
	NODE* next;

	NODE();
	NODE(int key_value);
	~NODE();
};

