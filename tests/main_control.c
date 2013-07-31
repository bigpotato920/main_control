#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "msg.h"

#define UNIX_SERV_PATH "/tmp/sinfor"

char *file_path_process_gps = "./process_gps &";
char *file_path_process_ip = "./process_ip &";
char *file_path_process_serial = "./process_serial &";
char *file_path_process_video = "./process_video &";

struct timer
{
	struct timeval clock;
	void (*handle)(char *);
	char *date;
	int using;
};

struct timer process_timer[4];

void callback_func(char *file_path)
{
	system(file_path);
}

void timer_init(struct timer *t, void (*f)(char *), char *date)
{
	struct timeval starttime;
	gettimeofday(&starttime, NULL);
	t->handle = f;
	t->date = date;
	t->clock.tv_sec = starttime.tv_sec;
	t->clock.tv_usec = starttime.tv_usec;
	t->using = 1;
}

void timer_reset(struct timer *t)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	t->clock.tv_sec = now.tv_sec;
	t->clock.tv_usec = now.tv_usec;
}

void timer_timeout()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	int i;
	for(i = 0; i < 4; i++)
	{
		if((process_timer[i].using == 1) && (now.tv_sec - process_timer[i].clock.tv_sec >= 30))
		{
			process_timer[i].handle(process_timer[i].date);
			timer_reset(&process_timer[i]);
		}
	}
}

void config_read(regismsg *msg)
{
	FILE *fd;
	fd = fopen("./configue", "r");
	char read[100] = {'\0'},pres[100] = {'\0'};
	char *now, *post = NULL;
	now = fgets(read, 100, fd);
	while(now != NULL)
	{
		post = strchr(read, '=');
		strncpy(pres, now, post-now);
		if(post != NULL)
		{
			post++;
			if(!(strcmp(pres,"NODE_NUMBER")))
				msg->nodenum = atoi(post);
			if(!(strcmp(pres,"IP")))
			{
				memset(msg->ip, '\0', sizeof(msg->ip));
				strncpy(msg->ip, post, strlen(post)-1);
			}
			if(!(strcmp(pres,"FLAG_GPS")))
				msg->flag_gps = atoi(post);
			if(!(strcmp(pres,"FLAG_IP")))
				msg->flag_ip = atoi(post);		
			if(!(strcmp(pres,"FLAG_SER")))
				msg->flag_ser = atoi(post);
			if(!(strcmp(pres,"FLAG_VIDEO")))
				msg->flag_video = atoi(post);	
			if(!(strcmp(pres,"VIDEO_EXIST")))
				msg->video_exist = atoi(post);
		}
		memset(pres,'\0',100);
		now = fgets(read, 100, fd);
	}
	fclose(fd);
	msg->latitude = 0;
	msg->longitude = 0;
}

int main()
{
	int sock_serfd, sock_clifd;
	struct sockaddr_un ser_addr;
	struct sockaddr_un cli_addr;
	socklen_t ser_len, cli_len;
	int result;
	fd_set rfd,testfd;

	sock_serfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	unlink(UNIX_SERV_PATH);
	ser_addr.sun_family = AF_LOCAL;
	strcpy(ser_addr.sun_path,UNIX_SERV_PATH);
	ser_len = sizeof(ser_addr);

	if((bind(sock_serfd, (struct sockaddr *)&ser_addr, ser_len)) != 0)
	{
		perror("main_control:: bind un error\n");
	}

	listen(sock_serfd,10);

	FD_ZERO(&rfd);
	FD_SET(sock_serfd,&rfd);

	char readbuff[100] = {'\0'};
	char sendbuff[100] = {'\0'};

	linkmsg linkmsg_store, linkmsg_temp;
	int gps_flag = 0;
	regismsg regist_msg;

	config_read(&regist_msg);

	printf("%d, %s, %d, %d, %d, %d, %d, %f, %f\n",regist_msg.nodenum, regist_msg.ip, regist_msg.video_exist,
			regist_msg.flag_ip, regist_msg.flag_gps, regist_msg.flag_ser, regist_msg.flag_video, regist_msg.latitude, regist_msg.longitude);
	int i;
	for(i = 0; i<4; i++)
		process_timer[i].using = 0;

	if(regist_msg.flag_gps == 1)
	{
		system(file_path_process_gps);
		timer_init(&process_timer[0], &callback_func, file_path_process_gps);
	}
	if(regist_msg.flag_ip == 1)
	{
		system(file_path_process_ip);
		timer_init(&process_timer[1], &callback_func, file_path_process_ip);
	}
	if(regist_msg.flag_ser == 1)
	{
		system(file_path_process_serial);
		timer_init(&process_timer[2], &callback_func, file_path_process_serial);
	}
	if(regist_msg.flag_video == 1)
	{
		system(file_path_process_video);
		timer_init(&process_timer[3], &callback_func, file_path_process_video);
	}

	while(1)
	{
		int fd;
		int n_read;
		testfd = rfd;

		timer_timeout();//no heartbeats restart 

		result = select(FD_SETSIZE, &testfd, NULL, NULL, (struct timeval *)0);
		if(result < 1)
		{
			perror("main_control:: server 5\n");
			if(errno == EINTR)
	       		continue;
		}
		for(fd = 0; fd < FD_SETSIZE; fd++)
		{
			if(FD_ISSET(fd,&testfd))
			{
				if(fd == sock_serfd)
				{
					cli_len = sizeof(cli_addr);
					sock_clifd = accept(sock_serfd, (struct sockaddr *)&cli_addr, &cli_len);
					FD_SET(sock_clifd, &rfd);
				}
				else
				{
					ioctl(fd, FIONREAD, &n_read);
					if(n_read == 0)
					{
						close(fd);
						FD_CLR(fd, &rfd);
					}
					else
					{
						read(fd, readbuff, sizeof(linkmsg_temp));
						memcpy(&linkmsg_temp, readbuff, sizeof(linkmsg_temp));
						switch(linkmsg_temp.flag)
						{
							case 1:
								timer_reset(&process_timer[0]);
								printf("main_control:: flag = %d  type = %d (heartbeats is 1, GPS is 2!)\n",linkmsg_temp.flag ,linkmsg_temp.type);
								if(linkmsg_temp.type == 2)
								{
									printf("main_control:: (lat, lon) = %f  %f\n", linkmsg_temp.latitude, linkmsg_temp.longitude);	
									linkmsg_store = linkmsg_temp;
									gps_flag = 1;
									regist_msg.latitude = linkmsg_temp.latitude;
									regist_msg.longitude = linkmsg_temp.longitude;
								}
								else
								{
									gps_flag = 0;
								}
								break;
							case 2:
								timer_reset(&process_timer[1]);
								printf("main_control:: flag = %d\n",linkmsg_temp.flag);
								if(gps_flag == 1)
								{
									memcpy(sendbuff, &linkmsg_store, sizeof(linkmsg_store));
									write(fd, sendbuff, sizeof(linkmsg_store));
								}
									break;
							case 3:
								timer_reset(&process_timer[2]);
								printf("main_control:: flag = %d\n",linkmsg_temp.flag);
								if(gps_flag == 1)
								{
									memcpy(sendbuff, &linkmsg_store, sizeof(linkmsg_store));
									write(fd, sendbuff, sizeof(linkmsg_store));
								}
									break;
							case 4:
								timer_reset(&process_timer[3]);
								printf("main_control:: flag = %d\n",linkmsg_temp.flag);
								break;
							default:
								break;
						}
										//usleep(5*1000);
										sleep(3);
					}
				}
			}
		}
	printf("main_control:: %d, %s, %d, %d, %d, %d, %d, %f, %f\n",regist_msg.nodenum, regist_msg.ip, regist_msg.video_exist,
			regist_msg.flag_ip, regist_msg.flag_gps, regist_msg.flag_ser, regist_msg.flag_video, regist_msg.latitude, regist_msg.longitude);
	}

	return 0;
}
