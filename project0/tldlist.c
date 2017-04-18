#include "tldlist.h"

struct tldlist {
	TLDNode *root;
	int total;

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
	TLDNode *inorder;
};

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
	tld->root = tld_insert_helper(tld->root, hostname);
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
	return 0;
}

/*
 * tldlist_iter_next returns the next element in the list; returns a pointer
 * to the TLDNode if successful, NULL if no more elements to return
 */
TLDNode *tldlist_iter_next(TLDIterator *iter){
	return 0;
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

//------------------ Internal Utility Functions --------------------

/*
 * tldnode_create generates a new TLDNode to house the TLD and its
 * frequency for use in a TLDList
 * 
 * Creates a single TLDNode with the specified parent and TLD with 
 * a frequency count of 1. Returns the address of the new node if
 * successful, NULL otherwise.
 */
TLDNode *tldnode_create(TLDNode *p, char *d){
	TLDNode *newnode = (TLDNode *) malloc(sizeof(TLDNode));
	
	// Allocate memory for the domain name and copy it in
	newnode->domain = (char *) malloc(strlen(d));
	strcpy(newnode->domain, d);
	
	// Set the child pointers to NULL
	newnode->left = 0;
	newnode->right = 0;

	// Set the node's parent
	newnode->parent = p;

	// Set the node's height to 1, as it's a leaf upon creation
	newnode->height = 1;

	// Return
	return newnode;
}

/*
 * tldnode_find locates a specific domain in the list and passes it 
 * back.
 * 
 * Returns a pointer to the TLDNode that houses the domain. If the
 * domain isn't in the list, it returns NULL.
 */
TLDNode *tldnode_find(TLDList *tld, char *d){
	TLDNode *scrutiny = tld->root;
	while(scrutiny != 0){
		int direction = strcmp(scrutiny->domain, d);
		if (direction == 0)
			return scrutiny;
		else if (direction < 0)
			scrutiny = scrutiny->left;
		else
			scrutiny = scrutiny->right;
	}
	
	// If it's reached here, then it wasn't in the list.
	return 0;
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
 * KUDOS: The AVL Tree article on geeksforgeeks.org for the core logic. No
 * particular author was specified that I could find.
 */
TLDNode *tld_insert_helper(TLDNode *scrutiny, char *domain){
	// Standard BST insertion
	if (scrutiny == 0)
		return tldnode_create()
}
