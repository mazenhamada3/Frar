#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>

using string = std::string;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::getline;

struct TreeNode {
    char ch;
    int frequency;
    struct TreeNode* left;
    struct TreeNode* right;
};

struct QueueNode {
    struct TreeNode* treenode;
    struct QueueNode* next;
};

void enqueue(struct QueueNode** back, struct TreeNode* treenode) {
    struct QueueNode* newNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    newNode->treenode = treenode;
    newNode->next = NULL;

    if (*back == NULL || (*back)->treenode->frequency > newNode->treenode->frequency) {
        newNode->next = *back;
        *back = newNode;
    }
    else {
        struct QueueNode* it = *back;
        while (it->next != NULL && it->next->treenode->frequency <= newNode->treenode->frequency) {
            it = it->next;
        }
        newNode->next = it->next;
        it->next = newNode;
    }
}

struct TreeNode* createTreeNode(char ch, int frequency) {
    struct TreeNode* newNode = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    newNode->ch = ch;
    newNode->frequency = frequency;
    newNode->left = newNode->right = NULL;
    return newNode;
}

struct TreeNode* dequeue(struct QueueNode** back) {
    if (*back == NULL)
        return NULL;
    struct QueueNode* temp = *back;
    *back = (*back)->next;
    struct TreeNode* treeNode = temp->treenode;
    free(temp);
    return treeNode;
}

void FrequencyTable(char* filename, int* frequency) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening the file\n");
        return;
    }
    char c;
    while ((c = fgetc(file)) != EOF) {
        frequency[(unsigned char)c]++;
    }
    fclose(file);
}

struct TreeNode* buildHuffmanTree(struct QueueNode** queue) {
    while (*queue && (*queue)->next) {
        struct TreeNode* leftNode = dequeue(queue);
        struct TreeNode* rightNode = dequeue(queue);

        struct TreeNode* newNode = createTreeNode('\0', leftNode->frequency + rightNode->frequency);
        newNode->left = leftNode;
        newNode->right = rightNode;

        enqueue(queue, newNode);
    }
    return dequeue(queue);
}

void saveCodesToFile(struct TreeNode* root, char* code, int depth, FILE* file) {
    if (!root) return;

    if (!root->left && !root->right) {
        code[depth] = '\0';
        fprintf(file, "'%c': %s\n", root->ch, code);
        return;
    }

    if (root->left) {
        code[depth] = '0';
        saveCodesToFile(root->left, code, depth + 1, file);
    }

    if (root->right) {
        code[depth] = '1';
        saveCodesToFile(root->right, code, depth + 1, file);
    }
}

void printAndSaveCodes(struct TreeNode* root, const char* filename) {
    char code[100];
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Unable to open file for writing");
        return;
    }
    saveCodesToFile(root, code, 0, file);
    fclose(file);
}

void freeHuffmanTree(struct TreeNode* root) {
    if (!root) return;
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    free(root);
}

void generateCompressedFile(const string& inputFileName, const string& codeFileName) {
    ifstream inputFile(inputFileName.c_str(), ios::binary);
    ifstream codeFile(codeFileName.c_str());

    if (!inputFile || !codeFile) {
        perror("Error Opening input or code file");
        return;
    }

    string line;
    char codeTable[256][256] = { 0 };
    while (getline(codeFile, line)) {
        int delimiterPosition = line.find(':');
        if (delimiterPosition != string::npos) {
            char Character = line[1];
            string code = line.substr(delimiterPosition + 2);
            strcpy(codeTable[static_cast<unsigned char>(Character)], code.c_str());
        }
    }

    string outputFileName = inputFileName.substr(0, inputFileName.find_last_of('.')) + ".com";

    ofstream outputFile(outputFileName.c_str(), ios::binary);
    if (!outputFile) {
        fprintf(stderr, "Error Opening Output File %s\n", outputFileName.c_str());
        return;
    }

    unsigned char Byte = 0;
    int bitIndex = 0;
    char Characters;
    long bitLength = 0;

    while (inputFile.get(Characters)) {
        const char* code = codeTable[static_cast<unsigned char>(Characters)];
        if (*code != '\0') {
            bitLength += strlen(code);
        }
    }

    int extraBits = (8 - (bitLength % 8)) % 8;

    inputFile.clear();
    inputFile.seekg(0, ios::beg);

    outputFile.write(reinterpret_cast<const char*>(&bitLength), sizeof(bitLength));

    while (inputFile.get(Characters)) {
        const char* code = codeTable[static_cast<unsigned char>(Characters)];
        if (*code != '\0') {
            for (const char* p = code; *p != '\0'; p++) {
                Byte |= (*p - '0') << (7 - bitIndex);
                bitIndex++;
                if (bitIndex == 8) {
                    outputFile.put(Byte);
                    Byte = 0;
                    bitIndex = 0;
                }
            }
        }
    }

    if (bitIndex > 0) {
        outputFile.put(Byte);
    }

    for (int i = 0; i < extraBits; i++) {
        outputFile.put(0);
    }

    inputFile.close();
    codeFile.close();
    outputFile.close();
}

void decompress(FILE* input, FILE* output, TreeNode* root) {
    if (!root->left && !root->right) {
        fputc(root->ch, output);
        return;
    }

    int c;
    TreeNode* it = root;
    long bitLength;
    fread(&bitLength, sizeof(bitLength), 1, input);

    long counter = 0;
    while ((c = fgetc(input)) != EOF) {
        for (int i = 7; i >= 0; i--) {
            if (counter >= bitLength) {
                return;
            }
            int bit = (c >> i) & 1;
            if (bit == 1) {
                it = it->right;
            }
            else {
                it = it->left;
            }

            if (it->left == NULL && it->right == NULL) {
                fputc(it->ch, output);
                it = root;
            }
            counter++;
        }
    }
}

TreeNode* rebuildHuffmanTree(const char* codeFilename) {
    FILE* file = fopen(codeFilename, "r");
    if (!file) {
        perror("Unable to open code file");
        return NULL;
    }

    TreeNode* root = createTreeNode('\0', 0);
    char line[256];
    char ch;
    char code[256];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, " '%c': %s\n", &ch, code) == 2) {
            TreeNode* currentNode = root;
            for (char* p = code; *p; p++) {
                if (*p == '0') {
                    if (!currentNode->left)
                        currentNode->left = createTreeNode('\0', 0);
                    currentNode = currentNode->left;
                }
                else {
                    if (!currentNode->right)
                        currentNode->right = createTreeNode('\0', 0);
                    currentNode = currentNode->right;
                }
            }
            currentNode->ch = ch;
        }
    }

    fclose(file);
    return root;
}

int main(void) {
    char action;
    printf("Press C for compress or Press D for decompress: ");

    scanf(" %c", &action);

    if (action == 'c' || action == 'C') {
        char filename[256];
        printf("Enter the filename to compress: ");
        scanf("%255s", filename);

        int frequency[256] = { 0 };
        FrequencyTable(filename, frequency);

        struct QueueNode* queue = NULL;

        for (int i = 0; i < 256; i++) {
            if (frequency[i] > 0) {
                struct TreeNode* node = createTreeNode((char)i, frequency[i]);
                enqueue(&queue, node);
            }
        }

        struct TreeNode* huffmanTree = buildHuffmanTree(&queue);

        const char* codeFilename = "output.cod";
        printAndSaveCodes(huffmanTree, codeFilename);

        string inputFilename(filename);
        generateCompressedFile(inputFilename, codeFilename);

        freeHuffmanTree(huffmanTree);

        printf("File '%s' has been compressed.\n", filename);
    }
    else if (action == 'd' || action == 'D') {
        char filename[256];
        printf("Enter the filename to decompress: ");
        scanf("%255s", filename);

        struct TreeNode* huffmanTree = rebuildHuffmanTree("output.cod");

        FILE* inputFile = fopen(filename, "rb");
        if (!inputFile) {
            perror("Error opening input file");
            return 1;
        }

        std::string outputFilename = std::string(filename).substr(0, std::string(filename).find_last_of('.')) + "_decompressed.txt";
        FILE* outputFile = fopen(outputFilename.c_str(), "w");
        if (!outputFile) {
            perror("Error opening output file");
            fclose(inputFile);
            return 1;
        }

        decompress(inputFile, outputFile, huffmanTree);

        fclose(inputFile);
        fclose(outputFile);

        freeHuffmanTree(huffmanTree);

        printf("File '%s' has been decompressed to '%s'.\n", filename, outputFilename.c_str());
    }
    else {
        printf("Invalid action. Please enter 'c' for compression or 'd' for decompression.\n");
    }

    return 0;
}