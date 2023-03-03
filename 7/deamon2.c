#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<syslog.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<string.h>
#define NNAME "./null"
#define FNAME "./out"
static int  deamon_n1(void){
	int fd;
	pid_t pid;
	fflush(NULL);
	pid = fork();
	if (pid<0){
		perror("fork()");
		return -1;
	}
	if (pid>0){
		exit(0);
	}
	FILE *ffp;

	pid_t pid2;
	
	pid2 = setsid();
	printf("%d\n",pid2);
	//chdir("/");
	//	umask(0);
	return 0;

}
int main(){
	FILE * fp;
	openlog("mydaemon",LOG_PID,LOG_DAEMON);
	
	if (deamon_n1()){
		syslog(LOG_ERR,"daemon_n1() failed!");

	}else{
		syslog(LOG_INFO,"daemon_n1 ok!");
	}
	fp = fopen(FNAME,"w");
	if(fp==NULL){
		syslog(LOG_ERR,"fopen():%s",strerror(errno));
		exit(1);
	}
	syslog(LOG_INFO,"%s was opened.",FNAME);
	for(int i=0;i>=0;i++){
		fprintf(fp,"%d\n",i);
		fflush(fp);

		sleep(1);
	}
	fclose(fp);
	closelog();
	exit(0);
}
