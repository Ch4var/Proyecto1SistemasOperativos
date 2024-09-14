#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lector.c"

#define MAX_TREE_HT 10000

typedef struct {
    char utf8_char[5];
    char code[256];  // El código Huffman, suponiendo que no excede los 255 bits
} Code;

struct Node {
    uint32_t value;
    unsigned freq;
    struct Node *left, *right;      
};

struct Tree {
    int currentSize;
    int capacity;
    struct Node** array;
};

struct Codes {
    uint32_t value;
    char* codigo;
};

struct Node* newNode(uint32_t value, unsigned freq) {
    struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
    if (!temp) { perror("Fallo en malloc para nodo"); exit(1); }
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

void getAllCodes(struct Node* root, char arr[], int top, Code *codes, int* index) {
    // Si el nodo es nulo, no hacemos nada
    if (root == NULL) {
        return;
    }

    // Si encontramos una hoja, guardamos el carácter y su código
    if (isLeaf(root)) {
        arr[top] = '\0';  // Terminar el código con null para indicar el final del string
        char character[5];
        unicode_to_utf8(root->value, character);  // Convertir el valor Unicode a UTF-8
        strcpy(codes[*index].utf8_char, character);  // Guardar el valor del carácter en el array de códigos
        strcpy(codes[*index].code, arr);    // Guardar el código Huffman generado
        (*index)++;  // Incrementar el índice para la siguiente entrada en el array codes
        return;
    }

    // Si hay nodo a la izquierda, añadimos '0' al código y llamamos recursivamente
    if (root->left) {
        arr[top] = '0';
        getAllCodes(root->left, arr, top + 1, codes, index);
    }

    // Si hay nodo a la derecha, añadimos '1' al código y llamamos recursivamente
    if (root->right) {
        arr[top] = '1';
        getAllCodes(root->right, arr, top + 1, codes, index);
    }
}

struct Node* HuffmanCodes(uint32_t value[], int freq[], int currentSize) {
    struct Node* root = buildHuffmanTree(value, freq, currentSize);
    return root; 
}
