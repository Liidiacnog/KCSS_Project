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

	KCSS::loc_t<int> v1(0);
	KCSS::loc_t<int> v2(20);
	KCSS::loc_t<int> v3(30);

	std::thread t1([&]() { // TODO what is the "&" ?
		for (int i = 1; i <= 100; i++){
			int exp = kcss_inst.get(v1);
			if (kcss_inst.kcss(v1, exp, 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "t1 -> Iteration " << i << ": t1 saw " << exp << " and changed v1 to: " << 1 << std::endl;
			}
		}
	});

	std::thread t2([&]() {
		for (int i = 1; i <= 100; i++){
			int exp = kcss_inst.get(v1);
			if (kcss_inst.kcss(v1, exp, 2, KCSS::mp(v2, 20), KCSS::mp(v3, 30))) {
				std::cout << "t2 -> Iteration " << i << ": t2 saw " << exp << " and changed v1 to: " << 2 << std::endl;
			}
		}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "Final value of v1 is: " << kcss_inst.get(v1) << std::endl;
}


void test_modify_nodes() { 
	struct node{
		node(int e, node* n): 
			elem(e), next(n) {}
			
		int elem;
		node* next;
	};

	// 1 -> 2 -> 3 -> nullptr
	node n3(3, nullptr);
	node n2(2, &n3);
	node n1(1, &n2);

    // std::cout << std::endl << "sizeof node: " << sizeof(node) << std::endl;
	// std::cout << std::endl << "sizeof node*: " << sizeof(node*) << std::endl;
	// std::cout << std::endl << "sizeof: node" << sizeof(node) << std::endl;

	KCSS kcss_inst;

	KCSS::loc_t<node*> v1(n1.next);
	KCSS::loc_t<node*> v2(n2.next);
	KCSS::loc_t<node*> v3(n3.next);

	std::thread t1([&]() { // TODO what is the "&" ?
		for (int i = 1; i <= 100; i++)
			node new_n2(200, &n3);
			if (kcss_inst.kcss(v2, &n2, &new_n2, KCSS::mp(v2, &n2), KCSS::mp(v3, &n3))) {
				std::cout << "Iteration " << i << ": t1 changed v1 to: " << i + 1 << std::endl;
			}
	});

	std::thread t2([&]() {
		for (int i = 1; i <= 100; i++)
			if (kcss_inst.kcss(v1, i, i + 1, KCSS::mp(v2, 2), KCSS::mp(v3, 3))) {
				std::cout << "Iteration " << i << ": t2 changed v1 to: " << i + 1 << std::endl;
			}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "Final value of v1 is: " << kcss_inst.get(v1) << std::endl;
}



int main() {

	// test0();
	test_modify_nodes();
	// test1();
}

