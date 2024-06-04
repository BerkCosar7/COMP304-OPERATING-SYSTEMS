#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE 1000


		
int main(int argc, char *argv[]){

char *input= argv[1];
char *output=argv[2];
char *name  =  "shared_memory";

int fdA[2];
int ack_pipe[2];

if(pipe(ack_pipe) == -1){
	printf("pipe failed");
	return -1;
}


pid_t A;

if(pipe(fdA) == -1 ){
	printf("Pipe failed");
	return -1;
}

A=fork();

if(A<0){
	printf("Fork failed.");
	return -1;
}


if(A==0){//child process
	
	int shm_fd;
	char *shm_add;
	ssize_t size=1024;


shm_fd=shm_open(name,O_CREAT | O_RDWR, 0666);
if(shm_fd==-1){
        printf("Shared memory failed");
        return -1;
}

ftruncate(shm_fd,size);

shm_add=mmap(0,size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd,0);
if(shm_add ==MAP_FAILED){
	printf("1mapfailed");
	return -1;
}
close(fdA[WRITE_END]);
ssize_t total_writen=0;

while(1){

	char buffer[1024];
	ssize_t b_read=read(fdA[READ_END],buffer,sizeof(buffer));
	if(b_read<=0){
		break;
	}
	memcpy(shm_add+total_writen,buffer,b_read);
	total_writen+=b_read;
	if(total_writen>=size){
		break;;
	}
}


printf("shared memo: %s",shm_add);
const char *ack_message = "Writing to shared memory is completed";
write(ack_pipe[WRITE_END],ack_message,strlen(ack_message));
exit(0);
}else{

int ack_pipe2[2];
if(pipe(ack_pipe2)==-1){
	printf("pipe failed");
	return -1;
}

close(fdA[READ_END]);
FILE *pipe_write=fdopen(fdA[WRITE_END],"w");
FILE *input_fp=fopen(input,"r");

char buffer[1024];
ssize_t bytes_read;
while((bytes_read=fread(buffer,1,sizeof(buffer),input_fp))>0){
	fwrite(buffer,1,bytes_read,pipe_write);
}

fclose(input_fp);
fclose(pipe_write);
wait(NULL);
close(ack_pipe[WRITE_END]);
char ack_buffer[1024];
ssize_t ack_read=read(ack_pipe[READ_END],ack_buffer,sizeof(ack_buffer));
if(ack_read>0){
	ack_buffer[ack_read]='\0';
	printf("Acknowledgement from Child: %s\n",ack_buffer);
	pid_t B = fork();
	if( B==-1){
		printf("fork failed");
		return -1;
	}else if(B==0){

	char *name ="shared_memory";
	int shm_fd;
        char *shm_add;
        ssize_t size=1024;


shm_fd=shm_open(name,O_RDONLY, 0666);
if(shm_fd==-1){
        printf("Shared memory failed");
        return -1;
}

shm_add=mmap(0,size, PROT_READ , MAP_SHARED, shm_fd,0);
if(shm_add ==MAP_FAILED){
        printf("1mapfailed");
        return -1;
}



 	int output_fd=open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	write(output_fd,shm_add,strlen(shm_add));
	close(output_fd);
	const char *second_ack_mes="Contents of shared memory is written to the output file";
	write(ack_pipe2[WRITE_END],second_ack_mes,strlen(second_ack_mes));
       exit(0);
}

wait(NULL);
close(ack_pipe[READ_END]);

ack_read=read(ack_pipe2[READ_END],ack_buffer,sizeof(ack_buffer));


if(ack_read>0){
        ack_buffer[ack_read]='\0';
        printf("Acknowledgement from 2Child: %s\n",ack_buffer);
	FILE *i=fopen(input,"r");
	FILE *o=fopen(output,"r");
	int c1,c2;	
        int compare=0;
	while(1){
		c1=fgetc(i);
		c2=fgetc(o);
		if(c1!=c2){
			printf("not equal");
			compare=1;
			return compare;
		}else{
			printf(" equal");
			compare=0;
			return compare;
			
		}

}


}


}
}
return 0;    
}
