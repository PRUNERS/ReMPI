#ifndef REMPI_RECORD_H
#define REMPI_RECORD_H

/* struct communicator_identifier { */
/*   int size; */
/*   int *ranks; */
/* }; */



#ifdef __cplusplus
extern "C" {
#endif

int rempi_record_init(int *argc, char ***argv, int rank);
int rempi_replay_init(int *argc, char ***argv, int rank);


/* struct communicator_identifier* rempi_allocate_communicator_identifier(int size); */
/* int rempi_free_communicator_identifier(struct communicator_identifier *comm_id); */

/* int rempi_record_communicator(int id, ) */
/* { */

/* } */

int rempi_record_irecv(
   void *buf,
   int count,
   int datatype, // The value is assigned in ReMPI_convertor
   int source,
   int tag,
   int comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   void *request
);
int rempi_replay_irecv(
   void *buf,
   int count,
   int datatype, // The value is assigned in ReMPI_convertor
   int source,
   int tag,
   int comm, // The value is set by MPI_Comm_set_name in ReMPI_convertor
   void *request
);

int rempi_record_test(
    void *request,
    int *flag,
    int source, // of MPI_Status
    int tab     // of MPI_tatus
);
int rempi_replay_test(
    void *request,
    int *flag,
    int *source, // of MPI_Status
    int *tab     // of MPI_tatus
);


int rempi_record_testsome(
    int incount,
    void *array_of_requests[],
    int *outcount,
    int array_of_indices[],
    void *array_of_statuses[]
);
int rempi_replay_testsome(
    int incount,
    void *array_of_requests[],
    int *outcount,
    int array_of_indices[],
    void *array_of_statuses[]
);

//TODO: Comm_dup Comm_split

int rempi_record_finalize(void);
int rempi_replay_finalize(void);

#ifdef __cplusplus
}
#endif

#endif
