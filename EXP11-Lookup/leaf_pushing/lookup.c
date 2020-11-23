#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "lookup.h"

long total_mem = 0;

int main() {
    FILE * fd = fopen("forwarding-table.txt","r");
    if (fd == NULL) {
        return 0;
    }
    char * line = (char*)malloc(MAXLINE) ;
    
    root = init_tree();

    while(fgets(line, MAXLINE, fd) != NULL) {
        RouterEntry* entry;
        entry = line_parser(line);
        add_node(entry);
    }
    printf("totoal mem: %ld\n", total_mem);

    // input the net id
    // while(1) {
    //     scanf("%s",line);
    //     printf("port:%d\n",lookup(net_parser(line)));
    // }
    double total_time = 0;
    double other_time = 0;
    double time_use = 0;
    struct timeval start, end;
    int input_num = 0;

    fseek(fd, 0, SEEK_SET);
    gettimeofday(&start,NULL); 
    while(fgets(line, MAXLINE, fd) != NULL) {
        char * net;
        net = strtok(line, " ");
        lookup(net_parser(net));
        input_num++;
    }
    gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)*1E9+(end.tv_usec-start.tv_usec)*1E3;

    total_time = time_use/input_num;

    input_num = 0;
    fseek(fd, 0, SEEK_SET);
    gettimeofday(&start,NULL); 
    while(fgets(line, MAXLINE, fd) != NULL) {
        char * net;
        net = strtok(line, " ");
        net_parser(net);
        input_num++;
    }
    gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)*1E9+(end.tv_usec-start.tv_usec)*1E3;
    other_time  = time_use/input_num;
    
    printf("time_use is %.9f\n",total_time - other_time);
    
    return 0;
}

RouterEntry* line_parser (char * line) {
    char * net;
    char * prefix;
    char * port;
    RouterEntry* entry = (RouterEntry*)malloc(sizeof(RouterEntry));
    total_mem += sizeof(RouterEntry);

    net = strtok(line, " ");
    prefix = strtok(NULL, " ");
    port = strtok(NULL, " ");
    entry->net = net_parser(net);
    entry->prefix_len = atoi(prefix);
    entry->port = atoi(port);
    return entry;
}

int net_parser(char * s) {
    unsigned int num = 0;
    int i = 0;
    int time = 0;
    while(time < 3) {
        num = num + atoi(&s[i]);
        num = num * 256;
        for (; i < strlen(s) && s[i] != '.'; i++) {
        }
        i++;
        time ++;
    }
    num = num + atoi(&s[i]);
    return num;
}

TreeNode * init_tree() {
    TreeNode * root = (TreeNode *) malloc(sizeof(TreeNode));
    total_mem += sizeof(TreeNode);
    if (root == NULL) {
        exit(0);
    }
    root->port = 0; //default port
    return root;
}

TreeNode * insert_tree(RouterEntry* entry, int p_len, TreeNode* parent) {
    TreeNode * treeNode = (TreeNode *) malloc(sizeof(TreeNode));
    total_mem += sizeof(TreeNode);
    if (treeNode == NULL) {
        exit(0);
    }
    if (entry->prefix_len != p_len) {
        treeNode->port = parent->port;
    } else {
        treeNode->port = entry->port; 
    }
    return treeNode;
}

int get_mask (int prefix_len) {
    int mask = 0x80000000;
    return mask >> (prefix_len-1);
}

int add_node (RouterEntry* entry) {
    TreeNode * ptr = root;
    unsigned int prefix_bit = 0x80000000;

    int insert_num = 0;
    for (int i = 1; i <= entry->prefix_len; i++) {
        if ((entry->net&prefix_bit) == 0) {
            if (ptr->left == NULL) {
                ptr->left = insert_tree(entry, i, ptr);
            } 
            ptr = ptr->left;
        } else {
            if (ptr->right == NULL) {
                 ptr->right = insert_tree(entry, i, ptr);
            } 
            ptr = ptr->right;
        }
        prefix_bit = prefix_bit >> 1;
    }


    total_mem -= sizeof(RouterEntry);
    free(entry);
    return ptr->port;
}

int lookup (int netID) {
    TreeNode * ptr = root;
    unsigned int prefix_bit = 0x80000000;
    int i;
    int port;
    for (i = 1; i < 32; i++) {
        if ((netID & prefix_bit) == 0) {
            if (ptr->left == NULL) {
                port = ptr->port; 
                break;
            } 
            ptr = ptr->left;
        } else {
            if (ptr->right == NULL) {
                port = ptr->port; 
                break;
            } 
            ptr = ptr->right;
        }
        prefix_bit = prefix_bit >> 1;
    }
    return port;

}


