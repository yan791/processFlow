#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
void executar_parallel(Task **tasks, int n);

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

    if (n - 3 > MAX_ARGS - 2) {
        printf("Numero de argumentos excede o limite (%d).\n", MAX_ARGS - 1);
        return;
    }

    Task *t = &tarefas[quant_tasks];

    strncpy(t->nome, args[1], sizeof(t->nome) - 1);
    t->nome[sizeof(t->nome) - 1] = '\0';

    strncpy(t->programa, args[2], sizeof(t->programa) - 1);
    t->programa[sizeof(t->programa) - 1] = '\0';

    t->quant_args = n - 3;

    for (int i = 3; i < n; i++) {
    strncpy(t->argumentos[i - 3], args[i],
            sizeof(t->argumentos[i - 3]) - 1);
    t->argumentos[i - 3][sizeof(t->argumentos[i - 3]) - 1] = '\0';
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

pid_t spawn(Task *task) {
    char *args[MAX_ARGS];
    montar_exec_args(task, args);

    pid_t pid = fork();

    if (pid == -1) {
        perror("Erro no fork");
        return -1;
    }

    if (pid == 0) {
        execvp(task->programa, args);
        perror("Erro ao executar tarefa");
        exit(127);
    }

    return pid;
}

void executar_task(Task *task) {
    pid_t pid = spawn(task);

    if (pid <= 0) {
        return;
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        printf("Tarefa '%s' terminou com codigo %d.\n",
               task->nome, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Tarefa '%s' terminou pelo sinal %d.\n",
               task->nome, WTERMSIG(status));
    }
}

void comando_run(char *args[], int n) {
    if (n < 2) {
        printf("Uso: run tarefa\n");
        return;
    }

    if (strcmp(args[1], "sequential") == 0) {
        for (int i = 2; i < n; i++) {
            int idx = procurar_task(args[i]);

            if (idx == -1) {
                printf("Tarefa '%s' nao existe.\n", args[i]);
            } else {
                executar_task(&tarefas[idx]);
            }
        }

        return;
    }

    if (strcmp(args[1], "parallel") == 0) {
        Task *lista[MAX_ARGS];
        int total = 0;

        for (int i = 2; i < n; i++) {
            int idx = procurar_task(args[i]);

            if (idx == -1) {
                printf("Tarefa '%s' nao existe.\n", args[i]);
            } else {
                lista[total++] = &tarefas[idx];
            }
        }

        if (total > 0) {
            executar_parallel(lista, total);
        }

        return;
    }

    int idx = procurar_task(args[1]);

    if (idx == -1) {
        printf("Tarefa '%s' nao existe.\n", args[1]);
    } else {
        executar_task(&tarefas[idx]);
    }
}


void executar_parallel(Task **tasks, int n) {
    pid_t pids[MAX_ARGS];
    char *nomes[MAX_ARGS];
    int total = 0;

    for (int i = 0; i < n; i++) {
        pid_t pid = spawn(tasks[i]);

        if (pid > 0) {
            pids[total] = pid;
            nomes[total] = tasks[i]->nome;
            total++;
        }
    }

    for (int i = 0; i < total; i++) {
        int status;
        waitpid(pids[i], &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            printf("Tarefa '%s' terminou com codigo %d.\n",
                   nomes[i], WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Tarefa '%s' terminou pelo sinal %d.\n",
                   nomes[i], WTERMSIG(status));
        }
    }
}

void registrar_job(pid_t pid, char *nome_task) {
    if (quant_jobs >= MAX_JOBS) {
        printf("Limite de jobs atingido.\n");
        return;
    }

    Job *j = &jobs[quant_jobs];
    j->id = prox_job;
    j->pid = pid;
    j->ativo = 1;

    strncpy(j->nome_task, nome_task, sizeof(j->nome_task) - 1);
    j->nome_task[sizeof(j->nome_task) - 1] = '\0';

    printf("[%d] %d\n", prox_job, pid);

    quant_jobs++;
    prox_job++;
}

void iniciar_task(Task *task) {
    pid_t pid = spawn(task);

    if (pid > 0) {
        registrar_job(pid, task->nome);
    }
}
void mostrar_jobs(void) {
    for (int i = 0; i < quant_jobs; i++) {
        if (jobs[i].ativo) {
            printf("[%d] %d Running %s\n",jobs[i].id,jobs[i].pid,jobs[i].nome_task);
        } 
        else {
            printf("[%d] Finalizado %s\n",
                   jobs[i].id,
                   jobs[i].nome_task);
        }
    }
}

void cmd_start(char *args[], int n) {
    if (n < 2) {
        printf("uso: start tarefa\n");
        return;
    }

    int idx = procurar_task(args[1]);

    if (idx == -1) {
        printf("tarefa nao existe.\n");
    } 
    else {
        iniciar_task(&tarefas[idx]);
    }
}

int main(void) {
    char *args[] = {"start", "teste"};
    int n = 2;

    strcpy(tarefas[0].nome, "teste");
    strcpy(tarefas[0].programa, "echo");
    strcpy(tarefas[0].argumentos[0], "Teste do cmd_start");
    tarefas[0].quant_args = 1;

    quant_tasks = 1;

    printf("Teste 3 - start com tarefa valida:\n");

    cmd_start(args, n);

    printf("quant_jobs = %d\n", quant_jobs);

    return 0;
}
