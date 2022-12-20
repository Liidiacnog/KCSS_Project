#include "kcss.h"

template<typename T>
class Multiset {
private:

    struct node{
        node(const T& newkey, node* newnext) :
			key(newkey), count(1), next(newnext) {
		}
        T key;
        KCSS::loc_t<uint64_t> count; //multiplicity of key in the set
        KCSS::loc_t<node*> next;
    };

    KCSS kcss; 


    /**
     * attempts to remove node curr by doing a kcss operation to set
     * prev->next to curr_next, and returns the pointer to prev.
     **/
    node* remove_node(node* prev, node* curr, node* curr_next){
        kcss.kcss(prev->next, curr, curr_next, 
            (prev->next, curr), (&prev->count, prev->count),  //TODO okay specified like this?
             (curr->next, curr_next), (&curr->count, 0)); //(mem.positions to check, expvals)    
    }


    /**
     * returns 2 adjacent nodes (i.e., the next field of one points to the other), 
     * the first with a key less than the given key, and the second with a key 
     * greater than or equal to the given key. 
     * To reduce the space overhead of the list, this procedure also removes from
     * the list any node it encounters whose count is 0. Thus, the search procedure 
     * never returns a node whose count field is 0 (at the time the search 
     * procedure read the field).
     * **/
    std::pair<node*, node*> search(const T& given_key){
        if(first == nullptr){
            printf("Attempted search on empty multiset");
            exit(EXIT_FAILURE);
        }

        node* prev_node = first;
        node* cur_node = (node*) kcss.read(prev_node->next);
        uint64_t cur_count = kcss.read(cur_node->count);

        while(cur_node != nullptr && cur_node->key < given_key){ //TODO do we have to use .equals?
            if(cur_count == 0){
                uint64_t prev_count = kcss.read(prev_node->count);
                if(prev_count == 0){
                    return search(given_key);//restart search()
                }else{
                    node* curr_next = (node*) kcss.read(cur_node->next); 
                    prev_node = remove_node(prev_node, cur_node, curr_next);
                }
            }else{
                prev_node = cur_node;
            }
            //cur_node is prev_node->next bc prev_node has been updated in else{} or in remove_node to already become cur_node 
            cur_node = (node*) kcss.read(prev_node->next); 
            cur_count = kcss.read(cur_node->count);
        }

        return std::pair<node*, node*>(prev_node, cur_node);
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
    uint64_t multiplicity(const T& given_key){
        std::pair<node*, node*> pair_prev_cur = search(given_key);
        node* cur_node = pair_prev_cur.second;
        if(cur_node->key == given_key){
            return kcss.read(&cur_node->count);
        }else{
            return 0;
        }
    }

    /**
     * increases the multiplicity of a given key by 1 and returns
     *  the multiplicity of the key before the insertion;
    **/
    uint64_t insert(const T& given_key){
        //TODO insert
        return 0;
    }

    /**
     * decreases the multiplicity of a given key by 1 (unless
     * the multiplicity is already 0 prior to the deletion) and
     *  returns the multiplicity of the key before the deletion.
    */
    uint64_t deleteKey(const T& given_key){
        std::pair<node*, node*> pair_prev_cur = search(given_key);
        node* cur_node = pair_prev_cur.second;
        node* prev_node = pair_prev_cur.first;
        if(cur_node->key == given_key){
            uint64_t oldcount = kcss.read(&cur_node->count);
            while(oldcount > 0){
                if(kcss.kcss(&cur_node->count, oldcount, oldcount - 1)){
                    return oldcount;
                }
                oldcount = kcss.read(&cur_node->count);
            }
            return 0;
        }else{ //node with given_key not found
            return 0;
        }
    }
};