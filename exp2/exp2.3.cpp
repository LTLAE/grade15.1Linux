#include <iostream>
#include <stdlib.h>

// I use cpp only because I don't want to use scanf
// Absolutely true dude cin and cout is the best

using namespace std;

class Node {
public:
    string data;
    Node* next;
};

int main() {
    Node *head = (Node *)malloc(sizeof(Node));
    head->next = (Node *)malloc(sizeof(Node));
    Node *scanner = head->next;
    cout << "Please input your string. Input 114514 to exit." << endl;
    string temp = "";
    // input some strings
    while (1){
        cin >> temp;
        if (temp == "114514") break;
        scanner->data = temp;
        // apply for space
        scanner->next = (Node *)malloc(sizeof(Node));
        scanner = scanner->next;
    }
    scanner->next = NULL;
    // return input strings
    scanner = head->next;
    while (scanner->next != NULL){
        cout << scanner->data << endl;
        scanner = scanner->next;
    }

}
