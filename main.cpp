#include <iostream>

#include "kcss.h"
#include <thread>


void test0() {
	KCSS m;

	KCSS::loc_t<int> v1(10);
	KCSS::loc_t<int> v2(20);
	KCSS::loc_t<int> v3(30);

	std::thread t1([&]() {
		for (int i = 0; i < 100; i++)
			if (m.kcss(v1, i, i + 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "t1 changed v1 to: " << i + 1 << std::endl;
			}
	});

	std::thread t2([&]() {
		for (int i = 0; i < 150; i++) {
			if (m.kcss(v1, i, 2 * i, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "t2 changed v1 to: " << 2 * i << std::endl;
			}
		}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "final value of v1 is: " << m.get(v1)
			<< std::endl;

}

int main() {

	test0();

}

