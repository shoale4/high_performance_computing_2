#include "collective.h"
#include "utils.h"

const int COLLECTIVE_DEBUG = 0;

/*************************** DECLARE YOUR HELPER FUNCTIONS HERE ************************/



/*************************** collective.h functions ************************/


 void HPC_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    // TODO: Implement this function using only sends and receives for communication instead of MPI_Bcast.
    
   // MPI_Bcast(buffer, count, datatype, root, comm);

   int rank, size;
   MPI_Comm_rank(comm, &rank);
   MPI_Comm_size(comm, &size);

   double log_p;
   log_p = log2(size);
   int d;
   d = ceil(log_p);

   int flip, mask;
   flip = 1<<(d-1);
   mask = flip-1;th

   for (int j=d-1; j >= 0; j--){
      if (((rank^root)&mask) == 0){
         if (((rank^root)&flip) == 0){
            MPI_Send(buffer, count, datatype, (rank^flip), 0, comm);
         }
         else{
            MPI_Recv(buffer, count, datatype, (rank^flip), 0, comm, MPI_STATUS_IGNORE);
         }
      }
   mask = mask>>1;
   flip = flip>>1;
   }

}


void HPC_Prefix(const HPC_Prefix_func* prefix_func, const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Comm comm, void* wb1, void* wb2, void* wb3) {
    if (count <= 0) return;

    /* Step 1. Run user function on local data with a NULL previous prefix. */
    const void* local_last_prefix = prefix_func(NULL, sendbuf, recvbuf, count);

    // TODO: Implement the rest of this function using sends and receives for communication.
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int d = (int) log2(size);
    if(1<<d < size) {
        d++;
    }
    prefix_func(NULL, local_last_prefix, wb1, 1); // wb1 is total_sum = last element of local prefix
    bool identity_element = true; // wb2 is prefix sum = identity element // TODO: smarter handling of identity
    for(int i = 1; i < 1 << d; i <<= 1) {
        MPI_Request req;
        int partner_rank = rank ^ i;
        if(partner_rank < size) {
            MPI_Isend(wb1, 1, datatype, partner_rank, 1, comm, &req); // send total sum wb1
            MPI_Recv(wb3, 1, datatype, partner_rank, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE); // receive sum wb3
            MPI_Wait(&req, MPI_STATUS_IGNORE);
            //prefix_func(wb3, wb1, wb1, 1); // add received sum to total sum
            if(partner_rank < rank) {
                prefix_func(wb3, wb1, wb1, 1); // add received sum to total sum
                if(identity_element) {
                    prefix_func(NULL, wb3, wb2, 1); // add received sum to identity element
                    identity_element = false;
                }else {
                    prefix_func(wb3, wb2, wb2, 1); // add received sum to prefix sum
                }
            }else {
                prefix_func(wb1, wb3, wb1, 1); // add received sum to total sum
            }
        }
    }
    if(rank != 0) {
        prefix_func(wb2, sendbuf, recvbuf, count);
    }
}


/*************************** DEFINE YOUR HELPER FUNCTIONS HERE ************************/

