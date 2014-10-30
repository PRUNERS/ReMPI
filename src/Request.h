/*
 * Request.h
 *
 *  Created on: Oct 15, 2014
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include "Communicator.h"

typedef int Tag;

class Request {
private:
	Rank src;
	Tag tag;
	Communicator comm;
public:

	Request(const Rank &r, const Tag &t, const Communicator &c) :
		src(r), tag(t), comm(c)
	{}

};


#endif /* REQUEST_H_ */
