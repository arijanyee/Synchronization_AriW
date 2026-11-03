/*
 * Outside Assistance:
 * ChatGPT (OpenAI) was used as a supplementary resource
 * to assist debugging and troubleshooting the implementation.
 *
 * All code was reviewed and tested by me.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>

#define DAD 1
#define MOM 2

typedef struct {
    int BankAccount;
    sem_t mutex;
} SharedData;

/* Lock the shared bank account */
void lockBank(SharedData *shared) {
    while (sem_wait(&shared->mutex) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }
    }
}

/* Unlock the shared bank account */
void unlockBank(SharedData *shared) {
    if (sem_post(&shared->mutex) == -1) {
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
}

/* Dear Old Dad process */
void dearOldDad(SharedData *shared) {
    int localBalance = 0;

    srand(time(NULL) ^ getpid());

    lockBank(shared);
    localBalance = shared->BankAccount;
    unlockBank(shared);

    while (1) {
        sleep(rand() % 6);
        lockBank(shared);
        localBalance = shared->BankAccount;
        unlockBank(shared);
        
        printf("Dear Old Dad: Attempting to Check Balance\n");

        int randomNumber = rand() % 2;

        if (randomNumber % 2 == 0) {

            if (localBalance < 100) {

                lockBank(shared);

                localBalance = shared->BankAccount;

                int amount = rand() % 100;
                int depositChoice = rand() % 2;

                if (depositChoice % 2 == 0) {
                    localBalance += amount;

                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n",
                           amount, localBalance);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }

                shared->BankAccount = localBalance;

                unlockBank(shared);

            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n",
                       localBalance);
            }

        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n",
                   localBalance);
        }

        fflush(stdout);
    }
}

/* Lovable Mom process - Extra Credit */
void lovableMom(SharedData *shared) {
    int localBalance = 0;

    srand(time(NULL) ^ getpid());

    lockBank(shared);
    localBalance = shared->BankAccount;
    unlockBank(shared);

    while (1) {
        sleep(rand() % 11);

        lockBank(shared);
        localBalance = shared->BankAccount;
        unlockBank(shared);

printf("Loveable Mom: Attempting to Check Balance\n");

        if (localBalance <= 100) {

            lockBank(shared);

            localBalance = shared->BankAccount;

            int amount = rand() % 125;

            localBalance += amount;

            printf("Lovable Mom: Deposits $%d / Balance = $%d\n",
                   amount, localBalance);

            shared->BankAccount = localBalance;

            unlockBank(shared);
        }

        fflush(stdout);
    }
}

/* Poor Student process */
void poorStudent(SharedData *shared) {
    int localBalance = 0;

    srand(time(NULL) ^ getpid());

    lockBank(shared);
    localBalance = shared->BankAccount;
    unlockBank(shared);

    while (1) {
        sleep(rand() % 6);
        
        lockBank(shared);
        localBalance = shared->BankAccount;
        unlockBank(shared);

        printf("Poor Student: Attempting to Check Balance\n");

        int randomNumber = rand() % 2;

        if (randomNumber % 2 == 0) {

            lockBank(shared);

            localBalance = shared->BankAccount;

            int need = rand() % 50;

            printf("Poor Student needs $%d\n", need);

            if (need <= localBalance) {

                localBalance -= need;

                printf("Poor Student: Withdraws $%d / Balance = $%d\n",
                       need, localBalance);

            } else {

                printf("Poor Student: Not Enough Cash ($%d)\n",
                       localBalance);
            }

            shared->BankAccount = localBalance;

            unlockBank(shared);

        } else {

            printf("Poor Student: Last Checking Balance = $%d\n",
                   localBalance);
        }

        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {

    int numberOfParents = 1;
    int numberOfStudents = 1;

    /* Read command-line arguments */
    if (argc == 3) {
        numberOfParents = atoi(argv[1]);
        numberOfStudents = atoi(argv[2]);
    }

    if (numberOfParents != 1 && numberOfParents != 2) {
        printf("Usage: %s [1 or 2] [number of students]\n", argv[0]);
        return 1;
    }

    if (numberOfStudents < 1) {
        printf("You must have at least one student.\n");
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    /* Create shared memory */
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedData),
                       IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    /* Attach shared memory */
    SharedData *shared = (SharedData *)shmat(shmid, NULL, 0);

    if (shared == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    /* Initialize the shared bank account */
    shared->BankAccount = 0;

    /* Initialize a process-shared semaphore */
    if (sem_init(&shared->mutex, 1, 1) == -1) {
        perror("sem_init");
        shmdt(shared);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    /* Create Lovable Mom if requested */
    if (numberOfParents == 2) {

        pid_t momPID = fork();

        if (momPID == -1) {
            perror("fork");
            return 1;
        }

        if (momPID == 0) {
            lovableMom(shared);
            exit(0);
        }
    }

    /* Create the Poor Student processes */
    for (int i = 0; i < numberOfStudents; i++) {

        pid_t childPID = fork();

        if (childPID == -1) {
            perror("fork");
            return 1;
        }

        if (childPID == 0) {
            poorStudent(shared);
            exit(0);
        }
    }

    /* Original parent process becomes Dear Old Dad */
    dearOldDad(shared);

    /* These statements are reached if Dad's loop ends */
    sem_destroy(&shared->mutex);
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
