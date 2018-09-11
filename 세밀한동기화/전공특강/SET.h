#pragma once
#include "NODE.h"

class CSET {
	NODE head, tail;

public:
	CSET();
	~CSET();

	void Init();

	bool Add(int key);
	bool Remove(int key);
	bool Contain(int key);

	void Dump();
};
