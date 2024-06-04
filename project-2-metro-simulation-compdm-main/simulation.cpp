#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <queue>
#include <string>
#include <ctime>
#include <ostream>
#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <sstream>

#define LIMIT 10

pthread_mutex_t section_mutex;//we are controlling the access to Sections
pthread_mutex_t train_count_mutex;//controlling total number of trains in system
pthread_cond_t overload; //conditional variable of train count
pthread_mutex_t log_mutex; //controlling the log 

sem_t log_semaphore; //semaphore of control.log
sem_t control_semaphore;//semaphore of control
sem_t tunnel_semaphore;//semaphore of tunnel
sem_t train_log_semaphore;//semaphore of tunnel.log

FILE* trainfile;
FILE* controlfile;

bool flag=false;
bool end1=false;
int simulation_time; 
int train_number=0; //train number of the system
int metro_id=0; //the id to given the newly created metro
float p; //probability 
int seed;
time_t start; //simualtion start time
time_t end; //simulation end time
std::queue<std::string> qlog;//queue for control.log messages
std::queue<std::string> qtrainlog;//queue for tunnel.log messages

struct Metro{//metro struct

        int id;//metro id
        int length;//metro length
        char from;//metro starting point
        char to;//metro finish point
	time_t arrival;//metro arrival time to simulation
	time_t depart;//metro departing time to simulation
};

struct Section{ //struct for all sections like A to E etc.

	char start;//starting point of section
	char end;//end point of section
	std::queue <Metro*> q;//queue that has the metros in a section
	int train_num;//number of trains that in a section
};

struct Tunnel{//struct for the tunnel

	Metro *metro;
};

struct Section sectionAC;
struct Section sectionBC;
struct Section sectionDE;
struct Section sectionDF;

struct Tunnel tunnel;

//functions that we use 
void *func_sec(void *ptr);//section threads use this
void *func_tun(void *ptr);//tunnel thread uses this
void *func_cont(void *arg);//control thread uses this
void *func_log(void *arg);//log.control thread uses this
std::string get_time();//returns the time as a string
std::string get_ids();//returns all trains ids as a string
int thread_sleeper (int sec);//sleeps the thread
void *func_tlog(void *arg);//tunnel.log thread uses this
std::string timeToString(time_t time);//returns given time as a string

int main(int argc, char *argv[]){



	if(argc==4){//checking the command line arguments
		p=atof(argv[1]);
		simulation_time=atoi(argv[2]);
		seed=atoi(argv[3]);
	}
	else{
		printf("Not enough arguments to start the simulation!");
		return 0;
	}

	start=time(NULL);//set starting time to current time.
	end=start+simulation_time;//calculating the ending time.

//now we need to do the initializations.

	tunnel;
	sectionAC.start='A';
	sectionAC.end='C';
	sectionAC.train_num=0;

	sectionBC.start='B';
	sectionBC.end='C';
	sectionBC.train_num=0;

        sectionDE.start='E';
        sectionDE.end='D';
        sectionDE.train_num=0;

	sectionDF.start='F';
        sectionDF.end='D';
        sectionDF.train_num=0;

	//initializing the mutex and semaphores
	pthread_mutex_init(&train_count_mutex,NULL);
	pthread_cond_init(&overload,NULL);
	pthread_mutex_init(&log_mutex,NULL);
	sem_init(&log_semaphore,0,0);
	sem_init(&tunnel_semaphore,0,0);
	pthread_mutex_init(&section_mutex,NULL);
	sem_init(&control_semaphore,0,0);
	sem_init(&train_log_semaphore,0,0);
	//creating the attribute to pass the threads.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	

	//crreating the needed threads.

	//we decided to create threads for all sections.
	pthread_t t_AC;
	pthread_t t_BC;
	pthread_t t_DE;
	pthread_t t_DF;

	//we decided to create a thread for tunnel and control center and logs

	pthread_t t_control;
	pthread_t t_tunnel;
	pthread_t t_log;
	pthread_t train_log;
	//creation of the above threads.

	pthread_create(&t_AC,&attr,func_sec, &sectionAC);
	pthread_create(&t_BC,&attr,func_sec, &sectionBC);
	pthread_create(&t_DE,&attr,func_sec, &sectionDE);
	pthread_create(&t_DF,&attr,func_sec, &sectionDF);
	pthread_create(&t_tunnel,&attr,func_tun,&tunnel);
	pthread_create(&t_control,&attr,func_cont,NULL);
	pthread_create(&t_log,&attr,func_log,NULL);
	pthread_create(&train_log,&attr,func_tlog,NULL);

//	pthread_join(t_AC,NULL);
//	pthread_join(t_BC,NULL);
//	pthread_join(t_DE,NULL);
//	pthread_join(t_DF,NULL);
//	pthread_join(t_control,NULL);

	sem_post(&tunnel_semaphore);
//	pthread_join(t_tunnel,NULL);

//	qlog.push(std::string("Program Terminated"));
//	printf("program terminated");
//	sem_post(&log_semaphore);
//	pthread_join(t_log,NULL);


//now we will end the simulation
	 while(end>time(NULL));
	while(end1==false);
	pthread_cancel(t_AC);
        pthread_cancel(t_BC);
        pthread_cancel(t_DE);
        pthread_cancel(t_DF);
        pthread_cancel(t_tunnel);
        pthread_cancel(t_control);
        pthread_cancel(t_log);
	pthread_cancel(train_log);


	//destroying the semaphore and mutexes
	sem_destroy(&log_semaphore);
	sem_destroy(&control_semaphore);
	sem_destroy(&tunnel_semaphore);
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&train_count_mutex);
   	pthread_cond_destroy(&overload);
  	pthread_mutex_destroy(&section_mutex);
  	pthread_mutex_destroy(&log_mutex);
	sem_destroy(&train_log_semaphore);
   	exit(0);
}	



void *func_sec(void *ptr){//section thread function

	struct Section *section=(Section*) ptr;//got the section
	float prob;//the probability to understand where is the start


	if(section->start=='B'){
		prob=1-p;
	}else{
		prob=p;
	}


	while(time(NULL)<end){


		int train_id;


		if( ((rand() %10) +1)  <= prob*10){//we are checking if we should create a train or not


			pthread_mutex_lock(&train_count_mutex);
			// we used a mutex because we want to give access to metro_id 1 thread at a time
			if(train_number>LIMIT){//if train limit is passed, wait for overload, no new train can come.
				pthread_cond_wait(&overload,&train_count_mutex);
			}
			train_id=metro_id;//give new train an id
			metro_id++;//new trains id
			train_number++;//number of trains in the system increased by 1

			pthread_mutex_unlock(&train_count_mutex);

			struct Metro *metro=(struct Metro*)malloc(sizeof(struct Metro));//metro created

			if(( (rand() %10 ) +1) >3){//with 0.3 prob, length is 200m, with 0.7 prob, length is 100m
				metro->length=100;
			}else{
				metro->length=200;
			}
			
			metro->id=train_id;//metro got its id
			metro->from=section->start;//metro got its starting point
			
			//calculating the destination of metro via probability.
			int prob_path=(rand() %10)+1;

			if(metro->from=='E' || metro->from=='F'){
				if(prob_path>=5){
					metro->to='B';

				}else{
					metro->to='A';
				}

			}else{
				if(prob_path>=5){
					metro->to='E';
				}else{
					metro->to='F';
				}
			}
			
			
			metro->arrival=time(NULL);//metro arrival time is
			thread_sleeper(1);

			pthread_mutex_lock(&section_mutex);
			(section->q).push(metro);//put the metro to the sections' queue
			(section->train_num)++;//number of trains in section increased by 1
			
			sem_post(&control_semaphore);
			pthread_mutex_unlock(&section_mutex);


			}else{

			thread_sleeper(1);  
			}
		}

	pthread_exit(NULL);
}





void *func_tun(void *ptr){

	struct Tunnel *tunnel=(Tunnel*) ptr;

	while(true){
	sem_wait(&tunnel_semaphore);
	if(end<time(NULL)){
		break;}
	
	thread_sleeper(1);
	pthread_mutex_lock(&train_count_mutex);
	train_number--;
	pthread_mutex_unlock(&train_count_mutex);

}

pthread_exit(NULL);
}


void *func_cont(void *arg){


	bool clear=false;
	int slow_start;
	struct Metro *metro;

	while(time(NULL)<end){

		pthread_mutex_lock(&train_count_mutex);
		if(train_number>LIMIT){//If limit is reached
			pthread_mutex_lock(&log_mutex);
			std::string message="System Overload\t\t" +get_time() + "\t\t#\t\t"+get_ids();//create an overload message
			qlog.push(message);//push the message to control.logs' queue
			sem_post(&log_semaphore);
			pthread_mutex_unlock(&log_mutex);
			clear=true;//got overload so need clearance
			slow_start=time(NULL);
		}

		if(clear){//if we need to clear
			if(train_number<=1){//if train number <=1
				int clear_time=time(NULL)-slow_start;
				pthread_mutex_lock(&log_mutex);
				std::string message = "Tunnel Cleared\t\t" + get_time() + "\t\t#\t\tTime to Clear: " + std::to_string(clear_time);//create clerance message
				qlog.push(message);//push the message to control.log queue
				sem_post(&log_semaphore);
				pthread_mutex_unlock(&log_mutex);
				clear=false;
				pthread_cond_broadcast(&overload);//wake up all threads that waiting because of overload
			}
		}
		pthread_mutex_unlock(&train_count_mutex);

		sem_wait(&control_semaphore);
		pthread_mutex_lock(&section_mutex);

		//breaking ties and decide which train go in the tunnel
		//indexes are sorted as below
		Section* array[4];
		array[0]=&sectionAC;
		array[1]=&sectionBC;
		array[2]=&sectionDE;
		array[3]=&sectionDF;

		int index=0;
		for(int i=1;i<4;i++){//checking if BC>AC, if not train from AC go etc.
				//therefore in ties. A>B>E>F 
			if(array[i]->train_num > array[index]->train_num){
				index=i;
			}
		}

		metro=array[index]->q.front();//got the metro to go the tunnel
		array[index]->q.pop();//pop that metro from the queue of the section
		array[index]->train_num--;//decrease the train number of that section by 1
		
		pthread_mutex_unlock(&section_mutex);

		pthread_mutex_lock(&log_mutex);
		std::string message="Tunnel Passing\t\t" +get_time() + "\t\t"+std::to_string(metro->id)+ "\t\t"+get_ids();//create tunnel passing message
		qlog.push(message);//push the message to the control.logs' queue
		sem_post(&log_semaphore);
		pthread_mutex_unlock(&log_mutex);

		int time_tunnel= (metro->length)/100 +1; //calculating the time passed in tunnel
		//sleep
		thread_sleeper(time_tunnel);//time passing in tunnel
		
		if( ((rand() %10) +1) ==1){//calculating if there wil be a breakdown
			pthread_mutex_lock(&log_mutex);
			std::string message="Breakdown\t\t" +get_time() + "\t\t"+std::to_string(metro->id)+"\t\t"+get_ids();//breakdown message
			qlog.push(message);//message pushed to control.logs' queue
			sem_post(&log_semaphore);
			pthread_mutex_unlock(&log_mutex);
			//sleep for 4 seconds for breakdown
			thread_sleeper(4);
			
		}
		tunnel.metro=metro;
		metro->depart=time(NULL)+1;//calculating the departing time of the metro
		sem_post(&tunnel_semaphore);
		
		pthread_mutex_lock(&log_mutex);
		std::string message1=std::to_string(metro->id)+ "\t\t"+metro->from+ "\t\t"+metro->to+ "\t\t\t"+std::to_string(metro->length)+ "\t\t"+timeToString(metro->arrival)+ "\t\t"+timeToString(metro->depart)+ "\t";//message for train.log
		qtrainlog.push(message1);//message pushed
		sem_post(&train_log_semaphore);
		pthread_mutex_unlock(&log_mutex);
	}

	pthread_exit(NULL);
}


void *func_log(void *arg){//function of control.log
	while(flag==false){};
	controlfile=fopen("controllog.txt","w");
	std::cout << "Event\t\tEvent Time\t\tTrain ID\t\tTrains Waiting Passage" << std::endl;
	fprintf(controlfile,"Event\t\tEvent Time\t\tTrain ID\t\tTrains Waiting Passage\n");
	//while(time(NULL)<end){

	while(!qlog.empty()){	 

		sem_wait(&log_semaphore);
		std::cout << qlog.front() << std::endl;
		fprintf(controlfile,"%s\n",qlog.front().c_str());
		qlog.pop();
	}
	end1=true;
fclose(controlfile);
pthread_exit(NULL);
}

std::string get_time(){
	time_t t=time(NULL);
	tm* timePtr=localtime(&t);
	int s=timePtr->tm_sec;
	int m=timePtr->tm_min;
	int h=timePtr->tm_hour;
	
	std::ostringstream oss; // Create a stringstream to format the string
   	 oss << std::setfill('0'); // Set fill character to '0'

    
   	 oss << std::setw(2) << h << ":" << std::setw(2) << m << ":" << std::setw(2) << std::setfill('0') << s;

    return oss.str(); 
//	return ""+std::to_string(h)+":"+std::to_string(m)+":"+std::to_string(s);
}

void *func_tlog(void *arg){

	std::cout << "Train ID\tStarting Point\tDestination Point\tLength(m)\tArrival Time\t\t\tDeparture Time" << std::endl;
	trainfile=fopen("trainlog.txt","w");
	fprintf(trainfile,"Train ID\tStarting Point\tDestination Point\tLength(m)\tArrival Time\t\t\tDeparture Time\n");
	while(time(NULL)<end){
		sem_wait(&train_log_semaphore);
		std::cout << qtrainlog.front() << std::endl;
		fprintf(trainfile,"%s\n",qtrainlog.front().c_str());
		qtrainlog.pop();
	}
fclose(trainfile);
flag=true;
pthread_exit(NULL);

}


std::string get_ids() {
    std:: queue<Metro*> q1 = sectionAC.q;
    std::queue<Metro*> q2 = sectionBC.q;
    std:: queue<Metro*> q3 = sectionDE.q;
    std::queue<Metro*> q4 = sectionDF.q;
    std::string result;

    while (!q1.empty()) {
        result += std::to_string(q1.front()->id) + ",";
        q1.pop();
    }
    while (!q2.empty()) {
        result += std::to_string(q2.front()->id) + ",";
        q2.pop();
    }
    while (!q3.empty()) {
        result += std::to_string(q3.front()->id) + ",";
        q3.pop();
    }
    while (!q4.empty()) {
        result += std::to_string(q4.front()->id) + ",";
        q4.pop();
    }


    if (!result.empty()) {
        result.pop_back();
    }

    return result;
}



int thread_sleeper (int sec){
	pthread_mutex_t sleep_mutex;
	pthread_cond_t cond;
	struct timeval tp;
	struct timespec fin;

	if(pthread_mutex_init(&sleep_mutex,NULL)){

	return -1;}

	if(pthread_cond_init(&cond,NULL)){

	return -1;}
 
	gettimeofday(&tp, NULL);
	fin.tv_sec = tp.tv_sec + sec;
	fin.tv_nsec = tp.tv_usec * 1000;

	pthread_mutex_lock(&sleep_mutex);
	int out=pthread_cond_timedwait(&cond, &sleep_mutex, &fin);
	//according to IBM documentation, pthread_cond_timedwait should wait for either the condition or the given time to pass.
	//condition is not important for us, we just waiting for the time to pass.
	pthread_mutex_unlock (&sleep_mutex);
	pthread_mutex_destroy(&sleep_mutex);
	pthread_cond_destroy(&cond);

	return out;
}

std::string timeToString(time_t time) {
    std::tm *tm = std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
