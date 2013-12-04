#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'
#define SQUIRREL_IN_TREE '$'
#define EMPTY ' '

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX_BREED(a, b) MAX(a->breeding_period, b->breeding_period)
#define MAX_STARV(a, b) MAX(a->starvation_period, b->starvation_period)

#define GET_X(cell) cell->number/grid_size
#define GET_Y(cell) cell->number%grid_size
#define CHOOSE_CELL(cell, p) cell->number%p

typedef struct {
	char type; /* Wolf, squirrel, squirrel in tree, tree, ice, empty */
	unsigned short breeding_period;
	unsigned short starvation_period;
	unsigned int number;
	char moved;
} world_cell;

/* Global Variables */
world_cell **world;
world_cell **world_previous;
unsigned short wolf_breeding_period;
unsigned short squirrel_breeding_period;
unsigned short wolf_starvation_period;
unsigned int number_of_generations;
unsigned short grid_size;

void initialize_world_array(){
	register unsigned short i = 0;
	world = malloc(grid_size * sizeof(world_cell*));
	world_previous = malloc(grid_size * sizeof(world_cell*));

	for(; i < grid_size; ++i){
		unsigned short j = 0, partial_number = i * grid_size;
		world[i] = calloc(grid_size, sizeof(world_cell));
		world_previous[i] = calloc(grid_size, sizeof(world_cell));

		for(; j < grid_size; ++j){
			world[i][j].type = world_previous[i][j].type = EMPTY;
			world[i][j].number = world_previous[i][j].number = partial_number + j;
		}
	}
}

void parse_input(char* filename){
	unsigned short i, j;
	char type;
	FILE *input;

	if((input = fopen(filename, "r+")) == NULL){
		printf("No such file %s\n", filename);
		exit(1);
	}

	if(!fscanf(input, "%hu\n", &grid_size)){
		printf("No grid size.");
		exit(2);
	}

	initialize_world_array();

	while(fscanf(input,"%hu %hu %c\n",&i, &j, &type) == 3){ /*All arguments read succesfully*/
		world[i][j].type = type;
		if(type == WOLF)
			world[i][j].starvation_period = wolf_starvation_period;
	}

	if(fclose(input) == EOF)
		exit(3);
}

void cleanup_cell(world_cell* cell){
	cell->type = EMPTY;
	cell->breeding_period = cell->starvation_period = 0;
}

void move_wolf(world_cell* cell, world_cell* dest_cell) {

	dest_cell->moved = 1;
	switch(dest_cell->type){
		case SQUIRREL:
			/* Wolf eating squirrel */
			dest_cell->type = cell->type;
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->starvation_period = wolf_starvation_period; 
			
			/* clean cell */
			cleanup_cell(cell);
			break;
		case WOLF:
			/* Wolf fighting wolf */
			dest_cell->type = cell->type;
			/* same starvation */			
			if(cell->starvation_period == dest_cell->starvation_period){
				dest_cell->breeding_period = MAX_BREED(cell, dest_cell);
			} else {
				dest_cell->breeding_period = (cell->starvation_period > dest_cell->starvation_period ? cell->breeding_period : dest_cell->breeding_period);
				dest_cell->starvation_period = MAX_STARV(cell, dest_cell);
			}
			
			/* clean cell */
			cleanup_cell(cell);
			break;
		default:
			if(dest_cell-> type != EMPTY){
				printf("Shouldn't happen, wolf moving to: %c\n", dest_cell->type); /* why the hell is this throwin' up wolfs?! */
			}
			/* simple Wolf */
			dest_cell->type = cell->type;
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->starvation_period = cell->starvation_period;
			
			/* clean cell or reproduce*/
			if(dest_cell->breeding_period >= wolf_breeding_period && dest_cell->starvation_period > 1){
				cell->type = WOLF;
				cell->breeding_period = dest_cell->breeding_period = 0;
				cell->starvation_period = wolf_starvation_period;
			} else
				cleanup_cell(cell);
	}
}

void move_squirrel(world_cell* cell, world_cell* dest_cell) {
	dest_cell->moved = 1;
	switch(dest_cell->type){
		case TREE:
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->type = SQUIRREL_IN_TREE;

			if(cell->type == SQUIRREL_IN_TREE){
				cell->type = TREE;
			} else {
				cleanup_cell(cell);
			}

			break;
		case SQUIRREL:
			/* Squirrel moving to squirrel*/
			
			dest_cell->breeding_period = MAX_BREED(cell, dest_cell);

			if(cell->type == SQUIRREL_IN_TREE){
				dest_cell->type = SQUIRREL;
				cell->type = TREE;
			} else {
				dest_cell->type = cell->type;
				cleanup_cell(cell);
			}
			
			break;
		case SQUIRREL_IN_TREE:
			dest_cell->type = SQUIRREL_IN_TREE;
			dest_cell->breeding_period = MAX_BREED(cell, dest_cell);

			if(cell->type == SQUIRREL_IN_TREE){
				cell->type = TREE;
			} else {
				cleanup_cell(cell);
			}

			break;
		case WOLF:
			/* Wolf eating squirrel */
			dest_cell->starvation_period = wolf_starvation_period; 
			
			if(cell->type == SQUIRREL_IN_TREE){
				cell->type = TREE;
			} else {
				cleanup_cell(cell);
			}

			break;
		default:
			if(dest_cell-> type != EMPTY){
				printf("Shouldn't happen, squirrel moving to: %c\n", dest_cell->type);
			}
			if(cell->type == SQUIRREL_IN_TREE){
				/* Squirrel leaving tree */
				dest_cell->type = SQUIRREL;
				dest_cell->breeding_period = cell->breeding_period;

				/* clean cell */
				cleanup_cell(cell);
				if(dest_cell->breeding_period >= squirrel_breeding_period){
					cell->type = SQUIRREL_IN_TREE;
					dest_cell->breeding_period = 0;
				} else
					cell->type = TREE;
			} else { /* destination is empty */
				/* simple Squirrel */
				dest_cell->type = cell->type;
				dest_cell->breeding_period = cell->breeding_period;

				/* clean cell or reproduce*/
				cleanup_cell(cell);
				if(dest_cell->breeding_period >= squirrel_breeding_period){
					cell->type = SQUIRREL;
					dest_cell->breeding_period = 0;
				} else
					cell->type = EMPTY;
			}
	}
}

char add_cell(world_cell* aux_cell, world_cell** possible_cells, char bad_type){
	if(aux_cell->type != bad_type && aux_cell->type != ICE && aux_cell->type != SQUIRREL_IN_TREE && aux_cell->type != WOLF){
		*possible_cells = &world_previous[GET_X(aux_cell)][GET_Y(aux_cell)];
		return 1;
	}
	return 0;
}

world_cell** retrieve_possible_cells(world_cell* cell){
	
	world_cell** possible_cells = calloc(4, sizeof(world_cell*)); /* 4: max possible positions */
	world_cell** tmp_cell = possible_cells;
	char bad_type = 0;
	unsigned short x = GET_X(cell), y = GET_Y(cell);

	if(cell->type == WOLF)
		bad_type = TREE;
	else /* Squirrels and squirrels in tree */
		bad_type = SQUIRREL;

	/*check top cell*/
	if(x && add_cell(&world_previous[x-1][y], tmp_cell, bad_type))
		++tmp_cell;
	
	/*check right cell*/
	if(y != grid_size-1 && add_cell(&world_previous[x][y+1], tmp_cell, bad_type))
		++tmp_cell;
	
	/*check bottom cell*/
	if(x != grid_size-1 && add_cell(&world_previous[x+1][y], tmp_cell, bad_type))
		++tmp_cell;
	
	/*check left cell */
	if(y && add_cell(&world_previous[x][y-1], tmp_cell, bad_type))
		++tmp_cell;
	
	return possible_cells;
}

void update_world_cell(unsigned short x, unsigned short y){
	world_cell *cell = &world[x][y];
	world_cell** possible_cells;
	int count = 0;

	/* perfom logic for each cell type */
	switch(cell->type){
		case EMPTY:
			break;
		case WOLF: {
				int squirrels_found = 0;
				world_cell** squirrel_cells = malloc(4 * sizeof(world_cell*));

	
				possible_cells = retrieve_possible_cells(cell);
				for(; count < 4 && possible_cells[count] != NULL; ++count){
					if(possible_cells[count]->type == SQUIRREL)
						squirrel_cells[squirrels_found++] = possible_cells[count];
				}
				
				if(squirrels_found){
					world_cell* prev_gen_dest_cell = squirrel_cells[CHOOSE_CELL(cell, squirrels_found)];
					move_wolf(cell, &world[GET_X(prev_gen_dest_cell)][GET_Y(prev_gen_dest_cell)]);
				} else if (count) {
					world_cell* prev_gen_dest_cell = possible_cells[CHOOSE_CELL(cell, count--)];
					move_wolf(cell, &world[GET_X(prev_gen_dest_cell)][GET_Y(prev_gen_dest_cell)]);
				}
				
				free(squirrel_cells);
				free(possible_cells);
				break;
			}
		case SQUIRREL: 
		case SQUIRREL_IN_TREE:
			possible_cells = retrieve_possible_cells(cell);
			for(; count < 4 && possible_cells[count] != NULL; ++count);

			if (count) {
				world_cell* prev_gen_dest_cell = possible_cells[CHOOSE_CELL(cell, count--)];
				move_squirrel(cell, &world[GET_X(prev_gen_dest_cell)][GET_Y(prev_gen_dest_cell)]);
			}
	
			free(possible_cells);
			break;
		default:	/* we assume that trees and ice are less frequent than the animals */
			break;
	}
}

void print_grid(world_cell ** world){
	register int i = 0;

	/*print header*/
	printf("  ");
	for(; i < grid_size; ++i)
		printf("%d ", i);

	printf("\n");

	/*print world*/
	for(i = 0; i < grid_size; ++i){
		int j = 0;
		printf("%d|", i);
		for(; j < grid_size; ++j)
			printf("%c|", world[i][j].type);

		printf("\n");
	}
}

void copy_world(void){
	register int i;
	for(i = 0; i < grid_size; ++i)
		memcpy(world_previous[i], world[i], grid_size*sizeof(world_cell));
}

void print_world(){
	int i = 0, j;
	for(; i < grid_size; ++i){
		for (j = 0; j < grid_size; ++j){
			if(world[i][j].type != EMPTY)
				printf("%d %d %c\n", i, j, world[i][j].type);
		}
	}
}
void start_world_simulation(void){
	register int i, j;
	for(; number_of_generations > 0; --number_of_generations){
		copy_world();

		/* update 'red' cells, think chessboard */
		for(i = 0; i < grid_size; ++i)
			for (j = i & 1; j < grid_size; j += 2)
				update_world_cell(i, j);

		copy_world();

		/* update 'black' cells, think chessboard */
		for(i = 0; i < grid_size; ++i)
			for (j = !(i & 1); j < grid_size; j += 2)
				update_world_cell(i, j);

		if(number_of_generations == 1)
			return;

		for(i = 0; i < grid_size; ++i){
			for (j = 0; j < grid_size; ++j){
				if (world[i][j].moved){
					if (world[i][j].type == SQUIRREL){
						world[i][j].breeding_period++;
					} else if (world[i][j].type == WOLF){
						world[i][j].starvation_period--;
						world[i][j].breeding_period++;
						/* wolf dies of starvation */
						if(world[i][j].starvation_period <= 0){
							cleanup_cell(&world[i][j]);
						}
					}
				}
				world[i][j].moved = 0;
			}
		}	
	}
	
}


void freemem(void){
	register unsigned short i = 0;

	for(; i < grid_size; ++i){
		free(world[i]);
		free(world_previous[i]);
	}

	free(world);
	free(world_previous);
}

int main(int argc, char **argv){

	#ifdef GETTIME
    double start = omp_get_wtime();
    #endif
        
	/* Maybe check for invalid input? */
	wolf_breeding_period = atoi(argv[2]);
	squirrel_breeding_period = atoi(argv[3]);
	wolf_starvation_period = atoi(argv[4]);
	number_of_generations = atoi(argv[5]);
		
	parse_input(argv[1]); /* Filename */

	start_world_simulation();

	print_world();

	#ifdef GETTIME
    printf("OpenMP time: %fs\n", omp_get_wtime() - start);
    #endif
    
    freemem();

	return 0;
}
