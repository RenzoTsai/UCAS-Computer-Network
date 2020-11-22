#define MAXLINE 100

typedef struct treeNode {
    int net;
    int prefix_len;
    int port;
    struct treeNode * left;
    struct treeNode * right; 
} TreeNode;

typedef struct {
    int net;
    int prefix_len;
    int port;
} RouterEntry;

typedef unsigned int uint;

RouterEntry* line_parser (char * line);

int net_parser(char * s);

TreeNode * init_tree();

TreeNode * root;

TreeNode * insert_tree(RouterEntry* entry, int p_len);

int add_node (RouterEntry* entry);

int get_mask (int prefix_len);

int lookup (int netID);

void compress_node(TreeNode * node);