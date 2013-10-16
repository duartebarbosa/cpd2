#include <stdio.h>
#include <stdlib.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'
#define EMPTY ' ' //We can just print as an empty space when printing the world

#define MAX 10 //Not sure if required

typedef struct _world_cell {
	int type; /* Wolf, squirrel, tree, ice*/
	int breeding_period;
	int starvation_period;
} world_cell;

// Functions
void initialize_world_array();
void parse_input(char* filename);
void print_world();
world_cell create_world_cell(int type, int breeding_period, int starvation_period);

// Global Variables
world_cell world[MAX][MAX];
int wolf_breeding_period;
int squirrel_breeding_period;
int wolf_starvation_period;
int number_of_generations;
int grid_size; //We could instead malloc the world with this size, instead of MAX.

int main(int argc, char **argv)
{
	// Maybe check for invalid input?
	wolf_breeding_period = atoi(argv[2]);
	squirrel_breeding_period = atoi(argv[3]);
	wolf_starvation_period = atoi(argv[4]);
	number_of_generations = atoi(argv[5]);
	
	initialize_world_array();
	parse_input(argv[1]); //Filename
	print_world();
	
	return 0;
}

void parse_input(char* filename){

	FILE *fp;

	if( (fp = fopen(filename, "r+")) == NULL)
	{
		printf("No such file %s\n", filename);
		exit(1);
	}  

	if (fp == NULL)
	{
		printf("Error reading file %s\n", filename);
	}

	if (fscanf(fp, "%d\n", &grid_size) == 0){
		printf("No grid size.");
		exit(1);
	}
	
	int i, j;
	char type;
	while(fscanf(fp,"%d %d %c\n",&i, &j, &type) == 3) //All arguments read succesfully  
	{  
		   int starvation = EMPTY, breeding = EMPTY;
		   switch(type){
				case WOLF:
					starvation = wolf_starvation_period;
					breeding = wolf_breeding_period;
					break;
				case SQUIRREL:
					//Squirrels have no starvation
					breeding = squirrel_breeding_period;
					break;
				case ICE:
					//No starvation nor breeding
					break;
				case TREE:
					//No starvation nor breeding
					break;
		   }
		   world[i][j] = create_world_cell(type, breeding, starvation);
	}  
}

world_cell create_world_cell(int type,int breeding_period,int starvation_period){
		world_cell *cell = (world_cell*)malloc(sizeof(world_cell));
		cell->type = type;
		cell->starvation_period = starvation_period;
		cell->breeding_period = breeding_period;
		
		return *cell;
}

void initialize_world_array(){
	int i;
	for(i = 0; i < MAX; i++){
		int j;
		for(j = 0; j < MAX; j++){
			world[i][j] = create_world_cell(EMPTY, EMPTY, EMPTY);
		}
	}
}

void print_world(){
	printf("%d\n", grid_size);
	printf("Wolf breeding period: %d\n", wolf_breeding_period);
	printf("Wolf starvation period: %d\n", wolf_starvation_period);
	printf("Squirrel breeding period: %d\n", squirrel_breeding_period);
	printf("Number of generations: %d\n", number_of_generations);
	int i;
	for(i = 0; i < grid_size; i++){
		int j;
		for(j = 0; j < grid_size; j++){
			printf("%c ", world[i][j].type);
		}
		printf("\n");
	}
}
