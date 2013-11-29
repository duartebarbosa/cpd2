#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'
#define SQUIRREL_IN_TREE '$'
#define EMPTY ' '

#define MASTER 0
#define INIT_TAG 1
#define FILL_TAG 50

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define GET_X(number) number/grid_size
#define GET_Y(number) number%grid_size
#define CHOOSE_CELL(number, p) number%p


#define CHUNK(x) ((x)/numtasks)
#define LIMIT_INF_CHUNK(x) ((taskid)*((x)/numtasks))
#define LIMIT_SUP_CHUNK(x) (((taskid+1)==numtasks)?(x):((taskid+1)*((x)/numtasks)))

#define FLIMIT_INF_CHUNK(x,y) ((y)*((x)/numtasks))
#define FLIMIT_SUP_CHUNK(x,y) (((y+1)==numtasks)?(x):((y+1)*((x)/numtasks)))

typedef struct {
	char type; /* Wolf, squirrel, squirrel in tree, tree, ice, empty */
	char moved;
	unsigned short breeding_period;
	unsigned short starvation_period;
	unsigned int number;
} world_cell;

/* Global Variables */
world_cell **world;
world_cell **world_previous;
unsigned short wolf_breeding_period;
unsigned short squirrel_breeding_period;
unsigned short wolf_starvation_period;
unsigned int number_of_generations;
unsigned short grid_size;
  
	int numtasks, taskid;

void initialize_world_array(int max){
	register unsigned short i = 0;
	world = malloc(grid_size * sizeof(world_cell*));
	world_previous = malloc(grid_size * sizeof(world_cell*));

	for(; i < max; ++i){
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

	initialize_world_array(grid_size);

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
				dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period);
			} else {
				dest_cell->breeding_period = (cell->starvation_period > dest_cell->starvation_period ? cell->breeding_period : dest_cell->breeding_period);
				dest_cell->starvation_period = MAX(cell->starvation_period, dest_cell->starvation_period);
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
			
			dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period);

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
			dest_cell->breeding_period = MAX(cell->breeding_period, dest_cell->breeding_period);

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
		*possible_cells = &world_previous[GET_X(aux_cell->number)][GET_Y(aux_cell->number)];
		return 1;
	}
	return 0;
}

world_cell** retrieve_possible_cells(world_cell* cell){
	
	world_cell** possible_cells = calloc(4, sizeof(world_cell*)); /* 4: max possible positions */
	world_cell** tmp_cell = possible_cells;
	char bad_type = 0;
	unsigned short x = GET_X(cell->number), y = GET_Y(cell->number);

	if(cell->type == WOLF){
		bad_type = TREE;
	}
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
					world_cell* prev_gen_dest_cell = squirrel_cells[CHOOSE_CELL(cell->number, squirrels_found)];
					move_wolf(cell, &world[GET_X(prev_gen_dest_cell->number)][GET_Y(prev_gen_dest_cell->number)]);
				} else if (count) {
					world_cell* prev_gen_dest_cell = possible_cells[CHOOSE_CELL(cell->number, count--)];
					move_wolf(cell, &world[GET_X(prev_gen_dest_cell->number)][GET_Y(prev_gen_dest_cell->number)]);
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
				world_cell* prev_gen_dest_cell = possible_cells[CHOOSE_CELL(cell->number, count--)];
				move_squirrel(cell, &world[GET_X(prev_gen_dest_cell->number)][GET_Y(prev_gen_dest_cell->number)]);
			}
	
			free(possible_cells);
			break;
		default:	/* we assume that trees and ice are less frequent than the animals */
			break;
	}
}

void print_grid(world_cell ** world, int max){
	register int i = 0;
	
	/*print header*/
	printf("  ");
	for(; i < grid_size; ++i)
		printf("%d ", i);

	printf("\n");
	
	/*print world*/
	for(i = 0; i < max; ++i){
		int j = 0;
		printf("[Task: %d] %d|",taskid,  i);
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

void print_world(int max){
	int i = 0, j;
	for(; i < max; ++i){
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

		/*if(number_of_generations == 1 && taskid == MASTER){
			print_world(chunk_size);
		}*/

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

int info[2];

int main(int argc, char **argv){
  
	int task, chunk_size, len;
        MPI_Status status;
	int chunks = CHUNK(grid_size);
        MPI_Request reqs[chunks];
	char hostname[MPI_MAX_PROCESSOR_NAME];

	#ifdef GETTIME
	double start = MPI_Wtime();
	#endif
        
	const int nitems=3;
	int          blocklengths[3] = {2,2,1};
	MPI_Datatype types[3] = {MPI_CHAR, MPI_UNSIGNED_SHORT, MPI_UNSIGNED};
	MPI_Datatype mpi_world_cell_type;
	MPI_Aint     offsets[3];

	offsets[0] = offsetof(world_cell, type);
	offsets[1] = offsetof(world_cell, breeding_period);
	offsets[2] = offsetof(world_cell, number);
	

	/* MPI Initialization */
        if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
                printf ("Error starting MPI program. Terminating.\n");
                /*MPI_Abort(MPI_COMM_WORLD, ret);*/
                return -1;
        }
        
        MPI_Get_processor_name(hostname, &len);
        
	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_world_cell_type);
	MPI_Type_commit(&mpi_world_cell_type);
        
        MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
        MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
	
	if(taskid == MASTER){
	  MPI_Request size_reqs[numtasks-1];

	  /* Maybe check for invalid input? */
	  wolf_breeding_period = atoi(argv[2]);
	  squirrel_breeding_period = atoi(argv[3]);
	  wolf_starvation_period = atoi(argv[4]);
	  number_of_generations = atoi(argv[5]);
		  
	  parse_input(argv[1]); /* Filename */
	  
	  	if(taskid == MASTER){
		print_grid(world, grid_size);
	}
	  
	  info[1]=grid_size;
	  
	   for(task = 1; task < numtasks; task++){
	        int chunk_size = FLIMIT_SUP_CHUNK(grid_size, task) - FLIMIT_INF_CHUNK(grid_size, task);
			info[0]=chunk_size;
		printf("[%s] Will send %d chunk_size to %d\n", hostname, chunk_size, task);
		MPI_Isend(info, 2, MPI_INT, task, INIT_TAG, MPI_COMM_WORLD, &size_reqs[task-1]);
	   }

           MPI_Waitall(numtasks - 1, size_reqs, MPI_STATUS_IGNORE);
	    
	   for(task = 1; task < numtasks; task++){
		   int c = FLIMIT_INF_CHUNK(grid_size, task);
		   int lim = FLIMIT_SUP_CHUNK(grid_size, task);
			int chunk_size = FLIMIT_SUP_CHUNK(grid_size, task) - FLIMIT_INF_CHUNK(grid_size, task);
			printf("[%s] Sending cells to %d\n", hostname, task);
			for( ; c < lim; c++){
				printf("[%s] Sending line %d to %d\n", hostname, c, task);
                MPI_Send(world[c], grid_size, mpi_world_cell_type, task, FILL_TAG, MPI_COMM_WORLD);
			}
	   }		
	}
	else{
		 int c ,lim, j = 0;
		   
		MPI_Recv(info, 2, MPI_INT, MASTER, INIT_TAG, MPI_COMM_WORLD, &status);
		chunk_size=info[0];
		grid_size=info[1];
		
		c = FLIMIT_INF_CHUNK(grid_size, taskid);
		lim = FLIMIT_SUP_CHUNK(grid_size, taskid);
		 
		printf("[%s-%d] Will receive chunk_size %d\n", hostname, taskid, chunk_size);
		
		initialize_world_array(chunk_size);
		
		for( ; j < chunk_size; j++){
		    printf("[%s-%d] Receiving line %d from %d\n", hostname, taskid, j, MASTER);
            MPI_Recv(world[j], grid_size, mpi_world_cell_type, MASTER, FILL_TAG, MPI_COMM_WORLD, &status);
            int i = 0;
			/*for(; i < grid_size; i++){
			  printf("[%s-%d] TYPE: %c - MOVED: %d - BP: %d - SP: %d - NUMBER: %d\n", hostname, taskid, world[j][i].type, world[j][i].moved,world[j][i].breeding_period,world[j][i].starvation_period,world[j][i].number);
			}*/
		}
		
		printf("[%s-%d] Received %d cells. Printing world:\n", hostname, taskid, chunk_size * grid_size);
		
	}
	
	//start_world_simulation();



	print_grid(world, FLIMIT_SUP_CHUNK(grid_size, taskid) - FLIMIT_INF_CHUNK(grid_size, taskid));

	#ifdef GETTIME
	if(taskid == MASTER){
	  printf("MPI time: %lf\n", MPI_Wtime() - start);
	}
    	#endif

	//freemem();
	MPI_Finalize();

	return 0;
}
