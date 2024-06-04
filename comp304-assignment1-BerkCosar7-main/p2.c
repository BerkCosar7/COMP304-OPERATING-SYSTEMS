#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#define BUFFER_SIZE 1
#define READ_END 0
#define WRITE_END 1

void max_finder(int *array, int size,  int *pipe,int child){

//finding local maximum of array
int local_max =array[0];
for(int i=1;i<size;i++){
	if(array[i]>local_max){
		local_max=array[i];
	}
}

//sending the local maximum to the parent with pipe.
write(pipe[WRITE_END],&local_max,sizeof(int));
printf("Child %d: Local Maximum: %d\n",child,local_max);
close(pipe[WRITE_END]);
exit(0);
}



int main(int argc, char *argv[]){

   int M =atoi(argv[1]);
   int N =atoi(argv[2]);

   if(M<N){
        printf("M cant be smaller than N, program will terminated.");
	return 0;
   }


  pid_t pid;
  int fd[2];

//creating the pipe
  if(pipe(fd) == -1){
	fprintf(stderr, "Pipe failed");
	return -1;
 }

//generation of a list with M elements randomly choosen between [0,255]
int num_list[M];
srand(time(NULL));
for(int i=0; i<M;i++){
	num_list[i] = rand() % 256;
}

//generating N forks
for(int i=0; i<N; i++){

pid=fork();

if(pid<0){
perror("Fork failed");
return -1;
}

if(pid==0){ //child
close(fd[READ_END]);

//calculating the element per child
int elem_per_child = M/N;
for(int j=0;j<M%N;j++){
	if(i==j){
		elem_per_child+=1;
		break;
	}
}	  

//calculating the index that current child should start reading
int start=i*(M/N);
for(int j=0;j<M/N;j++){
	if(i>=j){
		start+=1;
	}
}

//calculating the index that current child should stop reading
int end=start+elem_per_child;


int sublist[M];
int sublist_size=0;

for(int j= start;j<end;j++){
	sublist[sublist_size++] = num_list[j];
}

max_finder(sublist,sublist_size,fd,i);
}
}

//parent process

close(fd[WRITE_END]);

int global_max = 0;

for(int i=0; i<N; i++){

int local_max;
read(fd[READ_END], &local_max,sizeof(int));

	if(local_max > global_max){
	global_max=local_max;
	}
}
close(fd[READ_END]);
printf("Global maximum is: %d\n", global_max);




return 0;}
