/* ==========================ReMPI:LICENSE==========================================   
   Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
   Produced at the Lawrence Livermore National Laboratory.                            
                                                                                       
   Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711357.                           
   All rights reserved.                                                               
                                                                                       
   This file is part of ReMPI. For details, see https://github.com/PRUNER/ReMPI       
   Please also see the LICENSE file for our notice and the LGPL.                      
                                                                                       
   This program is free software; you can redistribute it and/or modify it under the   
   terms of the GNU General Public License (as published by the Free Software         
   Foundation) version 2.1 dated February 1999.                                       
                                                                                       
   This program is distributed in the hope that it will be useful, but WITHOUT ANY    
   WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
   FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
   General Public License for more details.                                           
                                                                                       
   You should have received a copy of the GNU Lesser General Public License along     
   with this program; if not, write to the Free Software Foundation, Inc., 59 Temple   
   Place, Suite 330, Boston, MA 02111-1307 USA
   ============================ReMPI:LICENSE========================================= */
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

