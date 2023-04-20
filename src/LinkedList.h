#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_
    #include <stdbool.h>
    #include "messages.h"
    #include <sys/wait.h>
    typedef struct node Node;
    typedef struct ll LinkedList;

    LinkedList* createLL();
    bool isEmpty(LinkedList*);
    void addLL(LinkedList*, MessageStart);
    MessageStart iterateLL(LinkedList*);
    int getLLSize(LinkedList*);
    void freeLinkedList(LinkedList*);
    void removeLL(LinkedList *, pid_t);

#endif
