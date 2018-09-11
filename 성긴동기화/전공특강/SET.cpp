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
	NODE * pred, *curr;

	pred = &head;
	glock.lock();
	curr = pred->next;
	while (curr->key < key)
	{
		pred = curr;
		curr = curr->next;
	}

	if (key == curr->key)
	{
		glock.unlock();
		return false;
	}
	else
	{
		NODE* node = new NODE(key);
		node->next = curr;
		pred->next = node;
		glock.unlock();
		return true;
	}
}

bool CSET::Remove(int key)
{
	NODE *pred, *curr;

	pred = &head;
	glock.lock();
	curr = pred->next;
	while (curr->key < key)
	{
		pred = curr;
		curr = curr->next;
	}

	if (key == curr->key)
	{
		pred->next = curr->next;
		delete curr;
		glock.unlock();
		return true;
	}
	else
	{
		glock.unlock();
		return false;
	}
}

bool CSET::Contain(int key)
{
	NODE *pred, *curr;

	pred = &head;
	glock.lock();
	curr = pred->next;
	while (curr->key < key)
	{
		pred = curr;
		curr = curr->next;
	}

	if (key == curr->key)
	{
		glock.unlock();
		return true;
	}
	else
	{
		glock.unlock();
		return false;
	}
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

