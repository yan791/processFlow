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

void cadastrar_task(char *args[], int n) {
    if (n < 3) {
        printf("Uso: task nome programa [argumentos]\n");
        return;
    }

    if (quant_tasks >= MAX_TASKS) {
        printf("Limite de tarefas atingido.\n");
        return;
    }

    if (n - 2 > MAX_ARGS - 1) {
        printf("Numero de argumentos excede o limite (%d).\n", MAX_ARGS - 1);
        return;
    }

    Task *t = &tarefas[quant_tasks];

    strncpy(t->nome, args[1], sizeof(t->nome) - 1);
    t->nome[sizeof(t->nome) - 1] = '\0';

    strncpy(t->programa, args[2], sizeof(t->programa) - 1);
    t->programa[sizeof(t->programa) - 1] = '\0';

    t->quant_args = n - 2;

    for (int i = 2; i < n; i++) {
        strncpy(t->argumentos[i - 2], args[i],
                sizeof(t->argumentos[i - 2]) - 1);
        t->argumentos[i - 2][sizeof(t->argumentos[i - 2]) - 1] = '\0';
    }

    quant_tasks++;
    printf("Tarefa '%s' cadastrada.\n", t->nome);
}

void montar_exec_args(Task *task, char *args[]) {
    args[0] = task->programa;

    for (int i = 0; i < task->quant_args; i++) {    
        args[i + 1] = task->argumentos[i];
    }

    args[task->quant_args + 1] = NULL;
}


int main(void) {
    char linha[MAX_LINE];
    char *args[MAX_ARGS];

    printf("ProcessFlow cadastro iniciado.\n");

    while (fgets(linha, sizeof(linha), stdin) != NULL) {
        int n = separar_argumentos(linha, args);

        if (n == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "task") == 0) {
            cadastrar_task(args, n);
        } else {
            printf("comando nao reconhecido.\n");
        }
    }

    return 0;
}

