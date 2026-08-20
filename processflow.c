#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>

#define MAX_TASKS 100
#define MAX_JOBS  100
#define MAX_ARGS  20
#define MAX_LINE  1024

typedef struct {
    char nome[50];
    char programa[100];
    char argumentos[MAX_ARGS][100];
    int  quant_args;
} Task;

typedef struct {
    int id;
    pid_t pid;
    char nome_task[50];
    int ativo;
} Job;

Task tarefas[MAX_TASKS];
Job jobs[MAX_JOBS];
int quant_tasks = 0;
int quant_jobs = 0;
int prox_job = 1;

volatile sig_atomic_t chegou_sigchld = 0;

int procurar_task(char *nome) {
    for (int i = 0; i < quant_tasks; i++) {
        if (strcmp(tarefas[i].nome, nome) == 0) {
            return i;
        }
    }

    return -1;
}

int separar_argumentos(char *linha, char *args[]) {
    int n = 0;
    char *palavra = strtok(linha, " \t\n");

    while (palavra != NULL && n < MAX_ARGS - 1) {
        args[n++] = palavra;
        palavra = strtok(NULL, " \t\n");
    }

    args[n] = NULL;
    return n;
}


int main(void) {
    char linha[MAX_LINE];
    char *args[MAX_ARGS];

    printf("ProcessFlow parser iniciado.\n");

    while (fgets(linha, sizeof(linha), stdin) != NULL) {
        int n = separar_argumentos(linha, args);

        if (n == 0) {
            continue;
        }

        printf("tokens=%d primeiro=%s\n", n, args[0]);

        if (strcmp(args[0], "exit") == 0) {
            break;
        }
    }

    return 0;
}
