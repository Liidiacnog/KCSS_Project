#include <iostream>
#include <atomic>
#include "kcss.h"
#include <thread>


void test0() {
	KCSS kcss_inst;

	KCSS::loc_t<int> v1(10);
	KCSS::loc_t<int> v2(20);
	KCSS::loc_t<int> v3(30);

	std::thread t1([&]() { // TODO what is the "&" ?
		for (int i = 0; i < 100; i++)
			if (kcss_inst.kcss(v1, i, i + 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "t1 changed v1 to: " << i + 1 << std::endl;
			}
	});

	std::thread t2([&]() {
		for (int i = 0; i < 150; i++) {
			if (kcss_inst.kcss(v1, i, 2 * i, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "t2 changed v1 to: " << 2 * i << std::endl;
			}
		}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "final value of v1 is: " << kcss_inst.get(v1) << std::endl;

}


void test1() {
	KCSS kcss_inst;

	KCSS::loc_t<int> v1(1);
	KCSS::loc_t<int> v2(20);
	KCSS::loc_t<int> v3(30);

	std::thread t1([&]() { // TODO what is the "&" ?
		for (int i = 1; i <= 100; i++)
			if (kcss_inst.kcss(v1, i, i + 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "Iteration " << i << ": t1 changed v1 to: " << i + 1 << std::endl;
			}
	});

	std::thread t2([&]() {
		for (int i = 1; i <= 100; i++)
			if (kcss_inst.kcss(v1, i, i + 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "Iteration " << i << ": t2 changed v1 to: " << i + 1 << std::endl;
			}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "Final value of v1 is: " << kcss_inst.get(v1) << std::endl;
}



int main() {

	// test0();
	test1();
}

