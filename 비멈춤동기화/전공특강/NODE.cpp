#include "NODE.h"

NODE::NODE()
{
	next = 0;
}

NODE::NODE(int key_value)
{
	key = key_value;
	next = 0;
}

NODE * NODE::GetNext()
{
	return reinterpret_cast<NODE*>(next & 0xFFFFFFFE);
}

NODE * NODE::GetNextWithMark(bool *mark)
{
	int temp = next;
	*mark = (temp % 2) == 1;
	
	return reinterpret_cast<NODE*>(temp & 0xFFFFFFFE);
}

void NODE::SetNext(NODE * ptr)
{
	next = reinterpret_cast<unsigned>(ptr);
}

bool NODE::CAS(int old_value, int new_value)
{
	return atomic_compare_exchange_strong(reinterpret_cast<atomic_int *>(&next), &old_value, new_value);
}

bool NODE::CAS(NODE * old_next, NODE * new_next, bool old_mark, bool new_mark)
{
	unsigned old_value = reinterpret_cast<unsigned>(old_next);
	if (old_mark)
	{
		old_value = old_value | 0x1;
	}
	else
	{
		old_value = old_value & 0xFFFFFFFE;
	}

	unsigned new_value = reinterpret_cast<unsigned>(new_next);
	if (new_mark)
	{
		new_value = new_value | 0x1;
	}
	else
	{
		new_value = new_value & 0xFFFFFFFE;
	}

	return CAS(old_value, new_value);
}

bool NODE::TryMark()
{
	unsigned old_value = next;
	old_value = old_value & 0xFFFFFFFE;
	unsigned new_value = old_value | 1;

	return CAS(old_value, new_value);
}


NODE::~NODE()
{
}
