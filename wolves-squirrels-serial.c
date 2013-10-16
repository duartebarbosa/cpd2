#include <stdio.h>
#include <stdlib.h>

#define WOLF 'w'
#define SQUIRREL 's'
#define ICE 'i'
#define TREE 't'

#define MAX 10 //Not sure if required

typedef struct _world_cell {
	int type; /* Wolf, squirrel, tree, ice*/
	int breeding_period;
	int starvation_period;
} world_cell;

world_cell world[MAX][MAX];

void initialize_world_array();
void print_world();
world_cell create_world_cell(int type, int breeding_period, int starvation_period);

int main(int argc, char **argv)
{
	initialize_world_array(world);
	print_world(world);
	
	return 0;
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
			world[i][j] = create_world_cell(WOLF, 0,0);
		}
	}
}

void print_world(){
	printf("%d\n", MAX);
	int i;
	for(i = 0; i < MAX; i++){
		int j;
		for(j = 0; j < MAX; j++){
			printf("%c ", world[i][j].type);
		}
		printf("\n");
	}
}
