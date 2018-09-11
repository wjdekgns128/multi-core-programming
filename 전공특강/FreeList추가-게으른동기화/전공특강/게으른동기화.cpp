#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "SET.h"

using namespace std;
using namespace std::chrono;

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;
int num_thread;
CSET my_set;
NodeManager node_pool;

void benchmark(int num_thread)
{
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; i++)
	{
		switch (rand() % 3)
		{
		case 0:
			key = rand() % KEY_RANGE;
			my_set.Add(key);
			break;
		case 1:
			key = rand() % KEY_RANGE;
			my_set.Remove(key);
			break;
		case 2:
			key = rand() % KEY_RANGE;
			my_set.Contain(key);
			break;
		default:
			cout << "Error\n";
			exit(-1);
		}
	}
}

int main()
{
	vector<thread> worker;

	for (num_thread = 1; num_thread <= 16; num_thread *= 2)
	{
		my_set.Init();
		worker.clear();

		auto t = high_resolution_clock::now();
		for (auto i = 0; i < num_thread; ++i)
		{
			worker.push_back(thread{ benchmark, num_thread });
		}

		for (auto &th : worker)
		{
			th.join();
		}

		auto d = high_resolution_clock::now() - t;

		node_pool.Recycle();
		my_set.Dump();

		cout << num_thread << "Threads, Time = " << duration_cast<milliseconds>(d).count() << " msecs\n";
	}

	char c;
	cin >> c;
}

