/////////////////////////////////
//Adventure
//sweetwog.adventure.c
//Author: Garret Sweetwood
//Date: 07/18/2017
//Description:  Based on the the original text based game, Adventure.
//Run sweetwog.buildrooms.c first.
/////////////////////////////////



#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>


#define MAX_ROOMS 7           
#define MAX_ROOM_LENGTH 11     

enum roomType { START_ROOM, END_ROOM, MID_ROOM };

char *timeFileName = "currentTime.txt";

pthread_mutex_t TimeFile_Mutex;


//Room struct to hold a name, an array of 0s and 1s which will determine
//which other rooms it's connected to, and the room type
struct Room {
	char name[30];
	int connection[MAX_ROOMS];
	enum roomType type;
};

//Declarations
int readRooms(struct Room *roomList[], char dirName[], char *roomNames[]);
int playGame(struct Room *roomList[], char *roomNames[]);
void clearMem(struct Room *roomList[]);
char *getDir();
int TimeThread();
void ReadCurrentTimeFile();
void* CreateTmFile();

int main() {

	
	//This will hold the names, it will be referenced when naming connections
	char *roomNames[MAX_ROOMS] = { 0 };

	//The array of pointers that point to each room struct
	struct Room *roomList[MAX_ROOMS];

	
	//Create a string, find the latest directory, write to the string
	char dirName[30]; 
	sprintf(dirName, getDir());
	
	//Using that directory, read the room info and populate the arrays
	readRooms(roomList, dirName, roomNames);

	//Start a game
	playGame(roomList, roomNames);

	//Free the memory that was allocated
	clearMem(roomList);

	return 0;


}


// readRooms(struct Room *roomList[], char dirName[], char *roomNames[])
// Inputs: Room* [], a directory name, and a 


int readRooms(struct Room *roomList[], char dirName[], char *roomNames[]) {

	int i, x;                 // loop iterators
	int roomCount =0;            // number of Room files read so far

	FILE *fp = NULL;          // file pointer
	DIR  *dp = NULL;          // directory pointer
	struct dirent *foundFiles;  // struct that holds directory data

	char fullPathName[50];    // holds the full path to a room file

	struct stat filestat;     // stat() information on a directory entity
	//Files have three columns, the first 2 are identifiers and labels that we can compare
	//and the third is the value we are after
	char block1[MAX_ROOM_LENGTH];
	char block2[MAX_ROOM_LENGTH];
	char blockValue[MAX_ROOM_LENGTH];

	//open directory
	dp = opendir(dirName);
	if (dp == NULL) {
		printf("Error opening %s: errno = %d\n", dirName, errno);
		exit(1);
	}

	

	//This populates the roomNames array using the filenames
	while (((foundFiles = readdir(dp)) != NULL) && (roomCount < MAX_ROOMS)) {
		sprintf(fullPathName, "%s/%s", dirName, foundFiles->d_name);
		stat(fullPathName, &filestat);
		if (S_ISREG(filestat.st_mode) != 0) {
			roomNames[roomCount] = foundFiles->d_name;
			roomCount++;
		}
	}

	for (i = 0; i < MAX_ROOMS; i++) {

		//Print to a string the path to the file, then open
		sprintf(fullPathName, "%s/%s", dirName, roomNames[i]);
		fp = fopen(fullPathName, "r");
		if (fp == NULL) {
			printf("Error opening %s: errno = %d\n", fullPathName, errno);
			exit(1);
		}
		//Allocate memory for a room struct pointer
		struct Room *newRoom = (struct Room *) malloc(sizeof(struct Room));

		//Fill the connection with zeros for now
		for (x = 0; x < MAX_ROOMS; x++) {
			newRoom->connection[x] = 0;
		}
		
		

		
		//Scan the three strings for each line into these strings
		while (fscanf(fp, "%s %s %s", block1, block2, blockValue) != EOF) {

			//Two of the lines start with room, this statement compares to determine which one it is
			//The other line are the connections
			if (strcmp(block1, "ROOM") >= 0) {
				if (strncmp(block2, "NAME", 4) == 0) {
					strcpy(newRoom->name, blockValue);
				}
				else if (strncmp(block2, "TYPE", 4) == 0) {
					if (strncmp(blockValue, "START_ROOM", 10) == 0) {
						newRoom->type = START_ROOM;
					}
					else if (strncmp(blockValue, "MID_ROOM", 8) == 0) {
						newRoom->type = MID_ROOM;
					}
					else if (strncmp(blockValue, "END_ROOM", 8) == 0) {
						newRoom->type = END_ROOM;
					}
				}
			}//loop through the name list each time, mark a 1 if found
			else if (strncmp(block1, "CONNECTION", 10) == 0) {
				for (x = 0; x < MAX_ROOMS; x++) {
					if (strcmp(blockValue, roomNames[x]) == 0) {
						newRoom->connection[x] = 1;
					}
				}
			}
			//room struct is finished, add it to the list
			roomList[i] = newRoom;
		}
		fclose(fp);
	}
	closedir(dp);
	return 0;
}



// playGame
//Inputs: the array of room names and the room structs 
//This is the interface to the game
int playGame(struct Room *roomList[], char *roomNames[]) {

	//an array to store the which rooms were visited
	//set them all equal to -1 to start (not 0 - 6, those will reference a room in roomNames)
	int pathTracker[100];
	int k;
	for (k = 0; k < 100; k++) {
		pathTracker[k] = -1;
	};
	//tracks the amount of steps taken
	int numMoves = 0;
	int i, x;            
	int pos;            	
	int inputLength;    
	
	//string to hold the user input
	char userInput[MAX_ROOM_LENGTH]; 
	int selection = -1;

	//This will act as the active room 
	struct Room *currRoom = NULL;
	
	//Find the start and set currRoom
	i = 0;
	do {
		if (roomList[i]->type == START_ROOM) {
			currRoom = roomList[i];
		}
		i++;
	} while ((currRoom == NULL) && (i < MAX_ROOMS));

	while (currRoom->type != END_ROOM) {

		// Display where you are and what options you can go to
		printf("CURRENT LOCATION: %s\n", currRoom->name);
		printf("POSSIBLE CONNECTIONS:");
		pos = 0;

		//The structs connections array holds 0s and 1s
		//if it is a 1, then there is a connection.  Print that name
		//from the roomNames array.  The indeces are the same.
		for (x = 0; x < MAX_ROOMS; x++) {
			if (currRoom->connection[x] == 1) {
				if (pos == 0) {
					printf(" %s", roomNames[x]);
				}
				else {
					printf(", %s", roomNames[x]);
				}
				pos++;
			}
		}
		printf(".\n");
		//Display the prompt
		printf("WHERE TO? >");
		//Get the string from the user from stdin			
		fgets(userInput, MAX_ROOM_LENGTH, stdin);

		//modify the string with a nul terminator
		inputLength = strlen(userInput) - 1;
		if (userInput[inputLength] == '\n') {
			userInput[inputLength] = '\0';
		}

		//Compare to each of the room names
		for (x = 0; x < MAX_ROOMS; x++) {
			if (strcmp(roomNames[x], userInput) == 0) {
				selection = x;
			}
		}
		//Then check to see if it is connected to the current room		
		if (currRoom->connection[selection] == 1) {
			currRoom = roomList[selection];
			pathTracker[numMoves] = selection;
			numMoves++;
			//Also check to see if the user entered the time command
		} else if (strcmp(userInput, "time") == 0) {
				if (TimeThread() == 1) {
				ReadCurrentTimeFile(); 
			}
		} else {
			//Print this when it doesn't match anything
			printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
		}
		//printf("%s", currRoom->name);  //trace statement
		printf("\n");
	}

	//This is how you "win" the game
	//Print the number of steps and then the path chosen by the user
	if (currRoom->type == END_ROOM) {
		printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
		printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", numMoves);

		// Print out the path that lead to the END ROOM.
		for (i = 0; i < numMoves; i++) {
			printf("%s\n", roomNames[pathTracker[i]]);
		}
	}
	return 0;
}




// getDir()
// Inputs: none
// returns the latest directory that starts with sweetwog.rooms
char *getDir() {
	time_t t;
	int i = 0;
	char *buffer = malloc(sizeof(char) * 64);
	DIR *dirPTR = opendir("./");
	struct dirent* dp;
	struct stat statbuf;

	if (dirPTR != NULL)
	{
		while (dp = readdir(dirPTR)) {
			//Check for the name, check if it is a directory and if it matches my name
			if (stat(dp->d_name, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) &&
				strncmp(dp->d_name, "sweetwog.rooms", 14) == 0) {

				t = statbuf.st_mtime;
				if (t>i) {
					strcpy(buffer, dp->d_name);
					i = t;

				}
			}
		}
		closedir(dirPTR);

	}
	return buffer;
}


void clearMem(struct Room *roomList[]) {

	int i;
	for (i = 0; i < MAX_ROOMS; i++) {
		if (roomList[i] != NULL) {
			free(roomList[i]);
			roomList[i] = 0;
		}
	}
}

// CreateTmFile
void* CreateTmFile()
{
	char TimeStr[256];
	time_t CurrTime;
	struct tm * TimeInfo;
	FILE *TimeFile;

	memset(TimeStr, '\0', sizeof(TimeStr)); // clear time string of garbage.

	time(&CurrTime); 
	TimeInfo = localtime(&CurrTime); 
	strftime(TimeStr, 256, "%I:%M%P %A, %B %d, %Y", TimeInfo); 
	//Create the file with the file name and put in the time string
	TimeFile = fopen(timeFileName, "w");
	fprintf(TimeFile, "%s\n", TimeStr); 
	fclose(TimeFile);

	return NULL;
}

// ReadCurrentTimeFile
// reads the file and prints the contents
void ReadCurrentTimeFile()
{
	char Buffer[256];
	FILE *TimeFile;
	memset(Buffer, '\0', sizeof(Buffer)); 

	TimeFile = fopen(timeFileName, "r"); // readin a file.
	if (TimeFile == NULL) {
		printf("%s was not accessed.\n", timeFileName);
		return;
	}

	//read the time line of the file
	while (fgets(Buffer, 256, TimeFile) != NULL) {
		printf("%s\n", Buffer); 
	}
	fclose(TimeFile);
}


// TimeThread
// creates a seperate thread to write a file 
int TimeThread()
{
	pthread_t Writing_Thread; 
	pthread_mutex_lock(&TimeFile_Mutex); 

	//Create the thread									
	if (pthread_create(&Writing_Thread, NULL, CreateTmFile, NULL) != 0) { // begin running write file function.
		printf("Error from thread!");
		return 0;
	}

	//Unlock the mutex
	pthread_mutex_unlock(&TimeFile_Mutex);
	pthread_join(Writing_Thread, NULL);
	return 1; 
}
