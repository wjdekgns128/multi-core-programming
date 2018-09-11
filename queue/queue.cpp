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

class CQUEUE
{
	NODE *head, *tail;
	null_mutex  enq_l, deq_l;
public:
	CQUEUE()
	{
		head = new NODE{ 0 };
		tail = head;
	}
	void Init()
	{
		while (head->next != nullptr) {
			NODE *temp = head;
			head = head->next;
			delete temp;
		}
		tail = head;
	}

	void Dump()
	{
		NODE *ptr = head->next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Enq(int x)
	{
		NODE *e = new NODE{ x };
		enq_l.lock();
		tail->next = e;
		tail = e;
		enq_l.unlock();
	}
	int Deq()
	{
		deq_l.lock();
		NODE *temp = head;
		if (nullptr == head->next) {
			deq_l.unlock();
			return -1;
		}
		int value = head->next->key;
		head = head->next;
		deq_l.unlock();
		delete temp;
		return value;
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

class LFQUEUE
{
	NODE * volatile head, * volatile tail;
public:
	LFQUEUE()
	{
		head = new NODE{ 0 };
		tail = head;
	}
	void Init()
	{
		while (head->next != nullptr) {
			NODE *temp = head;
			head = head->next;
			delete temp;
		}
		tail = head;
	}

	void Dump()
	{
		NODE *ptr = head->next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Enq(int x)
	{
		NODE *e = new NODE(x);
		while (true) {
			NODE *last = tail;
			NODE *next = last->next;
			if (last != tail) continue;
			if (NULL == next) {
				if (CAS(&(last->next), NULL, e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else CAS(&tail, last, next);
		}
	}

	int Deq()
	{
		while (true) {
			NODE *first = head;
			NODE *last = tail;
			NODE *next = first->next;
			if (first != head) continue;
			if (first == last) {
				if (next == NULL) return -1;
				CAS(&tail, last, next);
				continue;
			}
			int value = next->key;
			if (false == CAS(&head, first, next)) continue;
			delete first;
			return value;
		}
	}
};

__declspec(align(8)) struct SPN {
	NODE * volatile ptr;
	volatile unsigned int stamp;
};

bool STAMP_CAS(SPN *addr, SPN old_v, SPN new_v)
{
	new_v.stamp = old_v.stamp + 1;
	return atomic_compare_exchange_strong(
		reinterpret_cast<atomic_llong *>(addr),
		reinterpret_cast<long long *>(&old_v),
		*reinterpret_cast<long long *>(&new_v));
}

bool STAMP_CAS(SPN *addr, SPN old_v, NODE *new_node)
{
	SPN new_v{ new_node, old_v.stamp + 1 };
	return atomic_compare_exchange_strong(
		reinterpret_cast<atomic_llong *>(addr),
		reinterpret_cast<long long *>(&old_v),
		*reinterpret_cast<long long *>(&new_v));
}
class SLFQUEUE
{
	SPN head, tail;
public:
	SLFQUEUE()
	{
		head.ptr = new NODE{ 0 };
		tail.ptr = head.ptr;
	}

	void Init()
	{
		while (head.ptr->next != nullptr) {
			NODE *temp = head.ptr;
			head.ptr = head.ptr->next;
			delete temp;
		}
		tail.ptr = head.ptr;
	}

	void Dump()
	{
		NODE *ptr = head.ptr->next;
		cout << "Result Contains : ";
		for (int i = 0; i < 20; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << endl;
	}
	void Enq(int x)
	{
//		NODE *e = new NODE(x);
		NODE *e = free_list.GetNode(x);
		while (true) {
			SPN last = tail;
			SPN next;
			next.ptr = last.ptr->next;
			if (last.stamp != tail.stamp) continue;
			if (nullptr == next.ptr) {
				if (CAS(&(last.ptr->next), nullptr, e)) {  // 이것도 Stamp를 찍어 놓아야 한다.
					STAMP_CAS(&tail, last, e);
					return;
				}
			}
			else STAMP_CAS(&tail, last, next);
		}
	}

	int Deq()
	{
		while (true) {
			SPN first = head;
			SPN last = tail;
			SPN next; 
			next.ptr = first.ptr->next;

			if (first.stamp != head.stamp) continue;
			if (next.ptr == nullptr) return -1;
			if (first.ptr == last.ptr) {
				STAMP_CAS(&tail, last, next);
				continue;
			}
			int value = next.ptr->key;
			if (false == STAMP_CAS(&head, first, next)) continue;
			free_list.FreeNode(first.ptr);
			return value;
		}
	}
};

//CQUEUE my_queue;
//LFQUEUE my_queue;
SLFQUEUE my_queue;

void benchmark(int num_thread)
{
	for (int i = 0; i < NUM_TEST / num_thread; ++i) {
		if ((rand() % 2) || (i < 1000 / num_thread) )
			my_queue.Enq(i);
		else 
			my_queue.Deq(); 
	}
}

int main()
{
	vector <thread> worker;
	for (int num_thread = 1; num_thread <= 128; num_thread *= 2) {
		my_queue.Init();
		worker.clear();

		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i)
			worker.push_back(thread{ benchmark, num_thread });
		for (auto &th : worker) th.join();
		auto du = high_resolution_clock::now() - start_t;
		my_queue.Dump();

		cout << num_thread << " Threads,  Time = ";
		cout << duration_cast<milliseconds>(du).count() << " ms\n";
	}
	system("pause");
}
