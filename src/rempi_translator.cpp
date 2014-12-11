#include "mpi.h"

#include "rempi_translator.h"

int rempi_translate_communicator(MPI_Comm *input_comm, int *output_ranks) 
{
  MPI_Group world_comm_group, input_comm_group;
  int input_comm_group_size;
  int *input_comm_group_ranks;
  int i;

  MPI_Comm_group(MPI_COMM_WORLD, &world_group);
  MPI_Comm_group(*input_comm, &group);

  MPI_Group_size(translated_group, &translated_group_size);

  input_comm_group_ranks  = malloc(input_comm_group_size * sizeof(int));

  for (i = 0; i < input_comm_group_size; i++) {
    ranks[i] = i;
  }

  MPI_Group_translate_ranks(input_comm_group, input_comm_group_size, input_comm_group, 
			    world_comm_group, output_ranks);

  free(input_comm_group_ranks);

  MPI_Group_free(&world_comm_group);
  MPI_Group_free(&input_comm_group);
  return 0;
}

