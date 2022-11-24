#include <chrono>
#include <iomanip>
#include <iostream>

#include "kcss.h"
#include <thread>
#include <chrono>

using std::__1::chrono::high_resolution_clock;
using std::__1::chrono::microseconds;
using namespace std::chrono_literals;

void test0(std::size_t n) {
	KCSS m;

	KCSS::loc_t<int> v1(10);
	KCSS::loc_t<int> v2(20);
	KCSS::loc_t<int> v3(30);


	auto f = [&]() {
		while (true) {
			int x = m.get(v1);
			if (x > 1000000)
				break;
			m.kcss(v1, x, x + 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30));
			// m.kcss(v1, x, x + 1);
		}
	};

	std::thread t[n];
	for (auto i = 0u; i < 5; i++)
		t[i] = std::thread(f);
	for (auto i = 0u; i < 5; i++)
		t[i].join();


//	std::cout << std::endl << "final value of v1 is: " << m.get(v1)
//			<< std::endl;

}



bool mutexcas(int *p, int expv, int newv, std::mutex &m) {
	m.lock();
	bool res = false;
	if (*p == expv) {
		*p = newv;
		res = true;
	}
	m.unlock();
	return res;
}

void test1(std::size_t n) {

	std::mutex m;

	int x = 10;

	auto f = [&]() {
		bool done = 0;
		while (!done) {
			if (x > 1000000)
				break;
			mutexcas(&x, x, x + 1, m);
//			m.lock();
//			if (y == 20 && z == 30) {
//				x += 1;
//			}
//			m.unlock();
		}
	};

	
	std::thread t[n];
	for (auto i = 0u; i < 5; i++)
		t[i] = std::thread(f);
	for (auto i = 0u; i < 5; i++)
		t[i].join();


//	std::cout << std::endl << "final value of x is: " << x << std::endl;

}

void foo() {
	struct double_layout {
		uint64_t mantisa :52;
		uint64_t exp :11;
		uint64_t sign :1;
	};

	union {
		double d;
		double_layout dl;
	};

	d = 0.123456789;
	std::cout << dl.mantisa << std::endl;
	std::cout << dl.exp << std::endl;
	std::cout << dl.sign << std::endl;

	KCSS::loc_t<double> v1;
	uint64_t d1 = v1.to_value_t(d);
	double d2 = v1.from_value_t(d1);
	std::cout.precision(std::numeric_limits<double>::max_digits10);
	std::cout << d << std::endl;
	std::cout << d2 << std::endl;
	std::cout << std::bitset<64>(*reinterpret_cast<uint64_t*>(&d)) << std::endl;
	std::cout << std::bitset<64>(*reinterpret_cast<uint64_t*>(&d2))
			<< std::endl;

	if (d == d2)
		std::cout << "Yes!" << std::endl;

	long double bla = 1.3523345345345345;
	std::cout << sizeof(bla) << " " << bla << std::endl;
}

void test_0() {
	auto t1 = high_resolution_clock::now();
	test0(20);
	auto t2 = high_resolution_clock::now();
	test1(20);
	auto t3 = high_resolution_clock::now();

	auto d1 = duration_cast<microseconds>(t2 - t1);
	auto d2 = duration_cast<microseconds>(t3 - t2);
	std::cout << d1.count() << std::endl;
	std::cout << d2.count() << std::endl;
	std::cout << d1.count() * 1.0 / d2.count() << std::endl;

}

void bar() {

	KCSS m;

	struct node {
		node() :
				x(0), next(nullptr) {
		}
		KCSS::loc_t<int> x;
		KCSS::loc_t<node*> next;
	};

	node *n1 = new node();
	node *n2 = new node();

	m.kcss(n1->next, (node*) nullptr, n2);

}

int sum(int x) {
	int s = 0;
	for (int i = 0; i < x; ++i)
		s += i;
	return s;
}

// sum(5)  sum1<5>()

template<int x>
inline int sum1() {
	return x + sum1<x - 1>();
}

template<>
inline int sum1<0>() {
	return 0;
}



template<typename ...Ts>
void f(Ts &&...args) {
}


int main() {

	int x;
	f(x, 1, 2);

	std::cout << sum1<10>() << std::endl;

	struct ff {
		int8_t v :7;
		int8_t x :1;

	};

	ff a;
	a.x = 0;
	a.v = -65;

	int8_t y = a.v;

	std::cout << std::bitset<8>(-65) << std::endl;

	std::cout << std::bitset<8>(*reinterpret_cast<int8_t*>(&a)) << std::endl;
	std::cout << std::bitset<8>(y) << std::endl;

	exit(0);
	for (auto i = 0u; i < 10; ++i)
		test_0();
//	foo();
}

