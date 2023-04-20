#ifndef _MESSAGES_H_
#define _MESSAGES_H_
#include <sys/wait.h>
#include <sys/time.h>
typedef struct ms
{
    pid_t pid;
    struct timeval time;
    char pName[100];
} MessageStart;

typedef struct mf
{
    pid_t pid;
    struct timeval time;
} MessageFinish;

typedef enum st
{
    TIME,
    COMMAND,
    UNIQUE,
} StatType;

typedef struct mstattype
{
    pid_t ogPid;
    StatType type;
    char pids[50][100];
    int nPid;
    char pName[100];
} MessageStats;

typedef struct mstatr
{
    long num;
    char names[100][100];
} MessageStatsResponse;
#endif