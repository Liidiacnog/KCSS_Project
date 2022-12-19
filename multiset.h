#include <kcss.h>

template<typename T>
class Multiset {
private:

    struct node{
        T key;
        uint64_t count; //multiplicity of key in the set
        node* next;
    }

    KCSS kcss; 

    /**
     * returns 2 adjacent nodes (i.e., the next field of one points to the other), 
     * the first with a key less than the given key, and the second with a key 
     * greater than or equal to the given key. 
     * To reduce the space overhead of the list, this procedure also removes from
     * the list any node it encounters whose count is 0. Thus, the search procedure 
     * never returns a node whose count field is 0 (at the time the search 
     * procedure read the field).
     * **/
    node* search(const T& given_key){
        if(first == nullptr){
            printf("Attempted search on empty multiset");
            exit(EXIT_FAILURE);
        }

        node* iter_node = first->next;
        node* prev_node = first;
        while(iter_node != nullptr && iter_node->key < given_key){ //TODO do we have to use .equals?
            if(iter_node->count <= 0){
                remove_node(prev_node, iter_node);//TODO remove_node()
                //restart search()
                return search(given_key);
            }else{
                if(prev_node->key )
            }

            prev_node = iter_node;
            iter_node = iter_node->next;
        }

        if(iter_node == nullptr){//not found
            printf("Attempted search on invalid key");
            exit(EXIT_FAILURE);
        }else if(iter_node->key >= given_key){
            //TODO , and maybe add next elem restriction?
        }
    }

public: 
    /**
     * Multiset implementation as a linked list of nodes
     * We will maintain it ordered in increasing order by node->key
     * 
     * The multiset is initialized with two sentinel nodes, first 
     * and last, which have keys that are respectively smaller and 
     * larger than any key that is passed to a multiset operation.
    **/
    struct node* first;
    struct node* last;


    //returns the multiplicity of a given key; 
    uint64_t multiplicity(const T& key){
        //TODO multiplicity
    }

    /**
     * increases the multiplicity of a given key by 1 and returns
     *  the multiplicity of the key before the insertion;
    **/
    uint64_t insert(const T& key){
        //TODO insert
    }

    /**
     * decreases the multiplicity of a given key by 1 (unless
     * the multiplicity is already 0 prior to the deletion) and
     *  returns the multiplicity of the key before the deletion.
    */
    uint64_t delete(const T& key){
        //TODO delete
    }
};