#include <mutex>
#include <thread>
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <atomic>

using namespace std;
using namespace chrono;

static const int NUM_TEST = 10000000;

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

	NODE() {
		next = nullptr;
	}
	NODE(int x) {
		key = x;
		next = nullptr;
	}
	~NODE() {
	}
};

bool CAS(NODE **addr, NODE *old_v, NODE *new_v)
{
	return atomic_compare_exchange_strong(
		reinterpret_cast<atomic_int *>(addr),
		reinterpret_cast<int *>(&old_v),
		reinterpret_cast<int>(new_v)
	);
}

bool CAS(NODE * volatile *addr, NODE *old_v, NODE *new_v)
{
	return atomic_compare_exchange_strong(
		reinterpret_cast<volatile atomic_int *>(addr),
		reinterpret_cast<int *>(&old_v),
		reinterpret_cast<int>(new_v)
	);
}

class CSTACK
{
	NODE *top;
	null_mutex  m_lock;;
public:
	CSTACK()
	{
		top = nullptr;
	}
	void Init()
	{
		while (top != nullptr) {
			NODE *temp = top;
			top = top->next;
			delete temp;
		}
	}

	void Dump()
	{
		NODE *ptr = top;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Push(int x)
	{
		NODE *e = new NODE{ x };
		m_lock.lock();
		e->next = top;
		top = e;
		m_lock.unlock();
	}
	int Pop()
	{
		m_lock.lock();
		if (nullptr == top)
		{
			m_lock.unlock();
			return 0;
		}
		int temp = top->key;
		NODE *ptr = top;
		top = top->next;
		m_lock.unlock();
		delete ptr;
		return temp;
	}
};

class LFSTACK
{
	NODE * volatile top;
public:
	LFSTACK()
	{
		top = nullptr;
	}
	void Init()
	{
		while (top != nullptr) {
			NODE *temp = top;
			top = top->next;
			delete temp;
		}
	}

	void Dump()
	{
		NODE *ptr = top;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Push(int x)
	{
		NODE *e = new NODE{ x };
		while (true) {
			NODE *first = top;
			e->next = first;
			if (true == CAS(&top, first, e)) break;
		}
	}

	int Pop()
	{
		while (true) {
			NODE *first = top;
			if (nullptr == first) return 0;
			NODE *next = first->next;
			if (true == CAS(&top, first, next)) return first->key;
		}
		// delete first;
	}
};

class BACKOFF
{
	int minDelay;
	int maxDelay;
	int limit;
public:
	BACKOFF()
	{
		minDelay = 10;
		maxDelay = 100000;
		limit = minDelay;
	}
	void Delay()
	{
		if (limit < maxDelay) limit = limit * 2;
		int wait_t = rand() % limit + 1;
		int start, current;
		_asm push ecx;
		_asm mov ecx, wait_t;
	myloop:
		_asm loop myloop;
		_asm pop ecx;

	}

	void Delay_OLD()
	{
		if (limit < maxDelay) limit = limit * 2;
		int wait_t = rand() % limit;
		int start, current;
		_asm RDTSC;
		_asm mov start, eax;
		do {
			_asm RDTSC;
			_asm mov current, eax;
		} while (current - start < wait_t);
	}
};

class LFBOSTACK
{
	NODE * volatile top;
public:
	LFBOSTACK()
	{
		top = nullptr;
	}
	void Init()
	{
		while (top != nullptr) {
			NODE *temp = top;
			top = top->next;
			delete temp;
		}
	}

	void Dump()
	{
		NODE *ptr = top;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Push(int x)
	{
		BACKOFF backoff;
		NODE *e = new NODE{ x };
		while (true) {
			NODE *first = top;
			e->next = first;
			if (true == CAS(&top, first, e)) break;
			backoff.Delay();
		}
	}

	int Pop()
	{
		BACKOFF backoff;
		while (true) {
			NODE *first = top;
			if (nullptr == first) return 0;
			NODE *next = first->next;
			if (true == CAS(&top, first, next)) return first->key;
			backoff.Delay();
		}
		// delete first;
	}
};

#define MAX_THREAD 64
thread_local int ex_size = 1;

#define ST_EMPTY 0
#define ST_WAIT 1
#define ST_BUSY 2

class EXCHANGER
{
	volatile int value;   // STATUS와 교환값의 합성
	bool CAS(int o_value, int n_value, int o_state, int n_state)
	{
		int old_v = o_value << 2 | o_state;
		int new_v = n_value << 2 | n_state;
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_int volatile *>(&value), &old_v, new_v);
	}
public:
	EXCHANGER() {
		value = 0;
	}
	int exchange(int x)
	{
		int count = 0;
		while (true) {
			switch (value & 0x3)
			{
			case ST_EMPTY:
			{
				int temp = value >> 2;
				if (false == CAS(temp, x, ST_EMPTY, ST_WAIT)) continue;
				/* ST_BUSY가 될 때 까지 기다리면서, TIME OUT이 된 경우는 -1을 리턴 */

				while (count < 100)
				{
					int d = value & 0x3;
					int v = value >> 2;
					if (d == ST_BUSY)
					{
						value = 0;
						return v;
					}
					count++;
				}

				if (CAS(x, 0, ST_WAIT, ST_EMPTY))
					return -1;
				else
				{
					int v = value >> 2;
					value = 0;
					return v;
				}
				break;
			}
			case ST_WAIT:
			{
				int temp = value >> 2;
				if (false == CAS(temp, x, ST_WAIT, ST_BUSY)) continue;
				return temp;
				break;
			}
			case ST_BUSY:
				if (ex_size < MAX_THREAD)
					ex_size++;
				break;
			default:
				cout << "ERROR STATE\n";
				system("pause");
			}
		}
	}

};

class EL_ARRAY
{
	EXCHANGER exchg[MAX_THREAD];
public:
	int visit(int x)
	{
		int index = rand() % ex_size;
		return exchg[index].exchange(x);
	}
	void shrink()
	{
		if (ex_size > 1) ex_size--;
	}
};

class LFELSTACK
{
	EL_ARRAY el_array;
	NODE * volatile top;
public:
	LFELSTACK()
	{
		top = nullptr;
	}
	void Init()
	{
		while (top != nullptr) {
			NODE *temp = top;
			top = top->next;
			delete temp;
		}
	}

	void Dump()
	{
		NODE *ptr = top;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Push(int x)
	{
		BACKOFF backoff;
		NODE *e = new NODE{ x };
		while (true) {
			NODE *first = top;
			e->next = first;
			if (true == CAS(&top, first, e)) break;
			int res = el_array.visit(x);
			if (0 == res) break;
			if (-1 == res) el_array.shrink();
		}
	}

	int Pop()
	{
		BACKOFF backoff;
		while (true) {
			NODE *first = top;
			if (nullptr == first) return 0;
			NODE *next = first->next;
			if (true == CAS(&top, first, next)) return first->key;
			int res = el_array.visit(0);
			if (0 == res) continue;
			if (-1 == res) el_array.shrink();
			return res;
		}
		// delete first;
	}
};

class FREE_LIST {
	NODE *head;
	NODE *tail;
public:
	FREE_LIST() {
		head = new NODE{ -1 };
		tail = head;
	}
	~FREE_LIST() {
		while (tail != head) {
			NODE *temp = head;
			head = head->next;
			delete temp;
		}
		delete head;
	}
	NODE *GetNode(int x)
	{
		if (tail == head) return new NODE(x);
		NODE *temp = head;
		head = head->next;
		temp->key = x;
		temp->next = nullptr;
		return temp;
	}
	void FreeNode(NODE *ptr)
	{
		tail->next = ptr;
		tail = ptr;
	}
};

thread_local FREE_LIST free_list;

LFELSTACK my_stack;

void benchmark(int num_thread)
{
	for (int i = 1; i < NUM_TEST / num_thread; ++i) {
		if ((rand() % 2) || (i < 1000 / num_thread))
			my_stack.Push(i);
		else
			my_stack.Pop();
	}
}

int main()
{
	vector <thread> worker;
	for (int num_thread = 1; num_thread <= 32; num_thread *= 2) {
		my_stack.Init();
		worker.clear();

		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i)
			worker.push_back(thread{ benchmark, num_thread });
		for (auto &th : worker) th.join();
		auto du = high_resolution_clock::now() - start_t;
		my_stack.Dump();

		cout << num_thread << " Threads,  Time = ";
		cout << duration_cast<milliseconds>(du).count() << " ms\n";
	}
	system("pause");
}
