#ifndef __AVLTREE__
#define __AVLTREE__

typedef struct node_t
{
    unsigned long int key;
    struct node_t *left;
    struct node_t *right;
    int depth;
    char name[16];
    char phone_number[16];
} node_t;

unsigned long int hash_function2 (unsigned char *str);
node_t* new_node(char name[16], char phone_number[16]);
void delete_node(node_t *node_to_delete);
void delete_tree(node_t *root);
node_t* tree_insert (node_t *root, node_t *node_to_insert);
node_t* tree_search(node_t *root, char *name, unsigned long int key);

#endif