#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'
#define SQUIRREL_IN_TREE '$'
#define EMPTY ' ' /*We can just print as an empty space when printing the world*/

#define MAX(a, b) ((a) > (b) ? (a) : (b))


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
world_cell** retrieve_possible_cells(world_cell* cell);
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
		printf("---- Generation %d ----\n", g+1);

		/* update 'red' cells, think chessboard */
		for(i = 0; i < grid_size; i++){
			for (j = (i % 2) ? 1 : 0; j < grid_size; j += 2){
				update_world_cell(i,j);
			}
		}
				
		/* update 'black' cells, think chessboard */
		for(i = 0; i < grid_size; i++){
			for (j = (i % 2) ? 0 : 1; j < grid_size; j += 2){
				update_world_cell(i,j);
			}
		}
		
		for(i = 0; i < grid_size; i++){
			for (j = 0; j < grid_size; j ++){
				world[i][j].moved = 0;
			}
		}
		
		print_world();
	}
}

void cleanup_cell(world_cell* cell){
	cell->type = EMPTY;
	cell->breeding_period = 0;
	cell->starvation_period = 0;
}

void move(world_cell* cell, world_cell* dest_cell) {
	dest_cell->moved = 1;
	if(dest_cell->type==TREE){
		/* Squirrel climbing tree */
		dest_cell->type = SQUIRREL_IN_TREE;
		dest_cell->breeding_period = cell->breeding_period+1;
		dest_cell->starvation_period = cell->starvation_period;

		/* clean cell */		
		cleanup_cell(cell);
	} else if((cell->type == SQUIRREL || cell->type == SQUIRREL_IN_TREE) && dest_cell->type == SQUIRREL){
		/* Squirrel moving to squirrel*/
		dest_cell->type = cell->type;
		dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period)+1; /*FIXME: Should we increment the breeding period? */
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */		
		cleanup_cell(cell);
	} else if(dest_cell->type == SQUIRREL_IN_TREE){
		/* squirrel eating squirrel on tree */
		dest_cell->type = SQUIRREL_IN_TREE;
		dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period)+1; /*FIXME: Should we increment the breeding period? */
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */		
		cleanup_cell(cell);
	} else if(cell->type == SQUIRREL_IN_TREE){
		/* Squirrel leaving tree */
		dest_cell->type = SQUIRREL;
		dest_cell->breeding_period = cell->breeding_period+1;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell */
		if(dest_cell->breeding_period >= squirrel_breeding_period){
			cell->type = SQUIRREL_IN_TREE;
			dest_cell->breeding_period = 0;
		} else {
			cell->type = TREE;
		}
		
		cell->breeding_period = 0;
		cell->starvation_period = 0;
		
	} else if(cell->type == SQUIRREL){
		/* simple Squirrel */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period + 1;
		dest_cell->starvation_period = cell->starvation_period;
		
		/* clean cell or reproduce*/
		if(dest_cell->breeding_period >= squirrel_breeding_period){
			cell->type = SQUIRREL;
			dest_cell->breeding_period = 0;
		} else {
			cell->type = EMPTY;
		}
		
		cell->breeding_period = 0;
		cell->starvation_period = 0;	

	} else if(cell->type == WOLF && dest_cell->type == SQUIRREL){
		/* Wolf eating squirrel */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period;
		dest_cell->starvation_period += wolf_starvation_period * 1; /* Number of squirrels eaten? */
		
		/* clean cell */		
		cleanup_cell(cell);
	} else if(cell->type == WOLF && dest_cell->type == WOLF){
		/* Wolf fighting wolf */
		dest_cell->type = cell->type;
		
		if(cell->starvation_period == dest_cell->starvation_period){
			dest_cell->breeding_period = (cell->breeding_period > dest_cell->breeding_period ? cell->breeding_period : dest_cell->breeding_period)+1; /*FIXME: Should we increment the breeding period? */
			/* same starvation */
		} else {
			dest_cell->breeding_period = (cell->starvation_period > dest_cell->starvation_period ? cell->breeding_period : dest_cell->breeding_period)+1; /*FIXME: Should we increment the breeding period? */
			dest_cell->starvation_period = (cell->starvation_period > dest_cell->starvation_period ? cell->starvation_period : dest_cell->starvation_period)+1;
		}
		
		/* clean cell */		
		cleanup_cell(cell);
	} else if(cell->type == WOLF){
		/* simple Wolf */
		dest_cell->type = cell->type;
		dest_cell->breeding_period = cell->breeding_period+1;
		dest_cell->starvation_period = cell->starvation_period-1;
		
		/* clean cell or reproduce*/
		if(dest_cell->breeding_period >= wolf_breeding_period){
			cell->type = WOLF;
			cell->breeding_period = 0;
			cell->starvation_period = wolf_starvation_period;
		} else {
			cleanup_cell(cell);
		}

	} else {
		exit(3); 
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
				int i = 0, possible_cells_count = 0, squirrels_found = 0;
				world_cell** squirrel_cells  = malloc(4*sizeof(world_cell*));

				printf("Checking possible cells for wolf in %d,%d\n", cell->x,cell->y);
				possible_cells = retrieve_possible_cells(cell);
				for(; i < 4; i++){
					if(possible_cells[i] != NULL){
						printf("Possible cell for wolf in %d,%d is %d,%d\n", cell->x,cell->y, possible_cells[i]->x,possible_cells[i]->y);

						if(possible_cells[i]->type == SQUIRREL){
							squirrel_cells[squirrels_found++] = possible_cells[i];
						}
						
						possible_cells_count++;
					} else {
						break;
					}
				}
				
				if(squirrels_found > 0){
					move(cell, squirrel_cells[choose_cell(cell->x, cell->y, squirrels_found)]);
				} else {
					move(cell, possible_cells[choose_cell(cell->x, cell->y, possible_cells_count)]);
				}
			
				break;
			}
		case SQUIRREL: 
		case SQUIRREL_IN_TREE: {
				int i = 0, possible_cells_count = 0;
				printf("Checking possible cells for squirrel in %d,%d\n", cell->x,cell->y);
				possible_cells = retrieve_possible_cells(cell);
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
		case TREE:
		default:
			break;
	}	
}

void add_cell(world_cell* aux_cell, world_cell** possible_cells, int bad_type){
	if(aux_cell->type != bad_type && aux_cell->type != ICE){
		*possible_cells = aux_cell;
	}
}

world_cell** retrieve_possible_cells(world_cell* cell){
	
	world_cell** possible_cells = malloc(4*sizeof(world_cell*)); /*max possible positions*/
	world_cell** tmp_cell = possible_cells;
	int bad_type;

	memset(possible_cells, 0, 4 * sizeof(world_cell*));

	if(cell->type == SQUIRREL)
		bad_type = WOLF;
	else
		bad_type = TREE;

	/*check top cell*/
	if(cell->y != 0)
		add_cell(&world[cell->x][cell->y - 1], tmp_cell++, bad_type);
	
	/*check right cell*/
	if(cell->x != grid_size-1)
		add_cell(&world[cell->x + 1][cell->y], tmp_cell++, bad_type);
	
	/*check bottom cell*/
	if(cell->y != grid_size-1)
		add_cell(&world[cell->x][cell->y + 1], tmp_cell++, bad_type);
	
	/*check left cell */
	if(cell->x != 0)
		add_cell(&world[cell->x - 1][cell->y], tmp_cell++, bad_type);
	
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
		world[i][j].type = type;
		/* FIXME: should we initialize the cells with the default breeding and starvation periods? previous = create_world_cell(type, 0, 0,i,j);*/
	}

	if(fclose(input) == EOF)
		exit(3);
}

world_cell create_world_cell(int type,int breeding_period,int starvation_period, int x, int y){
		world_cell *cell = malloc(sizeof(world_cell));
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
			world[i][j] = create_world_cell(EMPTY, 0, 0, i, j);
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
