#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_TREE_HT 10000

struct Node {
    uint32_t value;
    unsigned freq;
    struct Node *left, *right;
};

struct Tree {
    unsigned currentSize;
    unsigned capacity;
    struct Node** array;
};

struct Node* newNode(uint32_t value, unsigned freq) {
    struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
    temp->left = temp->right = NULL;
    temp->value = value;
    temp->freq = freq;
    return temp;
}

struct Tree* createTree(unsigned capacity) {
    struct Tree* Tree = (struct Tree*)malloc(sizeof(struct Tree));
    Tree->currentSize = 0;
    Tree->capacity = capacity;
    Tree->array = (struct Node**)malloc(Tree->capacity * sizeof(struct Node*));
    return Tree;
}

void swapNode(struct Node** a, struct Node** b) {
    struct Node* temp = *a;
    *a = *b;
    *b = temp;
}

void Treeify(struct Tree* Tree, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < Tree->currentSize && Tree->array[left]->freq < Tree->array[smallest]->freq)
        smallest = left;

    if (right < Tree->currentSize && Tree->array[right]->freq < Tree->array[smallest]->freq)
        smallest = right;

    if (smallest != index) {
        swapNode(&Tree->array[smallest], &Tree->array[index]);
        Treeify(Tree, smallest);
    }
}

int isSizeOne(struct Tree* Tree) {
    return (Tree->currentSize == 1);
}

struct Node* extractMinNode(struct Tree* Tree) {
    struct Node* temp = Tree->array[0];
    Tree->array[0] = Tree->array[Tree->currentSize - 1];
    --Tree->currentSize;
    Treeify(Tree, 0);
    return temp;
}

void insertTree(struct Tree* Tree, struct Node* Node) {
    ++Tree->currentSize;
    int i = Tree->currentSize - 1;

    while (i && Node->freq < Tree->array[(i - 1) / 2]->freq) {
        Tree->array[i] = Tree->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    Tree->array[i] = Node;
}

void buildTree(struct Tree* Tree) {
    int n = Tree->currentSize - 1;
    for (int i = (n - 1) / 2; i >= 0; --i)
        Treeify(Tree, i);
}

int isLeaf(struct Node* root) {
    return !(root->left) && !(root->right);
}

struct Tree* createAndBuildTree(uint32_t value[], int freq[], int currentSize) {
    struct Tree* Tree = createTree(currentSize);
    for (int i = 0; i < currentSize; ++i)
        Tree->array[i] = newNode(value[i], freq[i]);
    Tree->currentSize = currentSize;
    buildTree(Tree);
    return Tree;
}

struct Node* buildHuffmanTree(uint32_t value[], int freq[], int currentSize) {
    struct Node *left, *right, *top;
    struct Tree* Tree = createAndBuildTree(value, freq, currentSize);

    while (!isSizeOne(Tree)) {
        left = extractMinNode(Tree);
        right = extractMinNode(Tree);

        top = newNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;

        insertTree(Tree, top);
    }

    return extractMinNode(Tree);
}

void printCodes(struct Node* root, int arr[], int top) {
    if (root->left) {
        arr[top] = 0;
        printCodes(root->left, arr, top + 1);
    }

    if (root->right) {
        arr[top] = 1;
        printCodes(root->right, arr, top + 1);
    }

    if (isLeaf(root)) {
        printf("U+%04X: ", root->value);

        for (int i = 0; i < top; ++i)
            printf("%d", arr[i]);

        printf("\n");
    }
}

void HuffmanCodes(uint32_t value[], int freq[], int currentSize) {
    struct Node* root = buildHuffmanTree(value, freq, currentSize);
    int arr[MAX_TREE_HT], top = 0;
    printCodes(root, arr, top);
}


