

#include "tldlist.h"

struct tldlist {
	TLDNode *root;
	int total;
	int nodes;

	Date *begin;
	Date *end;
};

struct tldnode {
	TLDNode *left;
	TLDNode *right;
	int height;
	
	char *domain;
	int frequency;
};

struct tlditerator {
	int index;
	int max;
	TLDNode **inorder;
};

//------------------ Internal Utility Functions --------------------

/*
 * tldnode_create generates a new TLDNode to house the TLD and its
 * frequency for use in a TLDList
 * 
 * Creates a single TLDNode with the TLD with 
 * a frequency count of 1. Returns the address of the new node if
 * successful, NULL otherwise.
 */
TLDNode *tldnode_create(char *d){
	TLDNode *newnode = (TLDNode *) malloc(sizeof(TLDNode));
	
	// Allocate memory for the domain name and copy it in
	newnode->domain = (char *) malloc(strlen(d));
	strcpy(newnode->domain, d);
	
	// Set the child pointers to NULL
	newnode->left = 0;
	newnode->right = 0;

	// Set the node's height to 1, as it's a leaf upon creation
	newnode->height = 1;

	// Return
	return newnode;
}

// Helper function for height updates
int max_height(TLDNode *a, TLDNode *b){
	if ((a == 0) && (b == 0))
		return 0;
	else if (a == 0)
		return b->height;
	else if (b == 0)
		return a->height;
	else
		return (a->height > b->height)? a->height : b->height;
}

// Helper function to rotate nodes to the right rooted at the target
TLDNode *rotate_right(TLDNode *target){
	TLDNode *r = target->left;
	TLDNode *t2 = r->right;

	// Rotate
	r->right = target;
	target->left = t2;

	// Reevaluate heights
	target->height = 1 + max_height(target->right, target->left);
	r->height = 1 + max_height(r->right, r->left);

	// Return new root
	return r;
}

// Helper function to rotate nodes to the left rooted at the target
TLDNode *rotate_left(TLDNode *target){
	TLDNode *r = target->right;
	TLDNode *t2 = r->left;
	
	// Rotate
	r->left = target;
	target->right = t2;

	// Reevaluate heights
	target->height = 1 + max_height(target->right, target->left);
	r->height = 1 + max_height(r->right, r->left);

	// Return new root
	return r;
}

/*
 * tld_insert_helper recursively assists in adding new nodes to the
 * tree and balancing it afterwards if needed.
 * 
 * Recursively searches for where to insert a node of the given domain
 * into the tree. If it finds an entry already present, it just increments
 * the count. Otherwise, it attaches a new node to the appropriate child
 * with that domain and rebalances on its way back if necessary.
 *
 * DEVNOTE: The only reason we need to pass along the TLDList is to increment
 * the node number so the iterator works. Might be worth reworking that bit.
 * 
 * KUDOS: The AVL Tree article on geeksforgeeks.org for the core logic. No
 * particular author was specified that I could find.
 */
TLDNode *tld_insert_helper(TLDList *tld, TLDNode *scrutiny, char *domain){
	// Standard BST insertion
	if (scrutiny == 0){
		tld->nodes++;
		return tldnode_create(domain);
	}
	
	if (strcmp(domain, scrutiny->domain) < 0)
		scrutiny->left = tld_insert_helper(tld, scrutiny->left, domain);
	else if (strcmp(domain, scrutiny->domain) > 0)
		scrutiny->right = tld_insert_helper(tld, scrutiny->right, domain);
	else {
		scrutiny->frequency++;
		return scrutiny;
	}

	// Update height, as we've added a node
	scrutiny->height = 1 + max_height(scrutiny->left, scrutiny->right);

	// Check balance of node
	int balance = scrutiny->left->height - scrutiny->right->height;

	// Four-way case, depending on balance and where the new node belongs
	if ((balance > 1) && (strcmp(domain, scrutiny->left->domain) < 0))
		return rotate_right(scrutiny);
	
	if ((balance < -1) && (strcmp(domain, scrutiny->right->domain) > 0))
		return rotate_left(scrutiny);

	if ((balance > 1) && (strcmp(domain, scrutiny->left->domain) > 0)){
		scrutiny->left = rotate_left(scrutiny->left);
		return rotate_right(scrutiny);
	}

	if ((balance < -1) && (strcmp(domain, scrutiny->right->domain) < 0)){
		scrutiny->right = rotate_right(scrutiny->right);
		return rotate_left(scrutiny);
	}

	// If nothing changed
	return scrutiny;
}

// Helper function to build iterator - inorder traversal
void iter_builder(int *i, TLDNode **target, TLDNode *current){
	// Return on a null node
	if (current == 0)
		return;

	// Inorder traversal, building target array along way
	iter_builder(i, target, current->left);
	target[*i] = current;
	(*i)++;
	iter_builder(i, target, current->right);
}

//---------------------------- HEADED FUNCTIONS -----------------------

/*
 * tldlist_create generates a list structure for storing counts against
 * top level domains (TLDs)
 *
 * creates a TLDList that is constrained to the `begin' and `end' Date's
 * returns a pointer to the list if successful, NULL if not
 */
TLDList *tldlist_create(Date *begin, Date *end){
	TLDList *newlist = (TLDList *) malloc(sizeof(TLDList));
	newlist->root = 0;
	newlist->total = 0;
	newlist->nodes = 0;
	newlist->begin = date_duplicate(begin);
	newlist->end = date_duplicate(end);

	return newlist;
}

/*
 * tldlist_destroy destroys the list structure in `tld'
 *
 * all heap allocated storage associated with the list is returned to the heap
 */
void tldlist_destroy(TLDList *tld){
	free(tld->begin);
	free(tld->end);

	TLDIterator *iter = tldlist_iter_create(tld);
	TLDNode *garbage = tldlist_iter_next(iter);
	while(garbage != 0){
		free(garbage);   // I mean, *I* don't want it
		garbage = tldlist_iter_next(iter);
	}

	free(iter);
	free(tld);
	tld = 0;
}

/*
 * tldlist_add adds the TLD contained in `hostname' to the tldlist if
 * `d' falls in the begin and end dates associated with the list;
 * returns 1 if the entry was counted, 0 if not
 */
int tldlist_add(TLDList *tld, char *hostname, Date *d){
	// Return 0 if the date is out of range
	if ( (date_compare(tld->begin, d) < 0)
	     | (date_compare(d, tld->end) < 0) 
	   )
		return 0;

	// Start at the root node - and change it, if necessary
	tld->root = tld_insert_helper(tld, tld->root, hostname);

	// Increment the total number of successfully added TLDs
	tld->total++;

	// Return
	return 1;
}

/*
 * tldlist_count returns the number of successful tldlist_add() calls since
 * the creation of the TLDList
 */
long tldlist_count(TLDList *tld){
	return tld->total;
}

/*
 * tldlist_iter_create creates an iterator over the TLDList; returns a pointer
 * to the iterator if successful, NULL if not
 */
TLDIterator *tldlist_iter_create(TLDList *tld){
	TLDIterator *newiter = (TLDIterator *) malloc(sizeof(TLDIterator));

	if (newiter == 0)
		return 0;

	newiter->inorder = (TLDNode **) malloc(sizeof(TLDNode *) * tld->nodes);

	// Fill the iterator
	int i = 0;
	int *p = &i;
	iter_builder(p, newiter->inorder, tld->root);
	
	// Fill remaining fields
	newiter->index = 0;
	newiter->max = tld->nodes;	

	// Return
	return newiter;
}

/*
 * tldlist_iter_next returns the next element in the list; returns a pointer
 * to the TLDNode if successful, NULL if no more elements to return
 */
TLDNode *tldlist_iter_next(TLDIterator *iter){
	if (iter->index == iter->max)
		return 0;
	
	TLDNode *element = iter->inorder[iter->index];
	iter->index++;
	return element;
}

/*
 * tldlist_iter_destroy destroys the iterator specified by `iter'
 */
void tldlist_iter_destroy(TLDIterator *iter){
	free(iter->inorder);
	free(iter);
	iter = 0;
}

/*
 * tldnode_tldname returns the tld associated with the TLDNode
 */
char *tldnode_tldname(TLDNode *node){
	return node->domain;
}

/*
 * tldnode_count returns the number of times that a log entry for the
 * corresponding tld was added to the list
 */
long tldnode_count(TLDNode *node){
	return node->frequency;
}
