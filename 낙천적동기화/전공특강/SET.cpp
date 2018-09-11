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
				NODE* node = new NODE(key);
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
				pred->next = curr->next;
				pred->unlock();
				curr->unlock();
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
				pred->unlock();
				curr->unlock();
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

bool CSET::validate(NODE* pred, NODE* curr)
{
	NODE* node = &head;
	while (node->key <= pred->key)
	{
		if (node->key == pred->key)
		{
			return true;
		}
		node = node->next;
	}
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

