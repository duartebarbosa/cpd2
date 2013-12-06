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

#define NEW_BORN 3
#define UPDATED 2
#define MOVED 1

#define MASTER 0
#define INIT_TAG 1
#define FILL_TAG 50
#define RECV_TAG 500
#define CONF_TAG 150

#define RED 999
#define BLACK 888

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX_BREED(a, b) MAX(a->breeding_period, b->breeding_period)
#define MAX_STARV(a, b) MAX(a->starvation_period, b->starvation_period)

#define GET_REAL_X(cell) ((cell->number-world[0][0].number)/grid_size)
#define GET_X(number) ((number)/grid_size)
#define GET_Y(cell) cell->number%grid_size
#define CHOOSE_CELL(cell, p) cell->number%p

#define CHUNK ((grid_size-(numtasks-1)*2)/numtasks)
#define CHUNK_REMAINDER ((grid_size-(numtasks-1)*2)%numtasks)

#define FLIMIT_INF_CHUNK(x) x*(CHUNK+2)
#define FLIMIT_SUP_CHUNK(x) FLIMIT_INF_CHUNK(x)+CHUNK

#define LIMIT_INF_CHUNK FLIMIT_INF_CHUNK(taskid)
#define LIMIT_SUP_CHUNK FLIMIT_SUP_CHUNK(taskid)

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
unsigned short chunk_size;
unsigned short payload;

MPI_Datatype mpi_world_cell_type;

int bottom, top, info[5], numtasks, taskid;

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
	int i, j;
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

	while(fscanf(input,"%d %d %c\n",&i, &j, &type) == 3){ /*All arguments read succesfully*/
		world[i][j].type = type;
		if(type == WOLF){
			world[i][j].starvation_period = wolf_starvation_period;
		}
	}

	if(fclose(input) == EOF)
		exit(3);
}

inline void cleanup_cell(world_cell* cell){
	cell->type = EMPTY;
	cell->breeding_period = cell->starvation_period = 0;
}

void move_wolf(world_cell* cell, world_cell* dest_cell) {

	dest_cell->moved = UPDATED;
	switch(dest_cell->type){
		case SQUIRREL:
			/* Wolf eating squirrel */
			dest_cell->type = WOLF;
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->starvation_period = wolf_starvation_period; 

			/* clean cell */
			cleanup_cell(cell);
			break;
		case WOLF:
			/* Wolf fighting wolf */
			dest_cell->type = WOLF;
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
			dest_cell->type = WOLF;
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->starvation_period = cell->starvation_period;
			
			/* clean cell or reproduce*/
			if(dest_cell->breeding_period >= wolf_breeding_period && dest_cell->starvation_period > 1){
				cell->type = WOLF;
				cell->moved = NEW_BORN;
				cell->breeding_period = dest_cell->breeding_period = 0;
				cell->starvation_period = wolf_starvation_period;
			} else {
				cleanup_cell(cell);
			}

	}
}

inline void move_squirrel_in_tree(world_cell* cell){
	if(cell->type == SQUIRREL_IN_TREE)
		cell->type = TREE;
	else
		cleanup_cell(cell);
}

void move_squirrel(world_cell* cell, world_cell* dest_cell) {
	dest_cell->moved = UPDATED;
	switch(dest_cell->type){
		case TREE:
			dest_cell->breeding_period = cell->breeding_period;
			dest_cell->type = SQUIRREL_IN_TREE;

			move_squirrel_in_tree(cell);
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
			dest_cell->breeding_period = MAX_BREED(cell, dest_cell);
			move_squirrel_in_tree(cell);
			break;
		case WOLF:
			/* Wolf eating squirrel */
			dest_cell->starvation_period = wolf_starvation_period;
			move_squirrel_in_tree(cell);
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
					cell->moved = UPDATED;
					dest_cell->breeding_period = 0;
				} else
					cell->type = EMPTY;
			}
	}
}

char add_cell(world_cell* aux_cell, world_cell** possible_cells, char bad_type){
	if(aux_cell->type != bad_type && aux_cell->type != ICE && aux_cell->type != SQUIRREL_IN_TREE && aux_cell->type != WOLF){
		*possible_cells = &world_previous[GET_REAL_X(aux_cell)][GET_Y(aux_cell)];
		return 1;
	}
	return 0;
}

world_cell** retrieve_possible_cells(world_cell* cell){
	
	world_cell** possible_cells = calloc(4, sizeof(world_cell*)); /* 4: max possible positions */
	world_cell** tmp_cell = possible_cells;
	char bad_type = 0;
	unsigned short x = GET_REAL_X(cell), y = GET_Y(cell);
	
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
	if(x != payload-1 && add_cell(&world_previous[x+1][y], tmp_cell, bad_type))
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
					move_wolf(cell, &world[GET_REAL_X(prev_gen_dest_cell)][GET_Y(prev_gen_dest_cell)]);
				} else if (count) {
					world_cell* prev_gen_dest_cell = possible_cells[CHOOSE_CELL(cell, count--)];
					move_wolf(cell, &world[GET_REAL_X(prev_gen_dest_cell)][GET_Y(prev_gen_dest_cell)]);
				} else {
					cell->moved = UPDATED; //was updated anyways
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
				move_squirrel(cell, &world[GET_REAL_X(prev_gen_dest_cell)][GET_Y(prev_gen_dest_cell)]);
			} else {
				cell->moved = UPDATED; //was updated anyways
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
	printf("              ");
	for(; i < grid_size; ++i)
		printf("%d ", i);

	printf("\n");
	
	/*print world*/
	for(i = 0; i < max; ++i){
		int j = 0;
		if (i < bottom){
			printf("[B Task: %d] %d|",taskid,  i);
		} else if (i >=	top){
			printf("[T Task: %d] %d|",taskid,  i);
		} else {
			printf("[- Task: %d] %d|",taskid,  i);
		}
		for(; j < grid_size; ++j){
				printf("%c|", world[i][j].type, world[i][j].starvation_period);
			
		}
		printf("\n");
	}
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

void gather(){
	MPI_Status status;

	//Sync to master
	if(taskid == MASTER){
		int i = top + 2;

		for(; i < grid_size; i++)
			MPI_Recv(world[i], grid_size, mpi_world_cell_type, MPI_ANY_SOURCE, RECV_TAG+i, MPI_COMM_WORLD, &status);

		print_world(grid_size);
	} else {
		int i = bottom;

		for(; i < payload; i++)
			MPI_Send(world[i], grid_size, mpi_world_cell_type, MASTER, RECV_TAG+GET_X(world[i][0].number), MPI_COMM_WORLD);
	}
}

void copy_world(){
	register int i;
	for(i = 0; i < payload; ++i)
		memcpy(world_previous[i], world[i], grid_size*sizeof(world_cell));
}


int get_cell_color(world_cell* cell){
	int x = GET_X(cell->number);
	int y = GET_Y(cell);

	if((x%2 == 0 && y%2 == 0) || (x%2 != 0 && y%2 != 0)){
		return RED;
	} else {
		return BLACK;
	}
}

void resolve_conflicts(int generation_color, int gen_number){
	int i;
	world_cell* conf1 = malloc(grid_size*sizeof(world_cell));
	world_cell* conf2 = malloc(grid_size*sizeof(world_cell));

	//send to taskid-1
	if(taskid != MASTER){
		MPI_Request size_reqs[2];
		MPI_Isend(world[0], grid_size, mpi_world_cell_type, taskid-1, CONF_TAG, MPI_COMM_WORLD, &size_reqs[0]);
		MPI_Isend(world[1], grid_size, mpi_world_cell_type, taskid-1, CONF_TAG+1, MPI_COMM_WORLD, &size_reqs[1]);
	}
	
	//receive from taskid +1
	if(taskid != numtasks-1){
		MPI_Status status;
		MPI_Recv(conf1, grid_size, mpi_world_cell_type, taskid+1, CONF_TAG, MPI_COMM_WORLD, &status);
		MPI_Recv(conf2, grid_size, mpi_world_cell_type, taskid+1, CONF_TAG+1, MPI_COMM_WORLD, &status);
	}

	//resolve conflicts on n+1 n+2
	// Keep all moved cells from THIS generation (world), discard others
	for(i=0; i < grid_size; i++){
		//discard unmoved cells from my world with color 'generation_color'
		if(((world[payload-2][i].moved != UPDATED) && (world[payload-2][i].moved != NEW_BORN)) && get_cell_color(&world[payload-2][i]) == generation_color && world[payload-2][i].type != ICE && world[payload-2][i].type != TREE && world[payload-2][i].type != EMPTY){
			cleanup_cell(&world[payload-2][i]);
		} else if (world[payload-2][i].moved == UPDATED || world[payload-2][i].moved == NEW_BORN) {
			world[payload-2][i].moved = MOVED;
		}
		if(((world[payload-1][i].moved != UPDATED) && (world[payload-1][i].moved != NEW_BORN)) && get_cell_color(&world[payload-1][i]) == generation_color && world[payload-1][i].type != ICE && world[payload-1][i].type != TREE && world[payload-1][i].type != EMPTY){
			cleanup_cell(&world[payload-1][i]);
		} else if (world[payload-1][i].moved == UPDATED || world[payload-1][i].moved == NEW_BORN){
			world[payload-1][i].moved = MOVED;
		}
		if(conf1[i].moved == UPDATED || conf1[i].moved == NEW_BORN){
			//move to my world
			if(conf1[i].type == WOLF)
				move_wolf(&conf1[i], &world[payload-2][i]);
			else
				move_squirrel(&conf1[i], &world[payload-2][i]);

			world[payload-2][i].moved = MOVED;
		}
		if(conf2[i].moved == UPDATED || conf2[i].moved == NEW_BORN){
			//move to my world
			if(conf2[i].type == WOLF)
				move_wolf(&conf2[i], &world[payload-1][i]);
			else
				move_squirrel(&conf2[i], &world[payload-1][i]);

			world[payload-1][i].moved = MOVED;
		}
	}
	
	//send to taskid+1
	if(taskid != numtasks-1){
		MPI_Request size_reqs[2];
		MPI_Isend(world[payload-2], grid_size, mpi_world_cell_type, taskid+1, CONF_TAG-2, MPI_COMM_WORLD, &size_reqs[0]);
		MPI_Isend(world[payload-1], grid_size, mpi_world_cell_type, taskid+1, CONF_TAG-1, MPI_COMM_WORLD, &size_reqs[1]);
	}	
	//receive from taskid -1
	if(taskid != MASTER){
		MPI_Status status;
		MPI_Recv(world[0], grid_size, mpi_world_cell_type, taskid-1, CONF_TAG-2, MPI_COMM_WORLD, &status);
		MPI_Recv(world[1], grid_size, mpi_world_cell_type, taskid-1, CONF_TAG-1, MPI_COMM_WORLD, &status);
	}
}

void start_world_simulation(void){
	register int i, j, btm_lim = bottom, top_lim = top;

	for(; number_of_generations > 0; --number_of_generations){
		copy_world();

		if(taskid != MASTER)
			btm_lim = bottom - 1;

		if(taskid != numtasks-1)
			top_lim = top + 1;

		/* update 'red' cells, think chessboard */
		for(i = btm_lim; i < top_lim; ++i){
			for (j = 0; j < grid_size; j++){
				if(get_cell_color(&world[i][j]) == RED){
					update_world_cell(i, j);
				}
			}
		}
	
		resolve_conflicts(RED, number_of_generations);
		copy_world();

		/* update 'black' cells, think chessboard */
		for(i = btm_lim; i < top_lim; ++i){
			for (j = 0; j < grid_size; j++){
				if(get_cell_color(&world[i][j]) == BLACK){
					update_world_cell(i, j);
				}
			}
		}

		resolve_conflicts(BLACK, number_of_generations);

		if(number_of_generations == 1)
			return;

		for(i = 0; i < payload; ++i){
			for (j = 0; j < grid_size; ++j){
				if (world[i][j].moved == UPDATED || world[i][j].moved == MOVED){
					if (world[i][j].type == SQUIRREL || world[i][j].type == SQUIRREL_IN_TREE){
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

	int task, len, chunks = CHUNK;
	MPI_Status status;
	char hostname[MPI_MAX_PROCESSOR_NAME];

	#ifdef GETTIME
	double start = MPI_Wtime();
	#endif

	const int nitems=3;
	int blocklengths[3] = {2,2,1};
	MPI_Datatype types[3] = {MPI_CHAR, MPI_UNSIGNED_SHORT, MPI_UNSIGNED};
	MPI_Aint offsets[3];

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

		info[1] = wolf_breeding_period = atoi(argv[2]);
		info[2] = squirrel_breeding_period = atoi(argv[3]);
		info[3] = wolf_starvation_period = atoi(argv[4]);
		info[4] = number_of_generations = atoi(argv[5]);

		parse_input(argv[1]);

		info[0] = grid_size;

		bottom = 0;
		top = chunk_size = CHUNK;
		payload = top + 2;

		for(task = 1; task < numtasks; task++)
			MPI_Isend(info, 5, MPI_INT, task, INIT_TAG, MPI_COMM_WORLD, &size_reqs[task-1]);

		MPI_Waitall(numtasks - 1, size_reqs, MPI_STATUS_IGNORE);

		for(task = 1; task < numtasks; task++){
			int bottom_task = FLIMIT_INF_CHUNK(task),
				top_task = FLIMIT_SUP_CHUNK(task),
				chunk_size = top_task-bottom_task;
			
			bottom_task -= 2;

			if (task == numtasks-1)
				top_task += CHUNK_REMAINDER;
			else
				top_task += 2;

			for( ; bottom_task < top_task; bottom_task++)
				MPI_Send(world[bottom_task], grid_size, mpi_world_cell_type, task, FILL_TAG, MPI_COMM_WORLD);
		}

	} else {
		int j = 0;

		MPI_Recv(info, 5, MPI_INT, MASTER, INIT_TAG, MPI_COMM_WORLD, &status);

		grid_size = info[0];
		wolf_breeding_period = info[1];
		squirrel_breeding_period = info[2];
		wolf_starvation_period = info[3];
		number_of_generations = info[4];
	
		bottom = 2;
		if(taskid == numtasks-1){
			chunk_size = CHUNK + CHUNK_REMAINDER;
			payload = top = chunk_size+bottom;
		} else {
			chunk_size = CHUNK;
			top = chunk_size+bottom;
			payload = top + 2;
		}

		initialize_world_array(payload );
		
		for( ; j < payload; j++)
			MPI_Recv(world[j], grid_size, mpi_world_cell_type, MASTER, FILL_TAG, MPI_COMM_WORLD, &status);
	}

	start_world_simulation();

	gather();

	#ifdef GETTIME
	if(taskid == MASTER){
	  printf("MPI time: %lf\n", MPI_Wtime() - start);
	  print_world();
	}
	#endif

	//freemem();
	MPI_Finalize();

	return 0;
}
