#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'
#define SQUIRREL_IN_TREE '$'
#define EMPTY ' ' /*We can just print as an empty space when printing the world*/


typedef struct {
	int type; /* Wolf, squirrel, tree, ice*/
	int breeding_period;
	int starvation_period;
	int x;
	int y;
	int moved;
} world_cell;

/* Functions */
void initialize_world_array(int size);
void parse_input(char* filename);
void print_world();
void print_world_stats();
void start_world_simulation();
world_cell create_world_cell(int type, int breeding_period, int starvation_period, int x, int y);
void update_world_cell(int i, int j);
world_cell** possible_cells_squirrel(world_cell* cell);
world_cell** possible_cells_wolf(world_cell* cell);
void move(world_cell* cell, world_cell* dest_cell);
int choose_cell(int i, int j, int p);

/* Global Variables */
world_cell **world;
int wolf_breeding_period;
int squirrel_breeding_period;
int wolf_starvation_period;
int number_of_generations;
int grid_size;

int main(int argc, char **argv){

	/* Maybe check for invalid input? */
	wolf_breeding_period = atoi(argv[2]);
	squirrel_breeding_period = atoi(argv[3]);
	wolf_starvation_period = atoi(argv[4]);
	number_of_generations = atoi(argv[5]);
		
	parse_input(argv[1]); /* Filename */
	
	print_world_stats();
	print_world();
	
	start_world_simulation();
	
	return 0;
}

void start_world_simulation(){
	int g = 0, i, j;
	for(; g < number_of_generations; g++){
		printf("---- Generation %d ----\n", g);

		/* update 'red' cells, think chessboard */
		for(i = 0; i < grid_size; i++){
			j = (i % 2) ? 1 : 0;
			for (; j < grid_size; j += 2){
				update_world_cell(i,j);
			}
		}
				
		/* update 'black' cells, think chessboard */
		for(i = 0; i < grid_size; i++){
			j = (i % 2) ? 0 : 1;
			for (; j < grid_size; j += 2){
				update_world_cell(i,j);
			}
		}
		
		for(i = 0; i < grid_size; i++){
			j = 0;
			for (; j < grid_size; j ++){
				world[i][j].moved = 0;
			}
		}
		
		print_world();
	}
}

void move(world_cell* cell, world_cell* dest_cell) {
	dest_cell->moved = 1;
	if(dest_cell->type==TREE){
		/* Squirrel climbing tree */
		dest_cell->type = SQUIRREL_IN_TREE;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = EMPTY;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	} else if((cell->type == SQUIRREL || cell->type == SQUIRREL_IN_TREE) && dest_cell->type == SQUIRREL){
		/* Squirrel moving to squirrel*/
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = EMPTY;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	} else if(cell->type == SQUIRREL_IN_TREE){
		/* Squirrel leaving tree */
		dest_cell->type = SQUIRREL;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = TREE;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	} else if(cell->type == SQUIRREL){
		/* simple Squirrel */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = EMPTY;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	} else if(cell->type == WOLF && dest_cell->type == SQUIRREL){
		/* Wolf eating squirrel */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = EMPTY;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	} else if(cell->type == WOLF && dest_cell->type == WOLF){
		/* Wolf fighting wolf */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = EMPTY;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	} else if(cell->type == WOLF){
		/* simple Wolf */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		cell->type = EMPTY;
		cell->breeding_period = 0;
		cell->starvation_period = 0;
	}
}

int choose_cell(int i, int j, int p){
	int c = i*grid_size + j;
	return c % p;
}

void update_world_cell(int i, int j){
	world_cell *cell = &world[i][j];
	
	world_cell** possible_cells;

	if(cell->moved){
		return;
	}

	/* perfom logic for each cell type */
	switch(cell->type){
		case WOLF: {
				int i = 0, possible_cells_count = 0;
				printf("Checking possible cells for wolf in %d,%d\n", cell->x,cell->y); fflush(stdout);
				possible_cells = possible_cells_wolf(cell);
				for(; i < 4; i++){
					if(possible_cells[i] != NULL){
						possible_cells_count++;
						printf("Possible cell for wolf in %d,%d is %d,%d\n", cell->x,cell->y, possible_cells[i]->x,possible_cells[i]->y);
					} else {
						break;
					}
				}
				
				move(cell, possible_cells[choose_cell(cell->x, cell->y, possible_cells_count)]);
				
				break;
			}
		case SQUIRREL: 
		case SQUIRREL_IN_TREE: {
				int i = 0, possible_cells_count = 0;
				printf("Checking possible cells for squirrel in %d,%d\n", cell->x,cell->y); fflush(stdout);
				possible_cells = possible_cells_squirrel(cell);
				for(; i < 4; i++){
					if(possible_cells[i] != NULL){
						possible_cells_count++;
						printf("Possible cell for squirrel in %d,%d is %d,%d\n", cell->x,cell->y, possible_cells[i]->x,possible_cells[i]->y);
					} else {
						break;
					}
				}
				
				move(cell, possible_cells[choose_cell(cell->x, cell->y, possible_cells_count)]);

				break;
			}
		case ICE:
			break;
		case TREE:
			break;
	}	
}

world_cell** possible_cells_squirrel(world_cell* cell){
	
	world_cell** possible_cells = malloc(4*sizeof(world_cell*)); /*max possible positions*/	
	int i = 0; /*cell counter*/
	world_cell* aux_cell;

	memset(possible_cells, 0, 4*sizeof(world_cell*));
			
	/*check top cell*/
	if(cell->y != 0){
		aux_cell = &world[cell->x][cell->y - 1];
		if(aux_cell->type != WOLF && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	/*check right cell*/
	if(cell->x != grid_size-1){
		aux_cell = &world[cell->x + 1][cell->y];
		if(aux_cell->type != WOLF && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	/*check bottom cell*/
	if(cell->y != grid_size-1){
		aux_cell = &world[cell->x][cell->y + 1];
		if(aux_cell->type != WOLF && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	/*check left cell */
	if(cell->x != 0){
		aux_cell = &world[cell->x - 1][cell->y];
		if(aux_cell->type != WOLF && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	return possible_cells;
}

world_cell** possible_cells_wolf(world_cell* cell){
	
	world_cell** possible_cells = malloc(4*sizeof(world_cell*)); /*max possible positions*/	
	int i = 0; /*cell counter*/
	world_cell* aux_cell;

	memset(possible_cells, 0, 4*sizeof(world_cell*));
	
	/*check top cell*/
	if(cell->y != 0){
		aux_cell = &world[cell->x][cell->y - 1];
		if(aux_cell->type != TREE && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	/*check right cell*/
	if(cell->x != grid_size-1){
		aux_cell = &world[cell->x + 1][cell->y];
		if(aux_cell->type != TREE && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	/*check bottom cell*/
	if(cell->y != grid_size-1){
		aux_cell = &world[cell->x][cell->y + 1];
		if(aux_cell->type != TREE && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	/*check left cell */
	if(cell->x != 0){
		aux_cell = &world[cell->x - 1][cell->y];
		if(aux_cell->type != TREE && aux_cell->type != ICE){
			possible_cells[i] = aux_cell;
			i++;
		}
	}
	
	return possible_cells;
}

void parse_input(char* filename){

	int i, j;
	char type;
	FILE *input;

	if((input = fopen(filename, "r+")) == NULL){
		printf("No such file %s\n", filename);
		exit(1);
	}

	if(fscanf(input, "%d\n", &grid_size) == 0){
		printf("No grid size.");
		exit(2);
	}
	
	/*We only know the world size here*/
	initialize_world_array(grid_size);

	while(fscanf(input,"%d %d %c\n",&i, &j, &type) == 3){ /*All arguments read succesfully*/
		world[i][j] = create_world_cell(type, 0, 0,i,j);
	}

	if(fclose(input) == EOF)
		exit(3);
}

world_cell create_world_cell(int type,int breeding_period,int starvation_period, int x, int y){
		world_cell *cell = (world_cell*)malloc(sizeof(world_cell));
		cell->type = type;
		cell->starvation_period = starvation_period;
		cell->breeding_period = breeding_period;
		cell->x = x;
		cell->y = y;
		cell->moved = 0;
		return *cell;
}

void initialize_world_array(int size){
	int i = 0;
	world = malloc(size * sizeof(world_cell*));
	for(; i < size; i++){
		int j = 0;
		world[i] = malloc(size * sizeof(world_cell*));
		for(; j < size; j++){
			world[i][j] = create_world_cell(EMPTY, EMPTY, EMPTY, i, j);
		}
	}
}

void print_world(){
	int i = 0;
	
	/*print header*/
	printf("  ");
	for(; i < grid_size; i++){
		printf("%d ", i);
	}
	printf("\n");
	
	/*print world*/
	for(i = 0; i < grid_size; i++){
		int j = 0;
		printf("%d|", i);
		for(; j < grid_size; j++){
			printf("%c|", world[j][i].type);
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
