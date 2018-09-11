#include "NODE.h"

NODE::NODE()
{
	next = NULL;
}

NODE::NODE(int key_value)
{
	next = NULL;
	key = key_value;
}

NODE::~NODE()
{
}
