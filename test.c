// remove as necessary idk
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <limits.h>

#define FORK_ERROR     101
#define PIPE_ERROR     102
#define MALLOC_ERROR   103
#define CHILD_ENDS     104
#define HANDLER_ERROR  105
#define SELECT_ERROR   106



typedef struct {
    pid_t pid;
    int self;
    int p2c[2];  // parent to child
    int c2p[2];  // child to parent
    int state;   // 0|1 for idle|busy
} child;

typedef struct {
    /*
    arg1 could be the type of operation {addition, subtraction, multiplication, division}
    args 2&3 could be the parameters
    arg0 could be for messaging, or maybe for results (child sends results)
    none of this matters much, given how pointless this is
    */
    int cur_length;
    char message[70];
    int arg0;
    int arg1;
    int arg2;
    int arg3;
} msg;

child ** array_of_death;
int total;
int pos;
int global_secs;

void parent_code(msg* parent_message, int N, child* children[]);
void child_code(msg* child_message, child* self);
int random_int(int a, int b);

void childSigHandler(int sig);
void parentSigHandler(int sig);



int main (int argc, char *argv[]){
    // terminal arguments
    //char* file_name = argv[1];
    //int N = atoi(argv[2]);
    int N = 10;

    // other important stuff
    int pid;

    // store info on children
    child* v_children[N];
    child self;
    msg parent_message;
    msg child_message;

    array_of_death = v_children;
    total = N;
    

    // create children and pipes
    for (int i=0; i<N; i++){
        v_children[i] = (child*)malloc(sizeof(child)); // look up execv // besser als diese loesung
        // send to child
        if (pipe(v_children[i]->p2c)<0) exit(PIPE_ERROR);
        // receive from child
        if (pipe(v_children[i]->c2p)<0) exit(PIPE_ERROR);
        
        // child inherits these and therefore only knows about itself
        self.self = i;
        
        // i hate the way this is done
        self.p2c[0] = v_children[i]->p2c[0];
        self.p2c[1] = v_children[i]->p2c[1];
        self.c2p[0] = v_children[i]->c2p[0];
        self.c2p[1] = v_children[i]->c2p[1];

        pid = fork();
        if (pid==0){
            pos = i;
            break;
        }
        if (pid<0) exit(FORK_ERROR);  // error while forking

        // stop child
        //kill(pid, SIGSTOP);

        // close pipes
        v_children[i]->pid = pid;
        v_children[i]->self = i;
        // close read end
        close(v_children[i]->p2c[0]);
        // close write end
        close(v_children[i]->c2p[1]);
    }


    // separate code for parent and children
    if (pid>0){
        // parent code
        parent_code(&parent_message, N, v_children);
    } else if (pid==0){
        // child code
        // fix pipes before doing anything else
        close(self.p2c[1]);
        close(self.c2p[0]);

        child_code(&child_message, &self);
    } else {
        exit(FORK_ERROR);
    }
}

/*
    1. parent is already running on a server
    2. parent receives an rpc (call an existing func?)
    3. parent receives the number of seconds to sleep
    4. parent forwards this to the children
    5. children sleep
    6. parent receives feedback and returns it to RPCer
    7. it's prob good to use some list or array ose dicka tjeter per te mbajtur punet e prinderit
*/



void parent_code(msg* parent_message, int N, child* children[]){
    signal(SIGINT, (void(*)(int))parentSigHandler);
    int pid = getpid();
    
    fd_set current_sockets, ready_sockets;
    int maxfd =-1;
    msg responses[N];
    
    // set fd sets
    FD_ZERO(&current_sockets);
    for (int i=0; i<N; i++){
        FD_SET(children[i]->c2p[0], &current_sockets);
        if (children[i]->c2p[0] > maxfd) maxfd = children[i]->c2p[0];
    }

    // busy loop
    while (true){
        // father sends one message
        for (int i=0; i<N; i++){
            if (children[i]->state==1) continue;

            sprintf(parent_message->message, "hatarake\0");
            parent_message->arg0 = 12;  // this means keep going
            parent_message->arg1 = random_int(1, 5);
            write(children[i]->p2c[1], parent_message, sizeof(msg));
            
            children[i]->state = 1;
        }

        ready_sockets = current_sockets;

        if(select(maxfd+1, &ready_sockets, NULL, NULL, NULL)<0){
            // error happens here // who cares abt timeout
            exit(SELECT_ERROR);
        }
        for (int i=0; i<N; i++){
            if (FD_ISSET(children[i]->c2p[0], &ready_sockets)){
                // there's a message ready
                read(children[i]->c2p[0], &responses[i], sizeof(msg));
                children[i]->state = 0;
            }
        }
    }
}

void child_code(msg* child_message, child* self){
    signal(SIGINT, (void (*)(int))childSigHandler);
    self->pid = getpid();
    int keep_going = 1;
    while (keep_going){
        do {
            // keep reading from pipe, eventually something will be written
            read(self->p2c[0], child_message, sizeof(msg));
        } while(child_message->message[0]==EOF);
        keep_going = child_message->arg0;  // if 1, keep going. else, terminate
        if (keep_going==0) break;

        // child message should now have something in it.
        // do something about it
        // arg1 will be the amount of seconds to sleep
        printf("[CHILD:%d] -> sleeping\n", self->pid);
        sleep(child_message->arg1);
        printf("[CHILD:%d] -> awake\n", self->pid);


        // now that you're done, reply to the parent
        sprintf(child_message->message, "I have risen, father\0");
        write(self->c2p[1], child_message, sizeof(msg));
    }

    if (keep_going==0){
        printf("ordered by parent to stop\n");
    }

    exit(CHILD_ENDS);
}


// extra functions
int random_int(int a, int b) {
    // Seed the random number generator with the current time
    srand(time(NULL));

    // Generate a random number in the range [a, b]
    int randomInt = rand() % (b - a + 1) + a;

    return randomInt;
}
void childSigHandler(int signal){
    printf("Father, why hast thou forsaken me?\n");
    close(array_of_death[pos]->c2p[1]);
    close(array_of_death[pos]->p2c[0]);
    // child must free the array_of_death too?
    // pastro GJITHCKA.
    // edhe nga ana a fmis, edhe nga ana e prindrit
    // te gjithe kan kopje
    exit(CHILD_ENDS);
}
void parentSigHandler(int signal){
    for (int i=0; i<total; i++){
        kill(array_of_death[i]->pid, SIGINT);
        close(array_of_death[pos]->c2p[0]);
        close(array_of_death[pos]->p2c[1]);
        int status;
        waitpid(array_of_death[i]->pid, &status, 0);
        free(array_of_death[i]);  // free each child one by one
    }
    printf("This is the end.\n");
    exit(0);
}