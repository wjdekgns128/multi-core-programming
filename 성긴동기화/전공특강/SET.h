#pragma once
#include "NODE.h"
#include <mutex>

class  null_mutex
{
public:
	void lock() {}
	void unlock() {}
};

class CSET {
	NODE head, tail;
	std::mutex glock;
	//null_mutex glock;

public:
	CSET();
	~CSET();

	void Init();

	bool Add(int key);
	bool Remove(int key);
	bool Contain(int key);

	void Dump();
};
