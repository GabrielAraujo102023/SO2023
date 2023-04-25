#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include "messages.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

long calculateTime(struct timeval t1, struct timeval t2)
{
    return (t1.tv_sec - t2.tv_sec) * 1000 + (t1.tv_usec - t2.tv_usec) / 1000;
}

char *trimwhitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

char **getArgs(char *args)
{
    args = trimwhitespace(args);
    char *command = strdup(strsep(&args, " ")); // Nome do programa
    int size = (args == NULL) ? 0 : strlen(args);
    int nSpaces = (size == 0) ? 2 : 3; // Conta o numero de espaços no input, o valor de inicio depende na existencia de mais argumentos
    for (int i = 0; i < size; i++)
        if (args[i] == ' ')
            nSpaces++;
    char **r = (char **)malloc(sizeof(char *) * nSpaces); // Argumentos do programa
    r[0] = strdup(command);
    for (int i = 1; i < nSpaces - 1; i++)
        r[i] = strdup(strsep(&args, " "));
    free(command);
    free(args);
    return r;
}

void waitAndTellServer(pid_t pid, struct timeval start)
{
    struct timeval finish;
    int status;
    char *sPid = malloc(15);
    sprintf(sPid, "./tmp/%d", pid);
    // Espera for fim do processo e envio de mensagem de fim para servidor
    printf("Running PID %d\n", pid);
    fflush(stdout);
    waitpid(pid, &status, 0);
    gettimeofday(&finish, NULL);
    printf("Ended in %ld ms\n", calculateTime(finish, start));
    MessageFinish msg = {pid, finish};
    int fifoFinish = open(sPid, O_WRONLY, 0666);
    write(fifoFinish, &msg, sizeof(MessageFinish));
    free(sPid);
    close(fifoFinish);
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("Argumentos não encontrados, insira um modo de utilização (execute, status, stats-time, stats-command, stats-uniq)");
        return 0;
    }
    int status;
    if (strcmp(argv[1], "execute") == 0 && argc >= 3)
    {
        int fifoStart = open("./tmp/start", O_WRONLY, 0666);
        pid_t pid;
        struct timeval start;
        gettimeofday(&start, NULL);
        if (strcmp(argv[2], "-u") == 0) // EXECUCAO DE PROGRAMA UNICO
        {
            if ((pid = fork()) == 0)
            {
                // Parsing dos argumentos
                char *input = strdup(argv[3]);
                char **args = getArgs(input);

                // Envio de mensagem de inicio para servidor e execução do programa
                char commandArr[100];
                strcpy(commandArr, args[0]);
                MessageStart msg = {getpid(), start};
                strcpy(msg.pName, args[0]);
                write(fifoStart, &msg, sizeof(MessageStart));
                close(fifoStart);
                execvp(args[0], args);
                perror(args[0]);
                _exit(pid);
            }
            else
            {
                waitAndTellServer(pid, start);
            }
        }
        else if (strcmp(argv[2], "-p") == 0) // EXECUCAO DE PIPELINE
        {
            int nCommands = 1;
            int len = strlen(argv[3]);
            for (int i = 0; i < len; i++)
            {
                if (argv[3][i] == '|')
                    nCommands++;
            }
            char *sep = strdup(argv[3]);
            char names[100];
            strcpy(names, strsep(&sep, " "));
            for (int i = 1; i < nCommands; i++)
            {
                strcat(names, ";");
                strsep(&sep, "|");
                sep = trimwhitespace(sep);
                strcat(names, strsep(&sep, " "));
            }
            if ((pid = fork()) == 0)
            {
                // Envio de mensagem de inicio para servidor e execução do programa
                MessageStart msg = {getpid(), start};
                strcpy(msg.pName, names);
                write(fifoStart, &msg, sizeof(MessageStart));
                close(fifoStart);
                char *sep = strdup(argv[3]);
                int i = 1;
                int nPipes = (nCommands - 1);
                int pipes[nPipes * 2];
                for (int j = 0; j < nPipes; j++)
                {
                    if (pipe(pipes + j * 2) < 0)
                    {
                        perror("PIPE");
                        return 1;
                    }
                }
                int pipeAux = 0;
                while (i <= nCommands)
                {
                    char **args = getArgs(strsep(&sep, "|"));
                    if ((pid = fork()) == 0)
                    {                      
                        if (i != 1) // Não realiza isto no primeiro
                        {
                            if (dup2(pipes[(pipeAux - 1) * 2], STDIN_FILENO) < 0)
                            {
                                perror("DUP2");
                                return 0;
                            }
                        }
                        if (i != nCommands) // Não realiza isto no último
                        {
                            if (dup2(pipes[pipeAux * 2 + 1], STDOUT_FILENO) < 0)
                            {
                                perror("DUP2");
                                return 0;
                            }
                        }
                        for (int j = 0; j < nPipes * 2; j++)
                        {
                            close(pipes[j]);
                        }
                        execvp(args[0], args);
                        perror(args[0]);
                        _exit(pid);
                    }
                    if(i == nCommands) wait(NULL);
                    pipeAux++;
                    i++;
                }
                for (i = 0; i < 2 * nPipes; i++)
                {
                    close(pipes[i]);
                }
                _exit(pid);
            }
            else
            {
                waitAndTellServer(pid, start);
            }
        }
    }
    else if (strcmp(argv[1], "status") == 0 && argc == 2)
    {
        if (fork() == 0)
        {
            int fifoStart = open("./tmp/start", O_WRONLY, 0666);
            MessageStart msgs = {-1, {0, 0}, {" "}}; // Avisa o servidor que vai ser feito um pedido de status
            write(fifoStart, &msgs, sizeof(MessageStart));

            pid_t pid = getpid();
            char *sPid = malloc(15);
            sprintf(sPid, "./tmp/%d", pid);
            mkfifo(sPid, 0666);
            int fifoStatus = open("./tmp/status", O_WRONLY, 0666);
            write(fifoStatus, &pid, sizeof(pid_t));
            close(fifoStatus);
            struct timeval time;
            MessageStart msgStatus;
            int fifoPid = open(sPid, O_RDONLY, 0666);
            read(fifoPid, &msgStatus, sizeof(MessageStart));
            gettimeofday(&time, NULL);
            while (msgStatus.pid != -1)
            {
                printf("%d %s %ld ms\n", msgStatus.pid, msgStatus.pName, calculateTime(time, msgStatus.time));
                read(fifoPid, &msgStatus, sizeof(MessageStart));
                gettimeofday(&time, NULL);
            }
            free(sPid);
            _exit(pid);
        }
    }
    else if ((strcmp(argv[1], "stats-time") == 0 || strcmp(argv[1], "stats-command") == 0 || strcmp(argv[1], "stats-uniq") == 0) && argc == 3)
    {
        if (fork() == 0)
        {
            MessageStats msgStats;
            msgStats.type = argv[1][6] == 't' ? TIME : argv[1][6] == 'c' ? COMMAND
                                                                         : UNIQUE;
            pid_t pid = getpid();
            char *sPid = malloc(15);
            sprintf(sPid, "./tmp/%d", pid);
            mkfifo(sPid, 0666);
            char *input = strdup(argv[2]);
            char *command = NULL;
            trimwhitespace(input);
            int size = strlen(input);
            int nSpaces = msgStats.type == COMMAND ? 0 : 1;
            for (int i = 0; i < size; i++)
                if (input[i] == ' ')
                    nSpaces++;
            char args[50][100]; // Argumentos do programa
            if (msgStats.type == COMMAND)
            {
                command = strdup(strsep(&input, " "));
                strcpy(msgStats.pName, command);
            }
            for (int i = 0; i < nSpaces; i++)
                strcpy(args[i], strsep(&input, " "));
            msgStats.ogPid = pid;
            msgStats.nPid = nSpaces;
            memcpy(msgStats.pids, args, 50 * 100);
            int fifoStats = open("./tmp/stats", O_WRONLY, 0666);
            write(fifoStats, &msgStats, sizeof(MessageStats));
            close(fifoStats);
            MessageStatsResponse res;
            int fifoPid = open(sPid, O_RDONLY, 0666);
            read(fifoPid, &res, sizeof(MessageStatsResponse));
            switch (msgStats.type)
            {
            case TIME:
                printf("Total execution time is %ld\n", res.num);
                break;
            case COMMAND:
                printf("%s was executed %ld times\n", command, res.num);
                break;
            case UNIQUE:
                int i = 0;
                while (i < 100 && strcmp(res.names[i], "-1") != 0)
                {
                    printf("%s\n", res.names[i++]);
                }
                break;
            }
            close(fifoStats);
            free(sPid);
        }
    }
    else
    {
        printf("Comando não reconhecido ou falta de argumentos.");
        return 0;
    }
    wait(&status);
}
