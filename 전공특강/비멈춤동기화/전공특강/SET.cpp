#include "SET.h"

CSET::CSET()
{
	head.key = 0x80000000;
	tail.key = 0x7FFFFFFF;
	head.SetNext(&tail);
}

CSET::~CSET()
{
}

void CSET::Init()
{
	while (head.GetNext() != &tail)
	{
		NODE *temp = head.GetNext();
		head.next = temp->next;
		delete temp;
	}
}

bool CSET::Add(int key)
{	
	NODE *pred, *curr;

	while (true)
	{
		Find(key, &pred, &curr);

		if (key == curr->key) 
		{
			return false;
		}
		else
		{
			NODE* node = new NODE(key);
			node->SetNext(curr);
			if (true == pred->CAS(curr, node, false, false))
			{
				return true;
			}
		}
		
	}
}

bool CSET::Remove(int key)
{
	NODE *pred, *curr;
	
	while (true)
	{
		Find(key, &pred, &curr);

		if (key != curr->key)
		{
			return false;
		}
		else
		{
			NODE *su = curr->GetNext();
			bool removed;
			removed = curr->TryMark();

			if (false == removed)
				continue;
			pred->CAS(curr, su, false, false);
			return true;
		}
	}
	return true;
}

bool CSET::Contain(int key)
{
	NODE *curr;
	curr = &head;
	bool removed;

	while (curr->key < key)
	{
		curr = curr->GetNext();

		NODE *su = curr->GetNextWithMark(&removed);
	}

	return (key == curr->key && false == removed);
	return true;
}

void CSET::Find(int key, NODE ** pred, NODE ** curr)
{
retry:
	NODE *pr = &head;
	NODE *cu = pr->GetNext();

	while (true)
	{
		bool removed;
		NODE *su = cu->GetNextWithMark(&removed);

		while (true == removed)
		{
			if (false == pr->CAS(cu, su, false, false))
				goto retry;
			cu = su;
			su = cu->GetNextWithMark(&removed);
		}
		if (cu->key >= key)
		{
			*pred = pr;
			*curr = cu;
			return;
		}
		pr = cu;
		cu = cu->GetNext();
	}
}


void CSET::Dump()
{
	NODE *ptr = head.GetNext();
	
	std::cout << "Result Contains : ";

	for (int i = 0; i < 20; ++i)
	{
		std::cout << ptr->key << " ";
		if (&tail == ptr)
			break;
		
		ptr = ptr->GetNext();
	}

	std::cout << std::endl;
}

