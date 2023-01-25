#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include "fruits.h"

// foward declaration
typedef struct Node Node;

// smaller size pointer
// serializability dump node_pool on the other file
struct Node{
    char text[24];
    int32_t left;
    int32_t right;
};

#define NODE_POOL_CAP 1024

typedef struct {
    size_t sz;
    Node nodes[NODE_POOL_CAP];
}Node_Pool;

static Node_Pool global_node_pool;

Node* node_pool_alloc(Node_Pool* np) {
    assert(np->sz < NODE_POOL_CAP);
    Node* result = &np->nodes[np->sz];
    memset(result, 0, sizeof(Node));
    np->sz += 1;
    return result;
}

void node_set_text(Node* node, const char* text_cstr) {
    size_t n = strlen(text_cstr);
    if (n > sizeof(node->text) -1) {
        n = sizeof(node->text) -1;
    }

    memset(node->text, 0, sizeof(node->text));
    memcpy(node->text, text_cstr, n);
}

Node* node_pool_alloc_with_text(Node_Pool* np, const char* text_cstr) {
    Node* result = node_pool_alloc(np);
    node_set_text(result, text_cstr);
    return result;
}

#define abs2rel32(rloc, aptr) rloc = (int32_t) ((uint8_t*) (aptr) - ((uint8_t*)&rloc))
#define rel2abs32(Type, rloc) (Type*)((uint8_t*)&rloc + rloc)

void print_tree(FILE* stream, Node* node, size_t level) {
    for(size_t i = 0; i < level; ++i) fputs("   ", stream);
    fputs(node->text, stream);
    fputc('\n', stream);
    if(node->left) print_tree(stream, rel2abs32(Node, node->left),  level+1);
    if(node->right) print_tree(stream, rel2abs32(Node, node->right), level+1);
}

Node* random_tree(Node_Pool* np, size_t level) {
    Node* root = node_pool_alloc_with_text(np, fruits[rand()%fruits_count]);
    if(level > 1) abs2rel32(root->left,  random_tree(np, level -1));
    if(level > 1) abs2rel32(root->right, random_tree(np, level -1));
    return root;
}

void save_node_pool_to_file(Node_Pool* np, const char* file_path) {
    FILE* out = fopen(file_path, "w");
    if(out == NULL) {
        fprintf(stderr, "ERROR: could not write to file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }
    size_t n = fwrite(np->nodes, sizeof(Node), np->sz, out);
    assert(n == np->sz);
    if(ferror(out)) {
        fprintf(stderr, "ERROR: could not write to file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }
    fclose(out);
}

void load_node_pool_from_file(Node_Pool* np, const char* file_path) {
    FILE* out = fopen(file_path, "rb");
    if(out == NULL) {
        fprintf(stderr, "ERROR: could not write to file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }
    fseek(out, 0, SEEK_END);
    long m = ftell(out);
    fseek(out, 0, SEEK_SET);
    assert(m % sizeof(Node) == 0);
    np->sz = m / sizeof(Node);
    assert(np->sz <= NODE_POOL_CAP);
    size_t n = fread(np->nodes, sizeof(Node), np->sz, out);
    assert(n == np->sz);
    if(ferror(out)) {
        fprintf(stderr, "ERROR: could not write to file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }
    fclose(out);
}

#define FRUITS_BIN_PATH "fruits-tree.bin"

int main(void) {
    srand(time(0));
    Node* root = random_tree(&global_node_pool, 5);
    save_node_pool_to_file(&global_node_pool, FRUITS_BIN_PATH);
    print_tree(stdout, root, 0);
    
    load_node_pool_from_file(&global_node_pool, FRUITS_BIN_PATH);
    print_tree(stdout, &global_node_pool.nodes[0], 0);
    return 0;
}