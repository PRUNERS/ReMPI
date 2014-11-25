/* C datatypes */
#define ReMPI_DATATYPE_NULL         (0)
#define ReMPI_BYTE                  (1)
#define ReMPI_PACKED                (2)
#define ReMPI_CHAR                  (3)
#define ReMPI_SHORT                 (4)
#define ReMPI_INT                   (5)
#define ReMPI_LONG                  (6)
#define ReMPI_FLOAT                 (7)
#define ReMPI_DOUBLE                (8)
#define ReMPI_LONG_DOUBLE           (9)
#define ReMPI_UNSIGNED_CHAR        (10)
#define ReMPI_SIGNED_CHAR          (11)
#define ReMPI_UNSIGNED_SHORT       (12)
#define ReMPI_UNSIGNED_LONG        (13)
#define ReMPI_UNSIGNED             (14)
#define ReMPI_FLOAT_INT            (15)
#define ReMPI_DOUBLE_INT           (16)
#define ReMPI_LONG_DOUBLE_INT      (17)
#define ReMPI_LONG_INT             (18)
#define ReMPI_SHORT_INT            (19)
#define ReMPI_2INT                 (20)
#define ReMPI_UB                   (21)
#define ReMPI_LB                   (22)
#define ReMPI_WCHAR                (23)


int rempi_translate_communicator(MPI_Comm *comm, int *ranks);

