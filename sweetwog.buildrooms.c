/////////////////////////////////
//Adventure
//sweetwog.buildrooms.c
//Author: Garret Sweetwood
//Date: 07/18/2017
//Description:  Based on the the original text based game, Adventure.
/////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define MAX_ROOMS 7           // Total rooms, <= number of names in roomNames[]
#define MAX_ROOM_CHARS 15     // Max number of chars in room names


enum roomType { START_ROOM, END_ROOM, MID_ROOM };   // Room classifications


//basic struct to hold the name, connecton reference, and the type
struct Room {
	char name[30];
	int connection[MAX_ROOMS];
	enum roomType type;
};

//Declarations
int initRooms(struct Room *roomList[], char dirName[], char *roomNames[]);
void clearMem(struct Room *roomList[]);

int main() {

	
	//Hardcoded list of ten possible room names, 7 will be chosen later at random
	char *roomNames[10] = { "Batcave", "Mansion", "GPD", "Arkham", "Streets", "WayneTower", "Rooftops", "Docks", "Sewers", "CityHall" };

	//Holds the room struct pointers
	struct Room *roomList[MAX_ROOMS];

	//This will hold the name of the directory to store the files we create
	char dirName[30];
	sprintf(dirName, "%s.%ld", "sweetwog.rooms", (long)getpid());
	mkdir(dirName, 0755); 
	
	//Create the files and store them into that directory
	initRooms(roomList, dirName, roomNames);
	return 0;
}



int initRooms(struct Room *roomList[], char dirName[], char *roomNames[]) {

	int i, x, r_num; 
	int seed;      
	//an array to keep track which names have been picked
	int roomNamesUsed[10] = { 0 }; 
	int numConnections;                

	//file pointer
	FILE *fp = NULL;             

	char fullPathName[50];       
								 
	seed = time(NULL);
	srand(seed);

	//build the rooms
	for (i = 0; i < MAX_ROOMS; i++) {
		
		do {
			r_num = (rand() % (10));
		} while (roomNamesUsed[r_num] == 1);

		struct Room *tempRoom = (struct Room *) malloc(sizeof(struct Room));

		//Set the name
		strcpy(tempRoom->name, roomNames[r_num]);

		//Name is now used
		roomNamesUsed[r_num] = 1;

		
		//Init the connections array
		for (x = 0; x < MAX_ROOMS; x++) {
			tempRoom->connection[x] = 0;
		}

		tempRoom->type = MID_ROOM;
		roomList[i] = tempRoom;
	}

	// Set the START and END rooms.  They are already random names in random
	// order, so it is not neccessary to set the types randomly.
	roomList[0]->type = START_ROOM;
	roomList[6]->type = END_ROOM;

	
	//sets the connections
	//loops through each room until there is a minimum of 3 connections
	for (i = 0; i < MAX_ROOMS; i++) {

		numConnections = 0; 

		while (numConnections < 3) {
			do {
				r_num = (rand() % (MAX_ROOMS));
			} while (r_num == i);

			//1 means there is a connection, 0 if not
			roomList[i]->connection[r_num] = 1;
			roomList[r_num]->connection[i] = 1;
			numConnections++;
		}
	}

	// Write the files to the directory
	for (i = 0; i < MAX_ROOMS; i++) {
		//create the room file path/name
		sprintf(fullPathName, "%s/%s", dirName, roomList[i]->name);
		fp = fopen(fullPathName, "w");

		//Add the info for each room
		fprintf(fp, "ROOM NAME: %s\n", roomList[i]->name);
		numConnections = 1;
		for (x = 0; x < MAX_ROOMS; x++) {
			if (roomList[i]->connection[x] == 1) {
				fprintf(fp, "CONNECTION %d: %s\n", numConnections, roomList[x]->name);
				numConnections++;
			}
		}

		// Write the Room type to the file.
		//
		fprintf(fp, "ROOM TYPE: ");
		if (roomList[i]->type == START_ROOM) {
			fprintf(fp, "START_ROOM");
		}
		else if (roomList[i]->type == MID_ROOM) {
			fprintf(fp, "MID_ROOM");
		}
		else {
			fprintf(fp, "END_ROOM");
		}
		fprintf(fp, "\n");
		//close
		fclose(fp);
	}

	//Clear the allocated memory
	clearMem(roomList);

	return 0;

}


//clearMem
//Loops through and frees the allocated pointers
void clearMem(struct Room *roomList[]) {

	int i; 
	for (i = 0; i < MAX_ROOMS; i++) {
		if (roomList[i] != NULL) {
			free(roomList[i]);
			roomList[i] = 0;
		}
	}

}

