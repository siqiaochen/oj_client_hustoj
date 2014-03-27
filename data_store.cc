#include <stdio.h>

struct node_pblm {
	int pblm_num;
	int testdata_id;
    struct node_pblm *next, *prev;
};

struct list_pblm {
	struct node_pblm head;
	struct node_pblm tail;
	int size;
}