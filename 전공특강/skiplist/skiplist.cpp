#include <mutex>
#include <thread>
#include <iostream>
#include <chrono>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

using namespace std;
using namespace chrono;

static const int NUM_TEST = 4000000;
static const int RANGE = 1000;
static const int MAX_LEVEL = 10;

class null_mutex
{
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:	
	int key;
	NODE *next;
	mutex nlock;
	bool removed;

	NODE() {
		next = nullptr;
		removed = false;
	}
	NODE(int x) {
		key = x;
		next = nullptr;
		removed = false;
	}
	~NODE() {
	}
	void Lock() 
	{ 
		nlock.lock(); 
	}
	void Unlock()
	{
		nlock.unlock();
	}
};

class SPNODE {
public:
	int key;
	shared_ptr<SPNODE> next;
	mutex nlock;
	bool removed;

	SPNODE() {
		next = nullptr;
		removed = false;
	}
	SPNODE(int x) {
		key = x;
		next = nullptr;
		removed = false;
	}
	~SPNODE() {
	}
	void Lock()
	{
		nlock.lock();
	}
	void Unlock()
	{
		nlock.unlock();
	}
};

class LFNODE {
public:
	int key;
	unsigned next;

	LFNODE() {
		next = 0;
	}
	LFNODE(int x) {
		key = x;
		next = 0;
	}
	~LFNODE() {
	}
	LFNODE *GetNext() {
		return reinterpret_cast<LFNODE *>(next & 0xFFFFFFFE);
	}

	void SetNext(LFNODE *ptr) {
		next = reinterpret_cast<unsigned>(ptr);
	}

	LFNODE *GetNextWithMark(bool *mark) {
		int temp = next;
		*mark = (temp % 2) == 1;
		return reinterpret_cast<LFNODE *>(temp & 0xFFFFFFFE);
	}

	bool CAS(int old_value, int new_value)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_int *>(&next), 
			&old_value, new_value);
	}

	bool CAS(LFNODE *old_next, LFNODE *new_next, bool old_mark, bool new_mark) {
		unsigned old_value = reinterpret_cast<unsigned>(old_next);
		if (old_mark) old_value = old_value | 0x1;
		else old_value = old_value & 0xFFFFFFFE;
		unsigned new_value = reinterpret_cast<unsigned>(new_next);
		if (new_mark) new_value = new_value | 0x1;
		else new_value = new_value & 0xFFFFFFFE;
		return CAS(old_value, new_value);
	}

	bool TryMark(LFNODE *ptr)
	{
		unsigned old_value = reinterpret_cast<unsigned>(ptr) & 0xFFFFFFFE;
		unsigned new_value = old_value | 1;
		return CAS(old_value, new_value);
	}

	bool IsMarked() {
		return (0 != (next & 1));
	}
};

class CSET
{
	NODE head, tail;
	null_mutex  g_lock;
public:
	CSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	void Init()
	{
		while (head.next != &tail) {
			NODE *temp = head.next;
			head.next = temp->next;
			delete temp;
		}
	}
	void Dump()
	{
		NODE *ptr = head.next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->key << ", ";
			if (&tail == ptr) break;
			ptr = ptr->next;
		}
		cout << endl;
	}
	bool Add(int x)
	{
		NODE *pred, *curr;
		pred = &head;
		g_lock.lock();
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == x) {
			g_lock.unlock();
			return false;
		}
		else {
			NODE *e = new NODE{ x };
			e->next = curr;
			pred->next = e;
			g_lock.unlock();
			return true;
		}
	}
	bool Remove(int x)
	{
		NODE *pred, *curr;
		pred = &head;
		g_lock.lock();
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key != x) {
			g_lock.unlock();
			return false;
		}
		else {
			pred->next = curr->next;
			delete curr;
			g_lock.unlock();
			return true;
		}
	}
	bool Contains(int x)
	{
		NODE *pred, *curr;
		pred = &head;
		g_lock.lock();
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == x) {
			g_lock.unlock();
			return true;
		}
		else {
			g_lock.unlock();
			return false;
		}
	}
};

class FSET
{
	NODE head, tail;
public:
	FSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	void Init()
	{
		while (head.next != &tail) {
			NODE *temp = head.next;
			head.next = temp->next;
			delete temp;
		}
	}
	void Dump()
	{
		NODE *ptr = head.next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->key << ", ";
			if (&tail == ptr) break;
			ptr = ptr->next;
		}
		cout << endl;
	}
	bool Add(int x)
	{
		NODE *pred, *curr;
		head.Lock();
		pred = &head;
		curr = pred->next;
		curr->Lock();
		while (curr->key < x) {
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (curr->key == x) {
			curr->Unlock();
			pred->Unlock();
			return false;
		}
		else {
			NODE *e = new NODE{ x };
			e->next = curr;
			pred->next = e;
			curr->Unlock();
			pred->Unlock();
			return true;
		}
	}
	bool Remove(int x)
	{
		NODE *pred, *curr;
		head.Lock();
		pred = &head;
		curr = pred->next;
		curr->Lock();
		while (curr->key < x) {
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (curr->key != x) {
			curr->Unlock();
			pred->Unlock();
			return false;
		}
		else {
			pred->next = curr->next;
			curr->Unlock();
			pred->Unlock();
			delete curr;
			return true;
		}
	}
	bool Contains(int x)
	{
		NODE *pred, *curr;
		head.Lock();
		pred = &head;
		curr = pred->next;
		curr->Lock();
		while (curr->key < x) {
			pred->Unlock();
			pred = curr;
			curr = curr->next;
			curr->Lock();
		}

		if (curr->key == x) {
			curr->Unlock();
			pred->Unlock();
			return true;
		}
		else {
			curr->Unlock();
			pred->Unlock();
			return false;
		}
	}
};

class OSET
{
	NODE head, tail;
public:
	OSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	void Init()
	{
		while (head.next != &tail) {
			NODE *temp = head.next;
			head.next = temp->next;
			delete temp;
		}
	}
	void Dump()
	{
		NODE *ptr = head.next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->key << ", ";
			if (&tail == ptr) break;
			ptr = ptr->next;
		}
		cout << endl;
	}
	bool Validate(NODE *pred, NODE *curr)
	{
		NODE *ptr = &head;
		while (ptr->key <= pred->key) {
			if (ptr == pred) return pred->next == curr;
			ptr = ptr->next;
		}
		return false;
	}
	bool Add(int x)
	{
		NODE *pred, *curr;
		while (true) {
			pred = &head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}
			pred->Lock(); curr->Lock();

			if (false == Validate(pred, curr)) {
				curr->Unlock();
				pred->Unlock();
				continue;
			}

			if (curr->key == x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				NODE *e = new NODE{ x };
				e->next = curr;
				pred->next = e;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		NODE *pred, *curr;
		while (true) {
			pred = &head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}
			pred->Lock(); curr->Lock();

			if (false == Validate(pred, curr)) {
				curr->Unlock();
				pred->Unlock();
				continue;
			}
			if (curr->key != x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				pred->next = curr->next;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		NODE *pred, *curr;
		while (true) {
			pred = &head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}

			pred->Lock(); curr->Lock();
			if (false == Validate(pred, curr)) {
				curr->Unlock(); pred->Unlock();
				continue;
			}

			if (curr->key == x) {
				curr->Unlock();
				pred->Unlock();
				return true;
			}
			else {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
		}
	}
};

class NodeManager
{
	NODE *first, *second;
	mutex f_m, s_m;
public:
	NodeManager()
	{
		first = second = nullptr;
	}
	~NodeManager()
	{
		while (nullptr != first) {
			NODE *p = first;
			first = first->next;
			delete p;
		}
		while (nullptr != second) {
			NODE *p = second;
			second = second->next;
			delete p;
		}
	}
	NODE *GetNode(int x)
	{
		f_m.lock();
		if (nullptr == first) {
			f_m.unlock();
			return new NODE{ x };
		}
		NODE *p = first;
		first = first->next;
		f_m.unlock();
		p->key = x;
		p->next = nullptr;
		p->removed = false;
		return p;
	}
	void FreeNode(NODE *e)
	{
		s_m.lock();
		e->next = second;
		second = e;
		s_m.unlock();
	}
	void Recycle()
	{
		NODE *p = first;
		first = second;
		second = p;
	}
};

NodeManager node_pool;

class ZSET
{
	NODE head, tail;
public:
	ZSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}
	void Init()
	{
		while (head.next != &tail) {
			NODE *temp = head.next;
			head.next = temp->next;
			delete temp;
		}
	}
	void Dump()
	{
		NODE *ptr = head.next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->key << ", ";
			if (&tail == ptr) break;
			ptr = ptr->next;
		}
		cout << endl;
	}
	bool Validate(NODE *pred, NODE *curr)
	{
		return (false == pred->removed) && (false == curr->removed) && (pred->next == curr);
	}
	bool Add(int x)
	{
		NODE *pred, *curr;
		while (true) {
			pred = &head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}
			pred->Lock(); curr->Lock();

			if (false == Validate(pred, curr)) {
				curr->Unlock();
				pred->Unlock();
				continue;
			}

			if (curr->key == x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				NODE *e = node_pool.GetNode(x);
				e->next = curr;
				pred->next = e;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		NODE *pred, *curr;
		while (true) {
			pred = &head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}
			pred->Lock(); curr->Lock();

			if (false == Validate(pred, curr)) {
				curr->Unlock();
				pred->Unlock();
				continue;
			}
			if (curr->key != x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				curr->removed = true;
				pred->next = curr->next;
				curr->Unlock();
				pred->Unlock();
				node_pool.FreeNode(curr);
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		NODE *curr;
		curr = &head;
		while (curr->key < x) {
			curr = curr->next;
		}

		return (false == curr->removed) && (x == curr->key);
	}
};

class SPZSET
{
	shared_ptr <SPNODE> head, tail;
public:
	SPZSET()
	{
		head = make_shared <SPNODE>( 0x80000000 );
		tail = make_shared <SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}
	void Init()
	{
		head->next = tail;
	}
	void Dump()
	{
		shared_ptr<SPNODE> ptr = head->next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->key << ", ";
			if (nullptr == ptr) break;
			ptr = ptr->next;
		}
		cout << endl;
	}
	bool Validate(shared_ptr<SPNODE> pred, shared_ptr<SPNODE> curr)
	{
		return (false == pred->removed) && (false == curr->removed) && (pred->next == curr);
	}
	bool Add(int x)
	{
		shared_ptr <SPNODE> pred, curr;
		while (true) {
			pred = head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}
			pred->Lock(); curr->Lock();

			if (false == Validate(pred, curr)) {
				curr->Unlock();
				pred->Unlock();
				continue;
			}

			if (curr->key == x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				shared_ptr<SPNODE> e = make_shared<SPNODE>(x);
				e->next = curr;
				pred->next = e;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		shared_ptr<SPNODE> pred, curr;
		while (true) {
			pred = head;
			curr = pred->next;
			while (curr->key < x) {
				pred = curr; curr = curr->next;
			}
			pred->Lock(); curr->Lock();

			if (false == Validate(pred, curr)) {
				curr->Unlock();
				pred->Unlock();
				continue;
			}
			if (curr->key != x) {
				curr->Unlock();
				pred->Unlock();
				return false;
			}
			else {
				curr->removed = true;
				pred->next = curr->next;
				curr->Unlock();
				pred->Unlock();
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		shared_ptr<SPNODE> curr;
		curr = head;
		while (curr->key < x) {
			curr = curr->next;
		}
		return (false == curr->removed) && (x == curr->key);
	}
};

class LFSET
{
	LFNODE head, tail;
public:
	LFSET()
	{
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.SetNext(&tail);
	}
	void Init()
	{
		while (head.GetNext() != &tail) {
			LFNODE *temp = head.GetNext();
			head.next = temp->next;
			delete temp;
		}
	}

	void Dump()
	{
		LFNODE *ptr = head.GetNext();
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->key << ", ";
			if (&tail == ptr) break;
			ptr = ptr->GetNext();
		}
		cout << endl;
	}
	
	void Find(int x, LFNODE **pred, LFNODE **curr)
	{
	retry:
		LFNODE *pr = &head;
		LFNODE *cu = pr->GetNext();
		while (true) {
			bool removed;
			LFNODE *su = cu->GetNextWithMark(&removed);
			while (true == removed) {
				if (false == pr->CAS(cu, su, false, false))
					goto retry;
				cu = su;
				su = cu->GetNextWithMark(&removed);
			}
			if (cu->key >= x) {
				*pred = pr; *curr = cu;
				return;
			}
			pr = cu;
			cu = cu->GetNext();
		}
	}
	bool Add(int x)
	{
		LFNODE *pred, *curr;
		while (true) {
			Find(x, &pred, &curr);

			if (curr->key == x) {
				return false;
			}
			else {
				LFNODE *e = new LFNODE(x);
				e->SetNext(curr);
				if (false == pred->CAS(curr, e, false, false))
					continue;
				return true;
			}
		}
	}
	bool Remove(int x)
	{
		LFNODE *pred, *curr;
		while (true) {
			Find(x, &pred, &curr);

			if (curr->key != x) {
				return false;
			}
			else {
				LFNODE *succ = curr->GetNext();
				if (false == curr->TryMark(succ)) continue;
				pred->CAS(curr, succ, false, false);
				return true;
			}
		}
	}
	bool Contains(int x)
	{
		LFNODE *curr = &head;
		while (curr->key < x) {
			curr = curr->GetNext();
		}

		return (false == curr->IsMarked()) && (x == curr->key);
	}
};

class SKNODE
{
public:
	int value;
	SKNODE *next[MAX_LEVEL];
	int top_level;
	SKNODE()
	{
		value = 0;
		for (auto i = 0; i < MAX_LEVEL; ++i)
			next[i] = nullptr;
		top_level = 0;
	}
	SKNODE(int x)
	{
		value = x;
		for (auto i = 0; i < MAX_LEVEL; ++i)
			next[i] = nullptr;
		top_level = 0;
	}
	SKNODE(int x, int top)
	{
		value = x;
		for (auto i = 0; i < MAX_LEVEL; ++i)
			next[i] = nullptr;
		top_level = top;
	}
};

class CSKSET
{
	SKNODE head, tail;
	mutex  g_lock;
public:
	CSKSET()
	{
		head.value = 0x80000000;
		head.top_level = MAX_LEVEL - 1;
		tail.value = 0x7FFFFFFF;
		tail.top_level = MAX_LEVEL - 1;
		for (int i = 0; i < MAX_LEVEL; ++i)
			head.next[i] = &tail;
	}
	void Init()
	{
		while (head.next[0] != &tail) {
			SKNODE *temp = head.next[0];
			head.next[0] = temp->next[0];
			delete temp;
		}
		for (int i = 1; i < MAX_LEVEL; ++i)
			head.next[i] = &tail;
	}
	void Dump()
	{
		SKNODE *ptr = head.next[0];
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			cout << ptr->value << ", ";
			if (&tail == ptr) break;
			ptr = ptr->next[0];
		}
		cout << endl;
	}
	void Find(int x, SKNODE* preds[], SKNODE* currs[])
	{
		preds[MAX_LEVEL - 1] = &head;
		for (int level = MAX_LEVEL - 1; level >= 0; --level) {
			currs[level] = preds[level]->next[level];
			while (currs[level]->value < x) {
				preds[level] = currs[level];
				currs[level] = currs[level]->next[level];
			}
			if (0 != level) preds[level - 1] = preds[level];
		}
	}
	bool Add(int x)
	{
		SKNODE *pred[MAX_LEVEL], *curr[MAX_LEVEL];
		g_lock.lock();

		Find(x, pred, curr);

		if (curr[0]->value == x) {
			g_lock.unlock();
			return false;
		}
		else {
			int top = 0;
			while ((rand() % 2) == 1)
			{
				top++;
				if (top >= MAX_LEVEL - 1) break;
			}
			SKNODE *e = new SKNODE{ x, top };
			// Ãß°¡
			g_lock.unlock();
			return true;
		}
	}
	bool Remove(int x)
	{
		NODE *pred, *curr;
		pred = &head;
		g_lock.lock();
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key != x) {
			g_lock.unlock();
			return false;
		}
		else {
			pred->next = curr->next;
			delete curr;
			g_lock.unlock();
			return true;
		}
	}
	bool Contains(int x)
	{
		NODE *pred, *curr;
		pred = &head;
		g_lock.lock();
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == x) {
			g_lock.unlock();
			return true;
		}
		else {
			g_lock.unlock();
			return false;
		}
	}
};

LFSET my_set;


void benchmark(int num_thread)
{
	for (int i = 0; i < NUM_TEST / num_thread; ++i)	{
	//	if (0 == i % 100000) cout << ".";
		switch (rand() % 3) {
		case 0: my_set.Add(rand() % RANGE); break;
		case 1: my_set.Remove(rand() % RANGE); break;
		case 2: my_set.Contains(rand() % RANGE); break;
		default: cout << "ERROR!!!\n"; exit(-1);
		}
	}
}

int main()
{
	vector <thread> worker;
	for (int num_thread = 1; num_thread <= 16; num_thread *= 2) {
		my_set.Init();
		worker.clear();

		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i) 
			worker.push_back(thread{ benchmark, num_thread });
		for (auto &th : worker) th.join();
		auto du = high_resolution_clock::now() - start_t;
		node_pool.Recycle();
		my_set.Dump();

		cout << num_thread << " Threads,  Time = ";
		cout << duration_cast<milliseconds>(du).count() << " ms\n";
	}
	system("pause");
}
