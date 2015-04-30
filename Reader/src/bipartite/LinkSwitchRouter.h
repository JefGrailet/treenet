/*
 * LinkSwitchRouter.h
 *
 *  Created on: Mar 6, 2015
 *      Author: grailet
 *
 * The name of this class is self-explanatory: it represents a link between a switch and a router 
 * in a bipartite graph.
 */

#ifndef LINKSWITCHROUTER_H_
#define LINKSWITCHROUTER_H_

#include <string>
using std::string;

#include "BipartiteSwitch.h"
#include "BipartiteRouter.h"

// Forward declarations
class BipartiteSwitch;
class BipartiteRouter;

class LinkSwitchRouter
{
public:

	LinkSwitchRouter(BipartiteSwitch *eSwitch, BipartiteRouter *router);
	~LinkSwitchRouter();

	// Accessors
	inline BipartiteSwitch *getSwitch() { return eSwitch; }
	inline BipartiteRouter *getRouter() { return router; }
	
	// Returns a notation for this link (e.g. "E1 - R2")
	string toString();
	
private:

    // Associated router and subnet
    BipartiteSwitch *eSwitch; // The e avoids confusion with the "switch" instruction
    BipartiteRouter *router;

};

#endif /* LINKSWITCHROUTER_H_ */

