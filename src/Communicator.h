/*
 * Communicator.h
 *
 *  Created on: Oct 15, 2014
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 */

#ifndef COMMUNICATOR_H_
#define COMMUNICATOR_H_

#include <set>
#include <mpi.h>

typedef int Rank;

class Communicator {
private:
	std::set<Rank> members;

public:
	Communicator(std::set<Rank> &s) : members(s) {};
	bool isMember(const Rank &r) const;

	//bool operator == (const Communicator &c);
	//bool operator < (const Communicator &c);

};


#endif /* COMMUNICATOR_H_ */
