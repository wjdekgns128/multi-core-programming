#include "SET.h"

CSET::CSET()
{
	head.key = 0x80000000;
	tail.key = 0x7FFFFFFF;
	head.next = &tail;
}

CSET::~CSET()
{
}

void CSET::Init()
{
	while (head.next != &tail)
	{
		NODE *temp = head.next;
		head.next = temp->next;
		delete temp;
	}
}

bool CSET::Add(int key)
{
	while (true)
	{
		NODE * pred, *curr;
		pred = &head;
		curr = pred->next;

		while (curr->key < key)
		{
			pred = curr;
			curr = curr->next;
		}
		pred->lock();
		curr->lock();

		if (validate(pred, curr))
		{
			if (key == curr->key)
			{
				curr->unlock();
				pred->unlock();
				return false;
			}
			else
			{
				NODE* node = node_pool.GetNode(key);
				node->next = curr;
				pred->next = node;
				curr->unlock();
				pred->unlock();
				return true;
			}
		}
		pred->unlock();
		curr->unlock();
	}
}

bool CSET::Remove(int key)
{
	while (true)
	{
		NODE *pred, *curr;
		pred = &head;
		curr = pred->next;

		while (curr->key < key)
		{
			pred = curr;
			curr = curr->next;
		}
		pred->lock();
		curr->lock();

		if (validate(pred, curr))
		{
			if (key == curr->key)
			{
				curr->removed = true;
				pred->next = curr->next;
				pred->unlock();
				curr->unlock();
				node_pool.FreeNode(curr);
				return true;
			}
			else
			{
				pred->unlock();
				curr->unlock();
				return false;
			}
		}
		pred->unlock();
		curr->unlock();
	}
}

bool CSET::Contain(int key)
{
	NODE *curr;
	curr = &head;

	while (curr->key < key)
	{
		curr = curr->next;
	}

	if (curr->key == key && !curr->removed)
		return true;
	else
		return false;
}

bool CSET::validate(NODE* pred, NODE* curr)
{
	if (!pred->removed && !curr->removed && pred->next == curr)
		return true;
	else
		return false;
}

void CSET::Dump()
{
	NODE *ptr = head.next;
	
	std::cout << "Result Contains : ";

	for (int i = 0; i < 20; ++i)
	{
		std::cout << ptr->key << " ";
		if (&tail == ptr)
			break;
		
		ptr = ptr->next;
	}

	std::cout << std::endl;
}

