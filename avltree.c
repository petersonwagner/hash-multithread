#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define COUNT 10

typedef struct node_t
{
    unsigned long int key;
    struct node_t *left;
    struct node_t *right;
    int depth;
    char name[16];
    char phone_number[16];
} node_t;


/*hash_function que retorna um valor para ser usado como indice na arvore avl
obs: É NECESSARIO UMA FUNCAO DIFERENTE DA HASH PARA INDEXAR A HASH
PARA QUE A ARVORE NAO SE TORNE UMA LISTA ENCADEADA*/
unsigned long int hash_function2 (unsigned char *str)
{
    unsigned long int h1 = ((unsigned long int *) str)[0];
    unsigned long int h2 = ((unsigned long int *) str)[1];
    return h1 + h2;
}

int depth(node_t *root)
{
    if (!root)
        return -1;

    return root->depth;
}

void update_depth(node_t *root)
{
    if (!root)
        return;

    int left_depth = depth(root->left),
    right_depth = depth(root->right);

    if (left_depth < right_depth)
        root->depth = 1 + right_depth;
    else
        root->depth = 1 + left_depth;
}

unsigned long int maxDepth(node_t* node)
{
   if (node==NULL) 
       return 0;
   else
   {
       /* compute the depth of each subtree */
       int lDepth = maxDepth(node->left);
       int rDepth = maxDepth(node->right);
 
       /* use the larger one */
       if (lDepth > rDepth) 
           return(lDepth+1);
       else return(rDepth+1);
   }
}

unsigned long int maxSize(node_t* node) 
{  
  if (node==NULL) 
    return 0;
  else    
    return(maxSize(node->left) + 1 + maxSize(node->right));  
}

int factor(node_t *root)
{
    if (!root)
        return 0;

    return depth(root->left) - depth(root->right);
}

node_t* rotate_left(node_t *root)
{
    node_t *pivot = root->right;
    root->right = pivot->left;
    pivot->left = root;

    update_depth(root);
    update_depth(pivot);

    return pivot;
}

node_t* rotate_right(node_t *root)
{
    node_t *pivot = root->left;
    root->left = pivot->right;
    pivot->right = root;

    update_depth(root);
    update_depth(pivot);

    return pivot;
}


node_t* new_node(char name[16], char phone_number[16])
{
    node_t *created_node = (node_t*) malloc (sizeof(node_t));
    memset (created_node, 0, sizeof(node_t));
    memcpy (created_node->name, name, 16);
    memcpy (created_node->phone_number, phone_number, 16);
    created_node->key = hash_function2 (name);

    return created_node;
}

void delete_node(node_t *node_to_delete)
{
    free(node_to_delete);
}

void delete_tree(node_t *root)
{
    if (!root)
        return;

    delete_tree(root->left);
    delete_tree(root->right);

    delete_node(root);
}

node_t* tree_insert (node_t *root, node_t *node_to_insert)
{
    if (!root)
        return node_to_insert;

    if (root->key == node_to_insert->key)
    {
        return root;
    }

    if (root->key < node_to_insert->key)
        root->right = tree_insert(root->right, node_to_insert);
    else
        root->left = tree_insert(root->left, node_to_insert);

    update_depth(root);

    if (factor(root) > 1)
    {
        if (factor(root->left) < 0)
            root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    else if (factor(root) < -1)
    {
        if (factor(root->right) > 0)
            root->right = rotate_right(root->right);
        return rotate_left(root);
    }

    return root;
}

node_t* tree_search(node_t *root, char *name, unsigned long int key)
{
    if (!root)
        return NULL;

    if (root->key == key) //TALVEZ TENHA UM BUG AQUI CASO INSIRA-SE DOIS DADOS COM A MESMA CHAVE
        return root;

    else if (root->key > key)
        return tree_search(root->left, name, key);

    return tree_search(root->right, name, key);
}
