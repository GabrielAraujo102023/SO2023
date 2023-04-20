#include "LinkedList.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct node{
    MessageStart data;
    struct node* next;
}Node;

typedef struct ll{
    Node* head;
    Node* tail;
    int size;
    Node *pointer;
}LinkedList;

LinkedList* createLL(){
    LinkedList* list = (LinkedList*) malloc (sizeof(LinkedList));
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    list->pointer = NULL;
    return list;
}

Node* createNode(MessageStart data){
    Node* node = (Node*) malloc (sizeof(Node));
    node->data = data;
    node->next = NULL;
    return node;
}

bool isEmpty(LinkedList* list){
    return list->size == 0;
}

void addLL(LinkedList* list, MessageStart data){
    Node* node = createNode(data);
    if(isEmpty(list)){
        list->head = node;
        list->tail = node;
    } else {
        node->next = list->head;
        list->head = node;
    }
    list->size++;
}

MessageStart iterateLL(LinkedList* list){
    if(!list->pointer){
        list->pointer = list->head;
        return list->pointer->data;
    } else if(list->pointer == list->tail){
        list->pointer = list->head;
        return list->pointer->data;
    }
    list->pointer = list->pointer->next;
    return list->pointer->data;
}

int getLLSize(LinkedList* list){
    if(!list)
        return 0;
    return list->size;
}

void freeLinkedList(LinkedList *list)
{
    Node* node;

    for(int i = 0; i < list->size; i++)
    {
        node = list->head;
        list->head = node->next;
        free(node);
    }
    free(list);
    return;
}

void removeLL(LinkedList *list, pid_t pid){
    Node *node = list->head;
    if(node && node->data.pid == pid){
        list->head = node->next;
        free(node);
        list->size--;
        return;
    }
    node = node->next;
    Node *prevNode = list->head;
    for(int i = 1; i < list->size; i++){
        if(node && node->data.pid == pid){
            prevNode->next = node->next;
            free(node);
            list->size--;
            return;
        }
        prevNode = node;
        node = node->next;
    }
    if(node && node->data.pid == pid){
        list->tail = prevNode;
        free(node);
        list->size--;
    }
}