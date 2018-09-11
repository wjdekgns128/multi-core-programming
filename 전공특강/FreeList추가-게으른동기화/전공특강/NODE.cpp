#include "NODE.h"

NODE::NODE()
{
	next = NULL;
	removed = false;
}

NODE::NODE(int key_value)
{
	next = NULL;
	key = key_value;
	removed = false;
}

void NODE::lock()
{
	node_lock.lock();
}

void NODE::unlock()
{
	node_lock.unlock();
}

NODE::~NODE()
{
}
