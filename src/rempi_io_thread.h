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
#ifndef __REMPI_IO_THREAD_H__
#define __REMPI_IO_THREAD_H__

#include <string>
#include <fstream>

#include "rempi_event.h"
#include "rempi_event_list.h"
#include "rempi_thread.h"
#include "rempi_encoder.h"

using namespace std;

class rempi_io_thread : public rempi_thread
{
	private:
		static rempi_mutex mtx;
		/*1 if this MPI process finish pushing events*/
		/*1 if this thread read reacord file to the end*/
		int read_finished;
		rempi_event_list<rempi_event*> *recording_events;
		rempi_event_list<rempi_event*> *replaying_events;
		string id;
		string record_path;
		int mode;
		rempi_encoder *encoder;
	protected:
		virtual void run();
		void write_record();
		void read_record();
		void read_record_lite();
	public:

                rempi_io_thread(rempi_event_list<rempi_event*> *recording_events, 
				rempi_event_list<rempi_event*> *replaying_events, 
				int mode, rempi_encoder *mc_encoder);
		~rempi_io_thread();
		void complete_flush();
};

#endif
