#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>


//MMAP functions for shared variables
#define MMAP(pointer){(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

//Variables where I saved arguments
int o_counter, h_counter, max_wait, max_create;

//File where I saved output
FILE *file;

//Semaphores
sem_t *mutex = NULL;
sem_t *o_end_sem = NULL;
sem_t *h_end_sem = NULL;

//Semaphore for printing output
sem_t *output = NULL;

//barier semaphores
sem_t *mutex2 = NULL;
sem_t *turnstile = NULL;
sem_t *turnstile2 = NULL;

//molecule creating semaphores
sem_t *oxygen_molecule_end = NULL;
sem_t *hydrogen_molecule_end = NULL;
sem_t *oxygen_semiprint = NULL;
sem_t *hydrogen_semiprint = NULL;

//Shared variables
int *line= NULL;
int *ox_count_sh = NULL;
int *h_count_sh = NULL;
int *actual_ox = NULL;
int *actual_h = NULL;
int *mol_count = NULL;
int *count = NULL;
bool *is_last = NULL;

//Function made for sleep
void mysleep(int time){
    if(time != 0){
        usleep((rand()%(time+1))*1000);
    }
}

//Function to close semaphores and destroy shared variables
void dealokacia() {
    sem_close(mutex);
    sem_unlink("/xsnope04_mutex");
    sem_close(output);
    sem_unlink("/xsnope04_output");
    sem_close(o_end_sem);
    sem_unlink("/xsnope04_hend");
    sem_close(h_end_sem);
    sem_unlink("/xsnope04_oend");
    sem_close(mutex2);
    sem_unlink("/xsnope04_mutex2");
    sem_close(turnstile2);
    sem_unlink("/xsnope04_turnstile2");
    sem_close(turnstile);
    sem_unlink("/xsnope04_turnstile");
    sem_close(hydrogen_molecule_end);
    sem_unlink("/xsnope04_h_molecule_end");
    sem_close(oxygen_molecule_end);
    sem_unlink("/xsnope04_o_molecule_end");
    sem_close(oxygen_semiprint);
    sem_unlink("/xsnope04_oxy_semiprint");
    sem_close(hydrogen_semiprint);
    sem_unlink("/xsnope04_hydro_semiprint");


    UNMAP(line);
    UNMAP(ox_count_sh);
    UNMAP(h_count_sh);
    UNMAP(actual_ox);
    UNMAP(actual_h);
    UNMAP(mol_count);
    UNMAP(count);
    UNMAP(is_last);
}


//Function to start the file, start and initialize sempaphores and shared variables
void alokacia(){
    file = fopen("proj2.out", "a");
    setbuf(file, NULL);

    if((mutex = sem_open("/xsnope04_mutex", O_CREAT|O_EXCL, 0666, 1))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: mutex");
        dealokacia();
        exit(1);
    }

    if((output = sem_open("/xsnope04_output", O_CREAT|O_EXCL, 0666, 1))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: output");
        dealokacia();
        exit(1);
    }

    if((h_end_sem = sem_open("/xsnope04_hend", O_CREAT|O_EXCL, 0666, 0))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: hend");
        dealokacia();
        exit(1);
    }

    if((o_end_sem = sem_open("/xsnope04_oend", O_CREAT|O_EXCL, 0666, 0))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: oend");
        dealokacia();
        exit(1);
    }

    if((mutex2 = sem_open("/xsnope04_mutex2", O_CREAT|O_EXCL, 0666, 1))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: mutex2");
        dealokacia();
        exit(1);
    }
    if((turnstile = sem_open("/xsnope04_turnstile", O_CREAT|O_EXCL, 0666, 0))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: turnstile");
        dealokacia();
        exit(1);
    }
    if((turnstile2 = sem_open("/xsnope04_turnstile2", O_CREAT|O_EXCL, 0666, 1))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: turnstile2");
        dealokacia();
        exit(1);
    }

    if((oxygen_molecule_end = sem_open("/xsnope04_o_molecule_end", O_CREAT|O_EXCL, 0666, 2))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: oxygen_molecule_end");
        dealokacia();
        exit(1);
    }

    if((hydrogen_molecule_end = sem_open("/xsnope04_h_molecule_end", O_CREAT|O_EXCL, 0666, 2))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: hydrogen_molecule_end");
        dealokacia();
        exit(1);
    }

    if((hydrogen_semiprint = sem_open("/xsnope04_hydro_semiprint", O_CREAT|O_EXCL, 0666, 0))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: hydro_semiprint");
        dealokacia();
        exit(1);
    }

    if((oxygen_semiprint = sem_open("/xsnope04_oxy_semiprint", O_CREAT|O_EXCL, 0666, 0))== SEM_FAILED){
        fprintf(stderr, "Could not start semaphore: oxy_semiprint");
        dealokacia();
        exit(1);
    }

    MMAP(line);
    MMAP(ox_count_sh);
    MMAP(h_count_sh);
    MMAP(actual_h);
    MMAP(actual_ox);
    MMAP(mol_count);
    MMAP(count);
    MMAP(is_last);
    MMAP(molecule);
}

//Function that I used as barrier in oxygen_create and hydrogen_create
void barrier(){
    sem_wait(mutex2);
    (*count)++;
    if((*count) == 3){
        for(int i=0;i<3;i++){
            sem_post(turnstile);
        }
    }
    sem_post(mutex2);
    sem_wait(turnstile);
    sem_wait(mutex2);
    (*count)--;
    if(*count == 0){
        for (int i = 0; i < 3; ++i) {
            sem_post(turnstile2);
        }
    }
    sem_post(mutex2);
    sem_wait(turnstile2);
}

//Function to create oxygen and molecules
void oxygen_create(int atom_time, int molecule_time,int oid){

    sem_wait(mutex);

    //Starting oxygen
    sem_wait(output);
    fprintf(file, "%d: O %d: started\n", ++(*line), oid);
    fflush(file);
    sem_post(output);

    //To implement randomness
    sem_post(mutex);
    sem_wait(mutex);

    //Sleep for creating atom
    mysleep(atom_time);

    //Sending oxygen to queue
    sem_wait(output);
    fprintf(file, "%d: O %d: going to queue\n", ++(*line), oid);
    fflush(file);
    sem_post(output);

    //Incrementing current oxygens
    (*actual_ox)++;

    //Oxygens left to go to queue
    (*ox_count_sh)--;

    //Checking if there are anymore molecules left to create
    if(*actual_h<2 || *actual_ox<1){
        //Checking the scenario where there are exact numbers to create molecules - no oxygens or hydrogens unused
        if(o_counter * 2 != h_counter){
            //Checking if this is the last atoms
            if(*ox_count_sh == 0 && *h_count_sh == 0){
                //True if this is last atom
                *is_last = true;

                for (int i = 0; i < *actual_ox; ++i) {
                    sem_post(h_end_sem);
                }
                for (int i = 0; i < *actual_h; ++i) {
                    sem_post(o_end_sem);
                }
            }
        }
    }
    //Deciding if there are enough atoms to create molecule
    if(*actual_h >= 2){
        sem_post(o_end_sem);
        sem_post(o_end_sem);
        *actual_h -= 2;
        sem_post(h_end_sem);
        *actual_ox -= 1;
        *is_last = false;
    }
    else{
        sem_post(mutex);
    }

    //Waiting for enough molecules semaphore post
    sem_wait(h_end_sem);

    //This is where program ends if there are no more molecules left to make and there are leftover oxygen
    if(*is_last == true){
        sem_wait(output);
        fprintf(file,"%d: O %d: not enough H\n", ++(*line), oid);
        sem_post(output);
        exit(0);
    }

    //Waiting until previous molecule is created
    sem_wait(hydrogen_molecule_end);
    sem_wait(hydrogen_molecule_end);

    //Variable for molecule index
    int moliny = (*mol_count);

    //Creating molecule
    sem_wait(output);
    fprintf(file,"%d: O %d: creating molecule %d\n", ++(*line), oid, moliny);
    fflush(file);
    sem_post(output);

    //Sleep to create molecule
    mysleep(molecule_time);

    //Semaphores for checking if all atoms are done with creating the molecule
    sem_post(oxygen_semiprint);
    sem_wait(hydrogen_semiprint);
    sem_post(oxygen_semiprint);
    sem_wait(hydrogen_semiprint);

    //Molecule is done being created
    sem_wait(output);
    fprintf(file, "%d: O %d: molecule %d created \n", ++(*line),oid, moliny);
    fflush(file);
    sem_post(output);

    //Checking of all atoms created a molecule
    sem_wait(hydrogen_semiprint);
    sem_post(oxygen_semiprint);
    sem_wait(hydrogen_semiprint);
    sem_post(oxygen_semiprint);

    //Incrementing molecule index
    (*mol_count) += 1;

    //Next molecule is ready to be created
    sem_post(oxygen_molecule_end);
    sem_post(oxygen_molecule_end);

    //Barrier so only 3 atoms can create a molecule at once
    barrier();

    //Checking if there are any leftover atoms
    if(o_counter * 2 != h_counter){
        if(*ox_count_sh == 0 && *h_count_sh == 0){
            *is_last = true;

            for (int i = 0; i < *actual_ox; ++i) {
                sem_post(h_end_sem);
            }
            for (int i = 0; i < *actual_h; ++i) {
                sem_post(o_end_sem);
            }
        }
    }
    sem_post(mutex);
    exit(0);
}

//Function for creating hydrogens
void hydrogen_create(int atom_time, int molecule_time,int hid){

    sem_wait(mutex);

    //Starting a hydrogen
    sem_wait(output);
    fprintf(file, "%d: H %d: started\n", ++(*line), hid);
    fflush(file);
    sem_post(output);

    //Implementing more randomness
    sem_post(mutex);
    sem_wait(mutex);

    //Time to queue a hydrogen
    mysleep(atom_time);

    //Sending hydrogen to queue
    sem_wait(output);
    fprintf(file, "%d: H %d: going to queue\n", ++(*line), hid);
    fflush(file);
    sem_post(output);

    //Incrementing actual count of hydrogens
    (*actual_h)++;

    //Decrementing hydrogens left to queue
    (*h_count_sh)--;

    //Checking if this is the last atom
    if(*actual_h<2 || *actual_ox<1){
        if(o_counter * 2 != h_counter){
            if(*ox_count_sh == 0 && *h_count_sh == 0){
                *is_last = true;

                for (int i = 0; i < *actual_ox; ++i) {
                    sem_post(h_end_sem);
                }
                for (int i = 0; i < *actual_h; ++i) {
                    sem_post(o_end_sem);
                }
            }
        }
    }

    //Checking if molecule is ready to be made
    if(*actual_h >= 2 && *actual_ox >= 1){
        sem_post(o_end_sem);
        sem_post(o_end_sem);
        *actual_h -= 2;
        sem_post(h_end_sem);
        *actual_ox -= 1;
        *is_last = false;
    }
    else{
        sem_post(mutex);
    }

    //Waiting until molecule is ready to be made
    sem_wait(o_end_sem);

    //If all atoms are done being made and all molecules are made - leftovers are printed
    if(*is_last == true){
        sem_wait(output);
        fprintf(file,"%d: H %d: not enough O or H\n", ++(*line), hid);
        sem_post(output);
        exit(0);
    }

    //previous molecule is created, so next can be started
    sem_wait(oxygen_molecule_end);

    //Mol. index
    int moliny = *mol_count;

    //Creating a molecule
    sem_wait(output);
    fprintf(file,"%d: H %d: creating molecule %d\n", ++(*line), hid, moliny);
    fflush(file);
    sem_post(output);

    //Time for molecule to create
    mysleep(molecule_time);

    //Waiting until the molecule is done creating
    sem_post(hydrogen_semiprint);
    sem_wait(oxygen_semiprint);

    //Molecule is created
    sem_wait(output);
    fprintf(file, "%d: H %d: molecule %d created \n", ++(*line),hid, moliny);
    fflush(file);
    sem_post(output);

    //Molecule is done created
    sem_post(hydrogen_semiprint);
    sem_wait(oxygen_semiprint);
    sem_post(hydrogen_molecule_end);

    //Barrier
    barrier();

    //Checking if any leftovers are left
    if(o_counter * 2 != h_counter){
        if(*ox_count_sh == 0 && *h_count_sh == 0){
            *is_last = true;

            for (int i = 0; i < *actual_ox; ++i) {
                sem_post(h_end_sem);
            }
            for (int i = 0; i < *actual_h; ++i) {
                sem_post(o_end_sem);
            }
        }
    }
    exit(0);
}


int main(int argc, char *argv[]) {
    //Checking number of arguments
    if(argc < 5 || argc > 5){
        fprintf(stderr, "Arguments written incorrectly");
        exit(1);
    }
    else{
        //Parsing and controlling arguments
        long tmp;
        char* p;
        tmp = strtol(argv[1], &p, 10);
        if(*p != '\0' || !strcmp(argv[1],"")) {
            fprintf(stderr, "Arguments written incorrectly");
            exit(1);
        }
        o_counter= (int)tmp;
        tmp= strtol(argv[2], &p, 10);
        if(*p != '\0' || !strcmp(argv[2],"")) {
            fprintf(stderr, "Arguments written incorrectly");
            exit(1);
        }
        h_counter=(int)tmp;
        tmp = strtol(argv[3], &p, 10);
        if(*p != '\0' || !strcmp(argv[3],"")) {
            fprintf(stderr, "Arguments written incorrectly");
            exit(1);
        }
        max_wait=(int)tmp;
        tmp= strtol(argv[4], &p, 10);
        if(*p != '\0' || !strcmp(argv[4],"")) {
            fprintf(stderr, "Arguments written incorrectly");
            exit(1);
        }
        max_create=(int)tmp;
        if(o_counter <= 0 || h_counter <= 0 || max_wait < 0 || max_wait > 1000 || max_create < 0 || max_create > 1000){
            fprintf(stderr, "Arguments written incorrectly");
            exit(1);
        }
    }

    //Alocation
    alokacia();

    //Saving values into shared variables
    *ox_count_sh = o_counter;           //Oxygens to create
    *h_count_sh = h_counter;            //Hydrogens to create
    *line = 0;                          //Line counter
    *actual_h=0;                        //Current number of hydrogens in queue
    *actual_ox=0;                       //Current number of oxygens in queue
    *mol_count=1;                       //Molecule index
    *count=0;                           //Counter in barrier
    *is_last = false;                   //Checking if current atom is last

    //Creating all hydrogens
    for (int hid = 1; hid <= h_counter; hid++){
        pid_t hydrogen = fork();
        if(hydrogen < 0){
            fprintf(stderr, "Error while forking");
            dealokacia();
            exit(1);
        }
        else if(hydrogen == 0){
            hydrogen_create(max_wait,max_create,hid);
        }
    }

    //Creating all oxygens
    for (int oid = 1; oid <= o_counter ; oid++) {
        pid_t oxygen = fork();
        if (oxygen < 0) {
            fprintf(stderr, "Error while forking");
            dealokacia();
            exit(1);
        } else if (oxygen == 0) {
            oxygen_create(max_wait,max_create ,oid);
        }
    }

    //Waiting for children to die
    while(wait(NULL)>0);

    //Closing the file
    fclose(file);

    //Dealocating
    dealokacia();
    exit(0);
}
