#include <stdio.h>
#include <stdlib.h>

#define DATA_SIZE 6

long data[DATA_SIZE] = {4, 8, 15, 16, 23, 42};

struct _Node {
    long value;
    struct _Node* next;
};

typedef struct _Node Node;

Node* add_element(long value, Node* next) {
    Node* node = malloc(16);
    if (node == NULL) abort();
    node->value = value;
    node->next = next;
    return node;
}

// iterate over elements (map)
void print_list(Node* root) {
    if (root == NULL) return;
    printf("%ld ", root->value);
    fflush(stdout);
    print_list(root->next);
}

// filter elements (filter)
Node* odd_elements(Node* root, Node* result) {
    if (root == NULL) return result;
    if (root->value & 1) result = add_element(root->value, result);
    return odd_elements(root->next, result);
}

void free_nodes(Node* root) {
    Node* next = NULL;
    while (root) {
        next = root->next;
        free(root);
        root = next;
    }
}

int main() {
    Node* root = NULL;
    for (int i = DATA_SIZE - 1; i >= 0; --i) root = add_element(data[i], root);

    print_list(root);
    puts("");

    Node* odd = odd_elements(root, NULL);

    print_list(odd);
    puts("");

    free_nodes(root);
    free_nodes(odd);

    return 0;
}
