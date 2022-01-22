#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<pthread.h>

//rle is RunLengthEncoding(RLE)

int sz ;
int nooftasks=0;
int currenttaskindex=0;
int addedtaskcount=0;
int threadresultstaskcount = 1;
int rleexecutioncompleted = 0;
int threadrlestringfinalcount =0;
int maxrlecharcount=0;
int totaltaskindex =0;

char** filemappedinmain;

typedef struct Task {
	int blocksize;
	int mmapoffset;
	int filedescriptor;
	int taskindex;
	char* mmappedfile;

} Task ;


Task *taskqueue ; 

int* threadrlecharcount;

char** threadrlestringtemp;

char* threadrlestringfinal;

int finalrlecharcount = 0;

pthread_mutex_t mutextaskqueue;
pthread_mutex_t mutexresult;
pthread_cond_t condtaskqueue;
pthread_cond_t condresult;


void runrle_thread(Task* task){

	char *filemapped;

	filemapped = (char*)calloc((task->blocksize),sizeof(char));

	filemapped = (task->mmappedfile)+(task->mmapoffset);

	char tempchar;
	char charcount = 0;
	int rlecharcount = 0;
	char **rlestring;

	
tempchar = filemapped[0] ;
	charcount = 1;

	for(int km = 1; km < (task->blocksize) ; km++){

		if(filemapped[km] == tempchar){

			charcount += 1;
		}

		else{

	   		if(rlecharcount==0){

	   				rlestring = (char**)calloc((rlecharcount+1),sizeof(char*));
		  			rlestring[rlecharcount] = (char*)calloc(2,sizeof(char));
		  			rlestring[rlecharcount][0] = tempchar ;
		  			rlestring[rlecharcount][1] = charcount ;
		  			rlecharcount+=1;	
	   		}

	   		else{

		   			rlestring = (char**)realloc(rlestring,(rlecharcount+1)*sizeof(char*));
		  			rlestring[rlecharcount] = (char*)calloc(2,sizeof(char));
		  			rlestring[rlecharcount][0] = tempchar ;
		  			rlestring[rlecharcount][1] = charcount ;
		 			rlecharcount+=1;
	   		}
	  			
	   		tempchar = filemapped[km];
	   		charcount = 1;
		}

	}	
		if(rlecharcount==0){

			rlestring = (char**)calloc((rlecharcount+1),sizeof(char*));
  			rlestring[rlecharcount] = (char*)calloc(2,sizeof(char));
  			rlestring[rlecharcount][0] = tempchar ;
  			rlestring[rlecharcount][1] = charcount ;
  			rlecharcount+=1;	
		}

	 	else {

			rlestring = (char**)realloc(rlestring,(rlecharcount+1)*sizeof(char*));
			rlestring[rlecharcount] = (char*)calloc(2,sizeof(char));
			rlestring[rlecharcount][0] = tempchar ;
			rlestring[rlecharcount][1] = charcount ;
			rlecharcount+=1;
			}

		threadrlestringtemp[(task->taskindex)-1] = (char*)calloc(2*rlecharcount+1,sizeof(char));

		threadrlecharcount[(task->taskindex)-1]= rlecharcount;

		for(int g=0;g<rlecharcount;g++){

			 threadrlestringtemp[(task->taskindex)-1] = strncat(threadrlestringtemp[(task->taskindex)-1],rlestring[g],2);
		 }

		if(task->taskindex == nooftasks){
			while(totaltaskindex!=nooftasks-1){}

				for(int z=0;z<nooftasks;z++){

					maxrlecharcount+=threadrlecharcount[z];
				}
			
			for(int cnt = 1; cnt< nooftasks ; cnt++){

				if(threadrlestringtemp[cnt-1][2*threadrlecharcount[cnt-1]-2]==threadrlestringtemp[cnt][0]){

					threadrlestringtemp[cnt][1] = threadrlestringtemp[cnt][1] + threadrlestringtemp[cnt-1][2*threadrlecharcount[cnt-1]-1];
					 threadrlestringfinalcount += 2*threadrlecharcount[cnt-1]-2;
					
					 			fprintf(stdout,"%.*s",((2*threadrlecharcount[cnt-1])-2),threadrlestringtemp[cnt-1]);		
				}

				else{
					 threadrlestringfinalcount += 2*threadrlecharcount[cnt-1];
					
				fprintf(stdout,"%.*s",2*threadrlecharcount[cnt-1],threadrlestringtemp[cnt-1]);		

				}
			}
				threadrlestringfinalcount += 2*threadrlecharcount[nooftasks-1];
				
				fprintf(stdout,"%.*s",2*threadrlecharcount[nooftasks-1],threadrlestringtemp[nooftasks-1]);		

		 	exit(0);
		 }
		}

void addtask(Task task ) {

    pthread_mutex_lock(&mutextaskqueue);
	    taskqueue[task.taskindex-1] = task;
	    addedtaskcount+=1;
	pthread_cond_signal(&condtaskqueue);

    pthread_mutex_unlock(&mutextaskqueue);
 
}



void* waitfortaskthread(void* arguments){

	while(1) {

		Task task;

		pthread_mutex_lock(&mutextaskqueue);
			if(addedtaskcount==0 || (currenttaskindex >= addedtaskcount) ){
				pthread_cond_wait(&condtaskqueue, &mutextaskqueue);
			}
			if(addedtaskcount>0 && currenttaskindex < addedtaskcount){
				task = taskqueue[currenttaskindex];
								currenttaskindex+=1;
			}
		pthread_mutex_unlock(&mutextaskqueue);

		runrle_thread(&task);

		pthread_mutex_lock(&mutexresult);
				totaltaskindex+=1;
		pthread_mutex_unlock(&mutexresult);	

	}

}

int main(int argc, char **argv) {

	sz = sysconf(_SC_PAGE_SIZE);

	pthread_mutex_init(&mutextaskqueue, NULL);
	pthread_mutex_init(&mutexresult, NULL);
    pthread_cond_init(&condtaskqueue, NULL);
    pthread_cond_init(&condresult, NULL);

	int option;
	int numthreads = 0;
	int multithread = 0;

	while((option = getopt(argc, argv, "j:"))!=-1){

		switch(option) {

  		case 'j' :
  			numthreads = atoi(optarg);
  			multithread = 1;
  			break;

  		default :
			numthreads = -1 ;	
		}
	}

	if (numthreads>0){
		char redirection[] = ">";
		int filecount = 0;

		for(int k=3;k<argc;k++){	

			if(k<argc && (strcmp(argv[k],redirection) != 0)){
				filecount+=1;
			}
		}

		struct stat buffer[filecount];
		int fd[filecount];
		off_t size[filecount];
		filemappedinmain = (char**)calloc(filecount,sizeof(char*));
		
		for( int k=0;k<filecount;k++){
			if((fd[k] = open(argv[k+3],O_RDONLY)) > 0){
			}
			fstat(fd[k],&buffer[k]);
			size[k] = buffer[k].st_size;

			filemappedinmain[k] = (char*)calloc(size[k],sizeof(char));
			filemappedinmain[k] = mmap(NULL,size[k],PROT_READ,MAP_PRIVATE,fd[k],0);
		}

		for(int k = 0; k < filecount ; k++){
			if(size[k]%sz == 0){
				for (int l = 0 ; l < (size[k]/sz) ; l++){
					nooftasks+=1;					
				}
			}
			else {
				for(int l = 0 ; l < (size[k]/sz) ; l++){
					nooftasks+=1;
				};
			}
				nooftasks+=1;
		}

		threadrlecharcount = (int*)calloc(nooftasks,sizeof(int));
		
		taskqueue = (Task*)calloc(nooftasks,sizeof(struct Task)) ; 

		threadrlestringtemp = (char**)calloc(nooftasks,sizeof(char*));

		pthread_t jobthreads[numthreads];	

		pthread_t resultsthread;

		for(int i=0;i<numthreads;i++){

			if(pthread_create(&jobthreads[i], NULL, &waitfortaskthread, NULL) != 0) {
				perror("Failed to create a thread");
			}  
		}

		int index = 0;

		for(int k = 0; k < filecount ; k++){

			int l = 0;
			if(size[k]%sz == 0){

				for (l = 0 ; l < (size[k]/sz) ; l++){

					index+=1;

					Task add = {
						add.blocksize = sz,
						add.mmapoffset = l*sz ,
						add.filedescriptor = fd[k] ,
						add.taskindex = index ,
						add.mmappedfile = filemappedinmain[k]

					};
					addtask(add);

				}

			}

			else {

				for( l = 0 ; l < (size[k]/sz) ; l++){

					index+=1;
					Task add = {
						add.blocksize = sz,
						add.mmapoffset = l*sz ,
						add.filedescriptor = fd[k] ,
						add.taskindex = index ,
						add.mmappedfile = filemappedinmain[k]
					};
					addtask(add);


				}
				index+=1;
				Task add = {

					add.blocksize = size[k]-(sz*(size[k]/sz)),
					add.mmapoffset = (size[k]/sz)*sz,
					add.filedescriptor = fd[k],
					add.taskindex = index ,
					add.mmappedfile = filemappedinmain[k]
				};
				addtask(add);

			}
		}
	}


	else {

		numthreads = 1;

		char redirection[] = ">";
		int filecount = 0;

		for(int k=1;k<argc;k++){	

			if(k<argc && (strcmp(argv[k],redirection) != 0)){
				filecount+=1;
			}
		}

		struct stat buffer[filecount];
		int fd[filecount];
		off_t size[filecount];
		filemappedinmain = (char**)calloc(filecount,sizeof(char*));

		for( int k=0;k<filecount;k++){
			if((fd[k] = open(argv[k+1],O_RDONLY)) > 0){
			}
			fstat(fd[k],&buffer[k]);
			size[k] = buffer[k].st_size;
			filemappedinmain[k] = (char*)calloc(size[k],sizeof(char));
			filemappedinmain[k] = mmap(NULL,size[k],PROT_READ,MAP_PRIVATE,fd[k],0);
		}


		for(int k = 0; k < filecount ; k++){
			if(size[k]%sz == 0){
				for (int l = 0 ; l < (size[k]/sz) ; l++){
					nooftasks+=1;					
				}
			}

			else {
				for(int l = 0 ; l < (size[k]/sz) ; l++){
					nooftasks+=1;
				};
			}
				nooftasks+=1;

		}

		threadrlecharcount = (int*)calloc(nooftasks,sizeof(int));
		
		taskqueue = (Task*)calloc(nooftasks,sizeof(struct Task)) ; 

		threadrlestringtemp = (char**)calloc(nooftasks,sizeof(char*));
	
		pthread_t jobthreads[numthreads];	


		for(int i=0;i<numthreads;i++){

			if(pthread_create(&jobthreads[i], NULL, &waitfortaskthread, NULL) != 0) {
				perror("Failed to create a thread");
			}  
		}
		
		int index = 0;

		for(int k = 0; k < filecount ; k++){

			int l=0;
			if(size[k]%sz == 0){

				for (l = 0 ; l < (size[k]/sz) ; l++){

					index+=1;
					Task add = {
						add.blocksize = sz,
						add.mmapoffset = l*sz ,
						add.filedescriptor = fd[k] ,
						add.taskindex = index ,
						add.mmappedfile = filemappedinmain[k]

					};
					addtask(add);
				}
			}

			else {

				for( l = 0 ; l < (size[k]/sz) ; l++){

					index+=1;
					Task add = {
						add.blocksize = sz,
						add.mmapoffset = l*sz ,
						add.filedescriptor = fd[k] ,
						add.taskindex = index ,
						add.mmappedfile = filemappedinmain[k]
					};
					addtask(add);
				}
				index+=1;
				Task add = {

					add.blocksize = size[k]-(sz*(size[k]/sz)),
					add.mmapoffset = (size[k]/sz)*sz,
					add.filedescriptor = fd[k],
					add.taskindex = index ,
					add.mmappedfile = filemappedinmain[k]
				};
				addtask(add);

			}
		}
				
	}

		while(1){}

	return 0;
}




