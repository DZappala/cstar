

typedef int MPI_Datatype;
typedef int MPI_Comm;
typedef int MPI_Op;

#define MPI_COMM_WORLD 1
#define MPI_SUCCESS 1
#define MPI_INT 1
#define MPI_FLOAT 2
#define MPI_CHAR 3
#define MPI_ANY_TAG -1
#define MPI_ANY_SOURCE -2
#define MPI_MAX 101
#define MPI_MIN 102
#define MPI_SUM 103
#define MPI_PROD 104
#define MPI_LAND 105
#define MPI_LOR 106
#define MPI_LXOR 107
#define MPI_PROC_NULL -10
#define MPI_COMM_NULL -20

typedef struct {
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
} MPI_Status;

int MPI_Init();

int MPI_Finalize();

int MPI_Comm_rank(int comm, int *rank);

int MPI_Comm_size(int comm, int *size);

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm);

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status);

int MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count);

int MPI_Barrier(MPI_Comm comm);

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm);

int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm);

int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm);

int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm);

int MPI_Allreduce(void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
               MPI_Status *status);

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods,
                    int reorder, MPI_Comm *comm_cart);

int MPI_Cartdim_get(MPI_Comm comm, int *ndims);

int MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods,
                 int *coords);

int MPI_Cart_rank(MPI_Comm comm, int *coords, int *rank);

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords);

int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source,
                   int *rank_dest);

int MPI_Comm_free(MPI_Comm *comm);

float MPI_Wtime();
