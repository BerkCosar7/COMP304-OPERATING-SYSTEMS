#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int argc, char *argv[] ){

struct timeval current_time;
gettimeofday(&current_time,NULL);

struct tm *local_time;
local_time = localtime(&current_time.tv_sec);

char time_str[9];
strftime(time_str, sizeof(time_str), "%H:%M:%S",local_time);

int input=atoi(argv[1]);

pid_t pid;
pid_t parent;
parent =getpid();
int level=0;
printf("Main process ID: %d, Level: %d, Time: %s\n",getpid(),level,time_str);
level +=1;


for(int i=0; i<input;i++){

	pid = fork();

	if(pid == 0){//child process

		for( int j=0;j<3;j++){

         		 struct timeval current_time;
   	       		gettimeofday(&current_time,NULL);

			struct tm *local_time;
			local_time = localtime(&current_time.tv_sec);

			char time_str[9];
			strftime(time_str, sizeof(time_str), "%H:%M:%S",local_time);


          printf("Process ID: %d,Parent ID: %d,  Level: %d, Time: %s\n",getpid(),parent, level,time_str);

          
          sleep(1);}
          parent = getpid();
          level +=1;
	}else if(pid > 0){//parent process
	wait(NULL);}
	}
return 0;}


