#include <iostream>
#ifndef kcss_h
    #define kcss_h
    #include "kcss.h"
#endif
#include <thread>


// ---------------------------------------------------------------------------------- //

// Instantiate KCSS class
KCSS kcss_instance;

// Linked List definition: linked nodes using the following node structure
struct node {
	node():
		x(0), next(nullptr) {
	}
	node(int _x_val, node* _next_val):
		x(_x_val), next(_next_val) {
	}
	KCSS::loc_struct_t<int> x;
	KCSS::loc_struct_t<node*> next;
};

void print_list(node* first_node){
	node* next_node = first_node;
	while(next_node != nullptr){
		std::cout << kcss_instance.get(next_node->x) << "->";
		next_node = kcss_instance.get(next_node->next);
	}
	std::cout << "nullptr\n\n";
}

void cleanup(node* first_node){
	node* next_node = first_node;
	node* following;
	while(next_node != nullptr){
		following = kcss_instance.get(next_node->next);
		delete(next_node);
		next_node = following;
	}
}


void one_thread() {
	
	// Initial list: a(1) -> b(2) -> c(3) -> e(5)
	node *e = new node(5, nullptr);
	node *c = new node(3, e);
	node *b = new node(2, c);
	node *a = new node(1, b);

	std::cout << "Creating initial list consisting of:\n";
	print_list(a);
	
	// We want to delete node e(5). We will make use of KCSS where K = 2.
	//  The first location involved is c->next, and the second one is e->next
	std::cout << "Deleting node e(5)...\n";
	// Apply kcss operation:
	bool success = kcss_instance.kcss(c->next, e, (node*) nullptr, KCSS::mp(e->next, (node*) nullptr));
	
	if(success){ // Now it is safe to delete node e
		std::cout << "Deletion operation successful. Current list:\n";
		print_list(a);

		delete(e);
	}
	else{ // Failure of kcss
		std::cout << "Deletion operation unsuccessful. Current list:\n";
		print_list(a);
		
		// ... (possibly retry) ...
	}

	// Perform the desired actions with the list (...)

	cleanup(a);
}

void two_threads() {
	
	// Initial list: a(1) -> c(3) -> d(4)
	node *d = new node(4, nullptr);
	node *c = new node(3, d);
	node *a = new node(1, c);

	std::cout << "Creating initial list consisting of:\n";
	print_list(a);
	
	// We want T1 to delete node d(4), while T2 inserts b(2). 
	
	auto delete_d = [&]() { // Will make use of KCSS where K = 2.
		//  The first location involved is c->next, and the second one is d->next
		std::cout << "Deleting node d(4)...\n";
		// Apply kcss operation:
		bool success = kcss_instance.kcss(c->next, d, (node*) nullptr, KCSS::mp(d->next, (node*) nullptr));
		if(success){ // Now it is safe to delete node e
			std::cout << "Deletion operation successful.\n";
		}else{ // Failure of kcss
			std::cout << "Deletion operation unsuccessful.\n";
		}
	};
	auto insert_b = [&]() { // Will make use of KCSS where K = 1.
		node *b = new node(2, c);
		std::cout << "Inserting node b(2)...\n";
		bool success = kcss_instance.kcss(a->next, c, b);
		if(success){ // Now it is safe to delete node e
			std::cout << "Insertion operation successful.\n";
		}else{ // Failure of kcss
			std::cout << "Insertion operation unsuccessful.\n";
		}
	};
	
	int num_threads = 2;
	std::thread t[num_threads];
	t[0] = std::thread(delete_d);
	t[1] = std::thread(insert_b);
	
	t[0].join();
	t[1].join();
	
	
	std::cout << "Current list:\n";
	print_list(a);
	
	// Perform the desired actions with the list (...)
	
	cleanup(a);
}

void n_threads(std::size_t n) {
	
	KCSS::loc_struct_t<int> v1(10);
	KCSS::loc_struct_t<int> v2(20);
	KCSS::loc_struct_t<int> v3(30);

	auto f = [&]() {
		while (true) {
			int x = kcss_instance.get(v1);
			if (x > 100000)
				break;
			kcss_instance.kcss(v1, x, x + 1, KCSS::mp(v2, 20), KCSS::mp(v3, 30));
		}
	};

	std::thread t[n];
	for (auto i = 0u; i < n; i++)
		t[i] = std::thread(f);
	for (auto i = 0u; i < n; i++)
		t[i].join();


	std::cout << "\nThe final value of v1 is: " << kcss_instance.get(v1) << std::endl;

}

int main(){

	std::cout << "\nTest 1\n";
	one_thread();
	
	std::cout << "\nTest 2\n";
	two_threads();

	std::cout << "\nTest 3\n";
	n_threads(6);

	return 0;	
}

// ---------------------------------------------------------------------------------- //