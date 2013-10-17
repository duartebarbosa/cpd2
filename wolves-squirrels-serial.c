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
void print_world_stats();
void start_world_simulation();
world_cell create_world_cell(int type, int breeding_period, int starvation_period);
void update_world_cell(int i, int j);

// Global Variables
world_cell world[MAX][MAX];
int wolf_breeding_period;
int squirrel_breeding_period;
int wolf_starvation_period;
int number_of_generations;
int grid_size; //We could instead malloc the world with this size, instead of MAX.

int main(int argc, char **argv){

	// Maybe check for invalid input?
	wolf_breeding_period = atoi(argv[2]);
	squirrel_breeding_period = atoi(argv[3]);
	wolf_starvation_period = atoi(argv[4]);
	number_of_generations = atoi(argv[5]);
	
	initialize_world_array();
	
	parse_input(argv[1]); //Filename
	
	print_world_stats();
	print_world();
	
	start_world_simulation();
	
	return 0;
}

void start_world_simulation(){
	int g = 0;
	for(; g < number_of_generations; g++){
		printf("---- Generation %d ----\n", g);
		
		int i,j;
		
		//update 'red' cells, think chessboard
		for(i = 0; i < grid_size; i++){
			if(i % 2 == 0) {// even row
				j = 0;
			} else { //odd row
				j = 1;
			}
			for (; j < grid_size; j += 2){
				update_world_cell(i,j);
			}
		}
				
		//update 'black' cells, think chessboard
		for(i = 0; i < grid_size; i++){
			if(i % 2 == 0) {// even row
				j = 1;
			} else { //odd row
				j = 0;
			}
			for (; j < grid_size; j += 2){
				update_world_cell(i,j);
			}
		}
		
		print_world();
	}
}

void update_world_cell(int i, int j){
	world_cell *cell = &world[i][j];
	
	//perfom logic for each cell type
	switch(cell->type){
		case WOLF:
			//logic goes here
			break;
		case SQUIRREL:
			//logic goes here
			break;
		case ICE:
			//logic goes here
			break;
		case TREE:
			//logic goes here
			break;
	}	
}

void parse_input(char* filename){

	FILE *fp;

	if( (fp = fopen(filename, "r+")) == NULL){
		printf("No such file %s\n", filename);
		exit(1);
	}

	if (fscanf(fp, "%d\n", &grid_size) == 0){
		printf("No grid size.");
		exit(2);
	}
	
	int i, j;
	char type;
	while(fscanf(fp,"%d %d %c\n",&i, &j, &type) == 3){ //All arguments read succesfully  
		world[i][j] = create_world_cell(type, 0, 0);
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
	int i = 0;
	for(; i < MAX; i++){
		int j = 0;
		for(; j < MAX; j++){
			world[i][j] = create_world_cell(EMPTY, EMPTY, EMPTY);
		}
	}
}

void print_world(){
	int i = 0;
	
	//print header
	printf("  ");
	for(; i < grid_size; i++){
		printf("%d ", i);
	}
	printf("\n");
	
	//print world
	for(i = 0; i < grid_size; i++){
		int j = 0;
		printf("%d|", i);
		for(; j < grid_size; j++){
			printf("%c|", world[i][j].type);
		}
		printf("\n");
	}
}

void print_world_stats(){
	printf("Grid size: %d\n", grid_size);
	printf("Wolf breeding period: %d\n", wolf_breeding_period);
	printf("Wolf starvation period: %d\n", wolf_starvation_period);
	printf("Squirrel breeding period: %d\n", squirrel_breeding_period);
	printf("Number of generations: %d\n", number_of_generations);
}
