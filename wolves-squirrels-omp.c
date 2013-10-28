#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'
#define SQUIRREL_IN_TREE '$'
#define EMPTY ' ' /*We can just print as an empty space when printing the world*/

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
	char type; /* Wolf, squirrel, squirrel in tree, tree, ice, empty */
	unsigned short breeding_period;
	unsigned short starvation_period;
	unsigned short x;
	unsigned short y;
	char moved;
} world_cell;

/* Global Variables */
world_cell **world;
world_cell **world_previous;
unsigned short wolf_breeding_period;
unsigned short squirrel_breeding_period;
unsigned short wolf_starvation_period;
unsigned short number_of_generations;
unsigned short grid_size;

void cleanup_cell(world_cell* cell){
	cell->type = EMPTY;
	cell->breeding_period = cell->starvation_period = 0;
}

void create_world_cell(world_cell *cell, char type, unsigned short breeding_period, unsigned short starvation_period, unsigned short x, unsigned short y){
	cell->type = type;
	cell->breeding_period = breeding_period;
	cell->starvation_period = starvation_period;
	cell->x = x;
	cell->y = y;
}

void move_wolf(world_cell* cell, world_cell* dest_cell) {
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
				/*printf("Wolves fighting wih same starvation!\n");*/
				dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period);
				/*printf("New wolf: %d\n", dest_cell->starvation_period);*/
			} else {
				/*printf("Wolves fighting! Wolf 1 (%d,%d) has %d and wolf 2 (%d,%d) has %d\n", cell->starvation_period, cell->x, cell->y, dest_cell->starvation_period, dest_cell->x, dest_cell->y);*/
				dest_cell->breeding_period = (cell->starvation_period > dest_cell->starvation_period ? cell->breeding_period : dest_cell->breeding_period); /*FIXME: Should we increment the breeding period? */
				dest_cell->starvation_period = MAX(cell->starvation_period, dest_cell->starvation_period);
				/*printf("New wolf: %d\n", dest_cell->starvation_period);*/
			}
			
			/* clean cell */		
			cleanup_cell(cell);
			break;
		default:
			/* simple Wolf */
			dest_cell->type = cell->type;
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->starvation_period = cell->starvation_period;
			
			/* clean cell or reproduce*/
			if(dest_cell->breeding_period >= wolf_breeding_period){
				cell->type = WOLF;
				cell->breeding_period = 0;
				cell->starvation_period = wolf_starvation_period;
				dest_cell->breeding_period = 0;
				/*printf("Left a new wolf on %d %d, from wolf now on %d %d (%c)\n", cell->x, cell->y, dest_cell->x, dest_cell->y, dest_cell->type);*/
			} else
				cleanup_cell(cell);
	}
}

void move_squirrel(world_cell* cell, world_cell* dest_cell) {
	dest_cell->starvation_period = cell->starvation_period;
	if(dest_cell->type == TREE){
		/* Squirrel climbing tree */
		dest_cell->type = SQUIRREL_IN_TREE;
		dest_cell->breeding_period = cell->breeding_period;

		/* clean cell */
		cleanup_cell(cell);
	} else if(dest_cell->type == SQUIRREL){
		/* Squirrel moving to squirrel*/
		dest_cell->type = cell->type;
		dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period); /*FIXME: Should we increment the breeding period? */
		
		/* clean cell */
		cleanup_cell(cell);
	} else if(dest_cell->type == SQUIRREL_IN_TREE){
		/* squirrel eating squirrel on tree */
		dest_cell->type = SQUIRREL_IN_TREE;
		dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period); /*FIXME: Should we increment the breeding period? */
		
		/* clean cell */
		cleanup_cell(cell);
	} else if(cell->type == SQUIRREL_IN_TREE){
		/* Squirrel leaving tree */
		dest_cell->type = SQUIRREL;
		dest_cell->breeding_period = cell->breeding_period;
		
		/* clean cell */
		cleanup_cell(cell);
		if(dest_cell->breeding_period >= squirrel_breeding_period)
			cell->type = SQUIRREL_IN_TREE;
		else
			cell->type = TREE;

		dest_cell->breeding_period = 0;
		
	} else {
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

void move(world_cell* cell, world_cell* dest_cell) {
	dest_cell->moved = 1;
	if(cell->type == WOLF)
		move_wolf(cell, dest_cell);
	else
		move_squirrel(cell, dest_cell);
}

unsigned short choose_cell(unsigned short i, unsigned short j, unsigned short p){
	return (i * grid_size + j) % p;
}

char add_cell(world_cell* aux_cell, world_cell** possible_cells, char bad_type){
	if(aux_cell->type != bad_type && aux_cell->type != WOLF && aux_cell->type != ICE){
		*possible_cells = &world[aux_cell->x][aux_cell->y];
		return 1;
	}
	return 0;
}

world_cell** retrieve_possible_cells(world_cell* cell){
	
	world_cell** possible_cells = calloc(4, sizeof(world_cell*)); /* 4: max possible positions */
	world_cell** tmp_cell = possible_cells;
	char bad_type = 0;

	if(cell->type == WOLF)
		bad_type = TREE;
	else if (cell->type == SQUIRREL)
		bad_type = SQUIRREL;

	/*check top cell*/
	if(cell->x && add_cell(&world_previous[cell->x-1][cell->y], tmp_cell, bad_type))
		tmp_cell++;
	
	/*check right cell*/
	if(cell->y != grid_size-1 && add_cell(&world_previous[cell->x][cell->y+1], tmp_cell, bad_type))
		tmp_cell++;
	
	/*check bottom cell*/
	if(cell->x != grid_size-1 && add_cell(&world_previous[cell->x+1][cell->y], tmp_cell, bad_type))
		tmp_cell++;
	
	/*check left cell */
	if(cell->y && add_cell(&world_previous[cell->x][cell->y-1], tmp_cell, bad_type))
		tmp_cell++;
	
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

				/*printf("Checking possible cells for wolf in %d,%d\n", cell->x,cell->y);*/
				possible_cells = retrieve_possible_cells(cell);
				for(; count < 4 && possible_cells[count] == NULL; count++){
					if(possible_cells[count]->type == SQUIRREL)
						squirrel_cells[squirrels_found++] = possible_cells[count];
				}
				
				if(squirrels_found)
					move(cell, squirrel_cells[choose_cell(cell->x, cell->y, squirrels_found)]);
				else if (count)
					move(cell, possible_cells[choose_cell(cell->x, cell->y, count--)]);

				free(squirrel_cells);
				free(possible_cells);
				break;
			}
		case SQUIRREL: 
		case SQUIRREL_IN_TREE:
			/*printf("Checking possible cells for squirrel in %d,%d\n", cell->x,cell->y);*/
			possible_cells = retrieve_possible_cells(cell);
			for(; count < 4 && possible_cells[count] == NULL; count++);

			if(count)
				move(cell, possible_cells[choose_cell(cell->x, cell->y, count--)]);
	
			free(possible_cells);
			break;
		default:	/* we assume that trees and ice are less frequent than the animals */
			break;
	}
}

void initialize_world_array(unsigned short size){
	unsigned short i = 0;
	world = malloc(size * sizeof(world_cell*));
	world_previous = malloc(size * sizeof(world_cell*));

	for(; i < size; i++){
		unsigned short j = 0;
		world[i] = calloc(size, sizeof(world_cell));
		world_previous[i] = calloc(size, sizeof(world_cell));

		for(; j < size; j++){
			world[i][j].type = world_previous[i][j].type = EMPTY;
			world[i][j].x = world_previous[i][j].x = i;
			world[i][j].y = world_previous[i][j].y = j;
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

	if(fscanf(input, "%hu\n", &grid_size) == 0){
		printf("No grid size.");
		exit(2);
	}
	
	/*We only know the world size here*/
	initialize_world_array(grid_size);

	while(fscanf(input,"%hu %hu %c\n",&i, &j, &type) == 3){ /*All arguments read succesfully*/
		world[i][j].type = type;
		if(type == WOLF)
			world[i][j].starvation_period = wolf_starvation_period;
	}

	if(fclose(input) == EOF)
		exit(3);
}

void print_world(world_cell ** world){
	int i = 0;
	
	/*print header*/
	printf("  ");
	for(; i < grid_size; i++)
		printf("%d ", i);

	printf("\n");
	
	/*print world*/
	for(i = 0; i < grid_size; i++){
		int j = 0;
		printf("%d|", i);
		for(; j < grid_size; j++)
			printf("%c|", world[i][j].type);

		printf("\n");
	}
}

void print_world_stats(void){
	printf("Grid size: %d\n", grid_size);
	printf("Wolf breeding period: %d\n", wolf_breeding_period);
	printf("Wolf starvation period: %d\n", wolf_starvation_period);
	printf("Squirrel breeding period: %d\n", squirrel_breeding_period);
	printf("Number of generations: %d\n", number_of_generations);
}

void copy_world(void){
	int i, j;
	#pragma omp parallel for private(j)
	for(i = 0; i < grid_size; i++)
		for(j = 0; j < grid_size; j++){
			world_cell *source = &world[i][j], *dest = &world_previous[i][j];
			create_world_cell(dest, source->type, source->breeding_period, source->starvation_period, source->x, source->y);
			world_previous[i][j].moved = world[i][j].moved;
		}
}

void start_world_simulation(void){
	int i, j;
	for(; number_of_generations > 0; --number_of_generations){
		/*printf("---- Generation %d ----\n", g + 1);*/

		copy_world();

		/* update 'red' cells, think chessboard */
		#pragma omp parallel for private(j)
		for(i = 0; i < grid_size; i++)
			for (j = i & 1; j < grid_size; j += 2)
				update_world_cell(i,j);

		/*printf("*** RED %d ***\n", g + 1);*/
		/*print_world(world);*/
		copy_world();

		/* update 'black' cells, think chessboard */
		#pragma omp parallel for private(j)
		for(i = 0; i < grid_size; i++)
			for (j = !(i & 1); j < grid_size; j += 2)
				update_world_cell(i,j);
		
		/*printf("*** BLACK %d ***\n", g + 1);*/
		/*print_world(world);*/
		#pragma omp parallel for private(j)
		for(i = 0; i < grid_size; i++){
			for (j = 0; j < grid_size; j ++){
				if (world[i][j].moved){
					if (world[i][j].type == SQUIRREL){
						world[i][j].breeding_period++;
					} else if (world[i][j].type == WOLF){
						world[i][j].starvation_period--;
						world[i][j].breeding_period++;
						
						if(world[i][j].starvation_period <= 0){
							cleanup_cell(&world[i][j]);
							/*printf("Bye Wolf (%d, %d)\n", i, j);*/
						}
					}
				}
				world[i][j].moved = 0;
			}
		}	
	}
	
}

void freemem(void){
	unsigned short i = 0;

	for(; i < grid_size; i++){
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
	
	print_world_stats();
	
	start_world_simulation();

	/*print_world(world);*/

	#ifdef GETTIME
    printf("OpenMP time: %fs\n", omp_get_wtime() - start);
    #endif
    
    freemem();

	return 0;
}