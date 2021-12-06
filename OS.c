#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{

    int A, B, C;
	char buffer[100];

	//create unnamed pipe
	int fd[3][2];
    int i;
    for(i=0; i<3; i++){
        if(pipe(fd[i]) < 0){
            printf("Unable to create a pipe; errno=%d\n",errno);
            exit(-1);
        }
    }

	if(mkfifo("myfifo1", 0777) == -1){
		if(errno !=EEXIST){
			printf("Could not create fifo1 file");
			exit(-1);
		}
	}

	if(mkfifo("myfifo2", 0777) == -1){
		if(errno !=EEXIST){
			printf("Could not create fifo2 file");
			exit(-1);
		}
	}

	if(mkfifo("myfifo3", 0777) == -1){
        if(errno !=EEXIST){
            printf("Could not create fifo3 file");
            exit(-1);
        }
	}


	char path[100];
	//receive image from the input
	printf("Enter file path: ");
	scanf("%s", path);

	FILE* fp = fopen(path, "rb");//Read the image.bmp file in the same directory.
	if(fp == NULL)
	{
		printf("Failed to open'%s'!\n", path);
		return -1;
	}
	printf("\nloaded image '%s' is successfully. \n", path);
	fclose(fp);
	


    A = fork();
    if(A < 0){
        return 4;
    }
    if(A == 0){
		// printf("child[1] --> pid = %d and ppid = %d\n",
		// 	getpid(), getppid());
		sleep(3);
		close(fd[0][0]);
		close(fd[0][1]);
		close(fd[1][0]);
		close(fd[1][1]);
		close(fd[2][0]);
		close(fd[2][1]);

		char *args[] = {"./histogram_calculator", NULL};
        execv(args[0], args);
        fprintf(stderr, "Failed to execute %s\n", args[0]);
		exit(EXIT_FAILURE);

        return 0;
    }
    else {
        B = fork();
        if (B == 0) {
			// printf("child[2] --> pid = %d and ppid = %d\n",
			// 	getpid(), getppid());
			sleep(2);
			close(fd[0][0]);
			close(fd[0][1]);
			close(fd[1][0]);
			close(fd[1][1]);
			close(fd[2][0]);
			close(fd[2][1]);
			
			char *args[] = {"./filtering", NULL};
            execv(args[0], args);
            fprintf(stderr, "Failed to execute %s\n", args[0]);
			exit(EXIT_FAILURE);

			return 0;
		}
		else {
			C = fork();
			if (C == 0) {
				// printf("child[3] --> pid = %d and ppid = %d\n",
				// 	getpid(), getppid());
				sleep(1);
				close(fd[0][1]);
				close(fd[1][0]);
				close(fd[2][0]);

				/////////////////Unnamed pipe: received image address from parent to child(C)
				int n;
				if ((n = read ( fd[0][0], buffer, sizeof(buffer) ) ) >= 0) {
					buffer[n] = 0;  //terminate the string
					printf("[CHILD C] RECIVED the image address from parent: %s\n", buffer);
				} else {
					perror("Read did not return expected value\n");
					return 5;
				}
				close(fd[0][0]);

				/////////////////named pip : send the image address to histogram_calculator
				printf("Opening fifo file ....\n");
				int writefd1 = open("myfifo1", O_WRONLY);
				//printf("Opened fifo file\n");
				if (write(writefd1, buffer, strlen(buffer)+1) == -1){
					return 6;
				}
				//printf("Written fifo file\n");
				close(writefd1);
				printf("Closed fifo file\n");

				////////////////named pip : send the image address to histogram_calculator
				printf("Opening fifo2 file ....\n");
				int writefd2 = open("myfifo2", O_WRONLY );
				//printf("Opened fifo2 file\n");
				if (write(writefd2, buffer, strlen(buffer)+1) == -1){
					return 7;
				}
				//printf("Written fifo2 file\n");
				close(writefd2);
				printf("Closed fifo2 file\n");

				///////////////named pipe: read histogram 
				int readfd;
				int readbuf[256];
				int read_bytes;  

				readfd = open("myfifo3", O_RDONLY);
				printf("\nopened read fifo3\n");
				printf("\nC Received histogram from child A: (The number of each pixel values of  range(0-255))\n");
				for (int i = 0; i < 256; i++){
					read_bytes = read(readfd, readbuf, sizeof(readbuf));
					readbuf[read_bytes] = '\0';
					//printf("%d ", readbuf[i]);
					//printf("C Received histogram: \"%d\" and length is %d\n", readbuf[i], sizeof(readbuf)*256);

					/////////Unnamed pipe: send histogram from child(C) to parent
					if(write(fd[1][1], readbuf, sizeof(readbuf))<0){
						printf("Write did not return expected value\n");
						return 8;
					}
				}
				printf("\n");
				close(readfd);    ///close named pipe: histogram from A to C
				close(fd[1][1]); //close unnamed pipe: histogram from C to Parent
				unlink("myfifo3");
				printf("\nclosed read fifo3\n");

				///////////named pipe: read output image address from child B
				int readB;
				char readimg[100];
				int read_bytes_img;  

				readB = open("myfifo4", O_RDONLY);
				read_bytes_img = read(readB, readimg, sizeof(readimg));
				readimg[read_bytes_img] = '\0';
				printf("C Received output iamge address: \"%s\" and length is %d\n", readimg, (int)strlen(readimg));
				
				close(readB); /// close named pipe B to C
				unlink("myfifo4");
				
				/////////Unnamed pipe: send histogram from child(C) to parent
				if(write(fd[2][1], readimg, sizeof(readimg))<0){
					printf("Write did not return expected value\n");
					return 9;
				}
				close(fd[2][1]);

				////////////////////////

				return 0;
			}
            else {
				//PARENT
				//printf("parent --> pid = %d\n", getpid());
				close(fd[0][0]); //close read mode
				close(fd[1][1]);
				close(fd[2][1]);
				
				//Unnamed pipe: send image address from parent to child(C)
				if(write(fd[0][1], path, strlen(path)+1)<0){
					printf("Write did not return expected value\n");
					return 1;
				}
				printf("[parent] send image address '%s' to child C \n", path);
				close(fd[0][1]); //close write mode //unnamed pipe p to c

				//////Unnamed pipe: received histogram from child(C)
				int readbuf[256];
				int n;
				for (int i = 0; i < 256; i++){
					if ((n = read ( fd[1][0], readbuf, sizeof(readbuf) ) ) >= 0) {
						//printf("%d ", readbuf[i]);
						
					} else {
						perror("Read did not return expected value\n");
						return 2;
					}
				}
				printf("\n[Parent ]Received histogram from child C\n");
				close(fd[1][0]); //close unnamed pip: histogram from C to Parent

				//////////////////unnamed pipe: received output image address from child(C)
				int n1;
				if ((n1 = read ( fd[2][0], buffer, sizeof(buffer) ) ) >= 0) {
					buffer[n1] = 0;  //terminate the string
					printf("[parent] RECIVE output image address '%s' from C\n", buffer);
				} else {
					perror("Read did not return expected value\n");
					return 3;
				}
				close(fd[2][0]);



				waitpid(A, NULL, 0);
				waitpid(B, NULL, 0);
				waitpid(C, NULL, 0);

				return 0;

			}
    	}

    }


    return 0;
}
