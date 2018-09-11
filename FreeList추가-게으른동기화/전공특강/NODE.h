#pragma once
#include <iostream>
#include <mutex>

class NODE
{
public:
	int key;
	NODE* next;
	bool removed;
	std::mutex node_lock;
	
	NODE();
	NODE(int key_value);
	void lock();
	void unlock();
	~NODE();
};

