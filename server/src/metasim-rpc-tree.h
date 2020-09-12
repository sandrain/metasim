#ifndef __METASIM_RPC_TREE_H
#define __METASIM_RPC_TREE_H

typedef struct {
    int rank;         /* global rank of calling process */
    int ranks;        /* number of ranks in tree */
    int parent_rank;  /* parent rank, -1 if root */
    int child_count;  /* number of children */
    int* child_ranks; /* list of child ranks */
} metasim_rpc_tree_t;

/* given the process's rank and the number of ranks, this computes a k-ary
 * tree rooted at rank 0, the structure records the number of children
 * of the local rank and the list of their ranks */
int metasim_rpc_tree_init(
    int rank,         /* rank of calling process */
    int ranks,        /* number of ranks in tree */
    int root,         /* rank of root process */
    int k,            /* degree of k-ary tree */
    metasim_rpc_tree_t* t /* output tree structure */
);

/* free resources allocated in metasim_rpc_tree_init */
void metasim_rpc_tree_free(metasim_rpc_tree_t* t);

#endif /* __METASIM_RPC_TREE_H */
