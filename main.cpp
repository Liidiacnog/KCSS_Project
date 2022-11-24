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

	std::cout << "Initial address of n2: " << &n2 << std::endl;
	std::cout << "Initial address of n3: " << &n3 << std::endl;

	//trying to delete n2
	node* new_n1_next = &n3;
	std::thread t1([&]() { // TODO what is the "&" ?
	for (int i = 1; i <= 10; i++){
		node* exp_next_n2 = kcss_inst.get(v2);
		if (kcss_inst.kcss(v1, &n2, new_n1_next, KCSS::mp(v2, exp_next_n2), KCSS::mp(v3, n3.next))) {
			std::cout << "Iteration " << i << ": t1 changed v1 to: " << new_n1_next << std::endl;
		}
	}
	});

	//trying to add a new node between n2 and n3
	node n2_3(4, &n3);
	std::thread t2([&]() {
		for (int i = 1; i <= 10; i++){
			node* exp_next_n1 = kcss_inst.get(v1);
			if (kcss_inst.kcss(v2, &n3, &n2_3, KCSS::mp(v1, exp_next_n1), KCSS::mp(v3, n3.next))) {
				std::cout << "Iteration " << i << ": t2 changed v2 to: " << &n2_3 << std::endl;
			}
		}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "Final value of v1 is: " << kcss_inst.get(v1) << std::endl;
	std::cout << std::endl << "Final value of v2 is: " << kcss_inst.get(v1) << std::endl;
	
	//Critical section ended. Now update values of nodes with those managed by KCSS //TODO This way of updating real values doesn't scale

	std::cout << std::endl << "Initial state of list was: " << std::endl;
	std::cout << "n1: " << &n1 << " : (" << n1.elem << ", " << n1.next << ") -> " << std::endl;
	std::cout << "n2: " << &n2 << " : (" << n2.elem << ", " << n2.next << ") -> " << std::endl;
	std::cout << "n3: " << &n3 << " : (" << n3.elem << ", " << n3.next << ") -> " << std::endl;

	n1.next = v1.from_value_t(v1.val);
	n2.next = v1.from_value_t(v2.val);
	n3.next = v1.from_value_t(v3.val);
	// n2_3.next = 

	std::cout << std::endl << "Final value of list is: " << std::endl;
	std::cout << "n1: " << &n1 << " : (" << n1.elem << ", " << n1.next << ") -> " << std::endl;
	std::cout << "n2: " << &n2 << " : (" << n2.elem << ", " << n2.next << ") -> " << std::endl;
	std::cout << "n2_3: " << &n2_3 << " : (" << n2_3.elem << ", " << n2_3.next << ") -> " << std::endl;
	std::cout << "n3: " << &n3 << " : (" << n3.elem << ", " << n3.next << ") -> " << std::endl;
}


/* void test_modify_nodes2() { 
	struct node{
		node(int e, node* n): 
			elem(e), next(n) {}
			
		int elem;
		node* next;
	};

	1 -> 2 -> 3 -> nullptr
	node n3(3, nullptr);
	node n2(2, &n3);
	node n1(1, &n2);

    std::cout << std::endl << "sizeof node: " << sizeof(node) << std::endl;
	std::cout << std::endl << "sizeof node*: " << sizeof(node*) << std::endl;
	std::cout << std::endl << "sizeof: node" << sizeof(node) << std::endl;

	KCSS kcss_inst;

	KCSS::loc_t<node**> v1(&n1.next);
	KCSS::loc_t<node**> v2(&n2.next);
	KCSS::loc_t<node**> v3(&n3.next);

	std::cout << "Initial address of n2: " << &n2 << std::endl;
	std::cout << "Initial address of n3: " << &n3 << std::endl;

	trying to delete n2
	node* new_n1_next = &n3;
	std::thread t1([&]() { // TODO what is the "&" ?
	for (int i = 1; i <= 10; i++){
		node** exp_next_n2 = kcss_inst.get(v2);
		if (kcss_inst.kcss(v1, &n2, new_n1_next, KCSS::mp(v2, exp_next_n2), KCSS::mp(v3, n3.next))) {
			std::cout << "Iteration " << i << ": t1 changed v1 to: " << new_n1_next << std::endl;
		}
	}
	});

	trying to add a new node between n2 and n3
	node n2_3(4, &n3);
	std::thread t2([&]() {
		for (int i = 1; i <= 10; i++){
			node* exp_next_n1 = kcss_inst.get(v1);
			if (kcss_inst.kcss(v2, &n3, &n2_3, KCSS::mp(v1, exp_next_n1), KCSS::mp(v3, n3.next))) {
				std::cout << "Iteration " << i << ": t2 changed v2 to: " << &n2_3 << std::endl;
			}
		}
	});

	t1.join();
	t2.join();

	std::cout << std::endl << "Final value of v1 is: " << kcss_inst.get(v1) << std::endl;
	std::cout << std::endl << "Final value of v2 is: " << kcss_inst.get(v1) << std::endl;
	
	Critical section ended. Now update values of nodes with those managed by KCSS //TODO This way of updating real values doesn't scale

	std::cout << std::endl << "Initial state of list was: " << std::endl;
	std::cout << "n1: " << &n1 << " : (" << n1.elem << ", " << n1.next << ") -> " << std::endl;
	std::cout << "n2: " << &n2 << " : (" << n2.elem << ", " << n2.next << ") -> " << std::endl;
	std::cout << "n3: " << &n3 << " : (" << n3.elem << ", " << n3.next << ") -> " << std::endl;

	n1.next = v1.from_value_t(v1.val);
	n2.next = v1.from_value_t(v2.val);
	n3.next = v1.from_value_t(v3.val);
	n2_3.next = 

	std::cout << std::endl << "Final value of list is: " << std::endl;
	std::cout << "n1: " << &n1 << " : (" << n1.elem << ", " << n1.next << ") -> " << std::endl;
	std::cout << "n2: " << &n2 << " : (" << n2.elem << ", " << n2.next << ") -> " << std::endl;
	std::cout << "n2_3: " << &n2_3 << " : (" << n2_3.elem << ", " << n2_3.next << ") -> " << std::endl;
	std::cout << "n3: " << &n3 << " : (" << n3.elem << ", " << n3.next << ") -> " << std::endl;
}
 */


int main() {

	// test0();
	test_modify_nodes();
	// test1();
}

