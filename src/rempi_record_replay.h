#ifndef REMPI_RECORD_REPLAY_H
#define REMPI_RECORD_REPLAY_H


int rempi_record_replay_init(
    int *argc,			     
    char ***argv
);
			     

int rempi_record_replay_irecv(
    void *buf, 
    int count, 
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm comm,
    MPI_Request *request
);

int rempi_record_replay_test(
    MPI_Request *request, 
    int *flag, 
    MPI_Status *status
);


int rempi_record_replay_testsome(
    int incount, 
    MPI_Request array_of_requests[],
    int *outcount, 
    int array_of_indices[], 
    MPI_Status array_of_statuses[]
);

int rempi_record_replay_finalize(
);

#endif
