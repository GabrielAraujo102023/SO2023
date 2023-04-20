#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>
#include "messages.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "LinkedList.h"
#include <string.h>

int pidFifo(pid_t pid, int mode)
{
    char *sPid = malloc(15);
    sprintf(sPid, "./tmp/%d", pid);
    int fifo = open(sPid, mode, 0666);
    free(sPid);
    return fifo;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Insira path para pasta onde serão guardadas as informações sobre programas executados\n");
        return 0;
    }
    mkfifo("./tmp/start", 0666);
    mkfifo("./tmp/status", 0666);
    mkfifo("./tmp/stats", 0666);
    int status;
    if (fork() == 0) // EXECUTE
    {
        int fifoStart = open("./tmp/start", O_RDONLY, 0666);
        MessageStart msgStart;
        LinkedList *messages = createLL();
        while (1)
        {
            if (read(fifoStart, &msgStart, sizeof(MessageStart)) > 0)
            {
                addLL(messages, msgStart);
                if (msgStart.pid > 0 && fork() == 0)
                {
                    // Lida com a termino de um processo do tracer
                    MessageFinish msgFinish;
                    char *sPid = malloc(15);
                    sprintf(sPid, "./tmp/%d", msgStart.pid);
                    mkfifo(sPid, 0666);
                    int fifoFinish = open(sPid, O_RDONLY, 0666);
                    read(fifoFinish, &msgFinish, sizeof(MessageFinish));
                    close(fifoFinish);
                    removeLL(messages, msgStart.pid);

                    // Guarda informação sobre termino de processo
                    char *path = malloc(512);
                    char *aux = malloc(512);
                    strcpy(path, argv[1]);
                    strcat(path, "/");
                    sprintf(aux, "%d", msgStart.pid);
                    strcat(path, aux);
                    int file = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                    char *input = malloc(1024);
                    strcpy(input, msgStart.pName);
                    strcat(input, ";");
                    sprintf(aux, "%ld", (msgFinish.time.tv_sec - msgStart.time.tv_sec) * 1000 + (msgFinish.time.tv_usec - msgStart.time.tv_usec) / 1000);
                    strcat(input, aux);
                    strcat(input, ";");
                    write(file, input, strlen(input));
                    free(path);
                    free(input);
                    free(aux);
                    _exit(getpid());
                }
                else if (fork() == 0)
                {
                    removeLL(messages, -1);
                    int fifoStatus = open("./tmp/status", O_RDONLY, 0666);
                    pid_t pid;

                    if (read(fifoStatus, &pid, sizeof(pid_t)) > 0 && fork() == 0)
                    {
                        int fifo = pidFifo(pid, O_WRONLY);
                        int size = getLLSize(messages);
                        MessageStart it;
                        for (int i = 0; i < size; i++)
                        {
                            it = iterateLL(messages);
                            write(fifo, &it, sizeof(MessageStart));
                        }
                        MessageStart stop = {-1, {0, 0}, {' '}};
                        write(fifo, &stop, sizeof(MessageStart));
                        _exit(getpid());
                    }
                }
            }
        }
    }
    if (fork() == 0) // STATS
    {
        int fifoStats = open("./tmp/stats", O_RDONLY, 0666);
        MessageStats msgStats;
        while (1)
        {
            if (read(fifoStats, &msgStats, sizeof(MessageStats)) > 0 && fork() == 0)
            {
                int *files = (int *)malloc(msgStats.nPid * sizeof(int));
                // Coleta os file descriptors de todos os PIDs
                for (int i = 0; i < msgStats.nPid; i++)
                {
                    char *aux = malloc(20);
                    strcpy(aux, "./");
                    strcat(aux, argv[1]);
                    strcat(aux, "/");
                    strcat(aux, msgStats.pids[i]);
                    files[i] = open(aux, O_RDONLY, 0666);
                    free(aux);
                }
                char *line = malloc(1024);
                char *sep;
                char *aux;
                long count = 0;
                char names[100][100];
                switch (msgStats.type)
                {
                case TIME:
                    for (int i = 0; i < msgStats.nPid; i++)
                    {
                        read(files[i], line, 1024);
                        sep = strdup(line);
                        while (strcmp(strsep(&sep, ";"), "") != 0)
                        {
                            count += atoi(strsep(&sep, ";"));
                        }
                        free(sep);
                        free(line);
                        line = malloc(1024);
                    }
                    break;

                case COMMAND:
                    for (int i = 1; i < msgStats.nPid; i++)
                    {
                        read(files[i], line, 1024);
                        sep = strdup(line);
                        aux = strdup(strsep(&sep, ";"));
                        while (strcmp(aux, "") != 0 && strcmp(aux, msgStats.pName) == 0)
                        {
                            count++;
                            free(aux);
                            aux = strdup(strsep(&sep, ";")); // Passa o tempo no ficheiro à frente
                        }
                        free(line);
                        line = malloc(1024);
                    }
                    break;

                case UNIQUE:
                    int maxSize = 10;
                    int count = 0;
                    char **arr = (char **)malloc(sizeof(char *) * maxSize);
                    for (int i = 0; i < msgStats.nPid; i++)
                    {
                        read(files[i], line, 1024);
                        sep = strdup(line);
                        aux = strdup(strsep(&sep, ";"));
                        while (strcmp(aux, "") != 0)
                        {
                            bool isIn = false;
                            for (int j = 0; j < count; j++)
                            {
                                if (strcmp(aux, arr[j]) == 0)
                                {
                                    isIn = true;
                                    break;
                                }
                            }
                            if (!isIn)
                            {
                                if (count == maxSize)
                                {
                                    maxSize *= 2;
                                    arr = realloc(arr, maxSize * 100);
                                }
                                arr[count] = malloc(100);
                                strcpy(arr[count++], aux);
                            }
                            strsep(&sep, ";");
                            free(aux);
                            aux = strdup(strsep(&sep, ";"));
                        }
                        free(aux);
                        free(line);
                        line = malloc(1024);
                        free(sep);
                    }
                    if (count > 99)
                        count = 99;
                    int i;
                    for (i = 0; i < count; i++)
                    {
                        strcpy(names[i], arr[i]);
                    }
                    strcpy(names[++i], "-1");
                    break;
                }
                MessageStatsResponse res;
                res.num = count;
                memcpy(res.names, names, 100 * 100);
                int fifoPid = pidFifo(msgStats.ogPid, O_WRONLY);
                write(fifoPid, &res, sizeof(MessageStatsResponse));
                close(fifoPid);
                _exit(getpid());
            }
        }
    }
    wait(&status);
}