#include "NodeManager.h"

NodeManager::NodeManager()
{
	first = second = nullptr;
}


NodeManager::~NodeManager()
{
	while (nullptr != first)
	{
		NODE *p = first;
		first = first->next;
		delete p;
	}
	while (nullptr != second)
	{
		NODE *p = second;
		second = second->next;
		delete p;
	}
}

NODE *NodeManager::GetNode(int key)
{
	f_m.lock();
	if (nullptr == first)
	{
		f_m.unlock();
		return new NODE{ key };
	}
	NODE *p = first;
	first = first->next;
	f_m.unlock();
	p->key = key;
	p->next = nullptr;
	p->removed = false;
	return p;
}

void NodeManager::FreeNode(NODE *e)
{
	s_m.lock();
	e->next = second;
	second = e;
	s_m.unlock();
}

void NodeManager::Recycle()
{
	NODE *p = first;
	first = second;
	second = p;
}
