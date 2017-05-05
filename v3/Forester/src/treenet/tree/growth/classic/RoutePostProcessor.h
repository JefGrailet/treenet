/*
 * RoutePostProcessor.h
 *
 *  Created on: Mar 27, 2017
 *      Author: jefgrailet
 *
 * This class is dedicated to the detection of route stretching and route cycling among routes 
 * to the subnets and mitigate both phenomenons as much as possible, as they worsen neighborhood 
 * inference by causing real-life neighborhoods to be split across several branches.
 *
 * Route stretching is the phenomenon of observing a same IP in several routes, but at different 
 * hop counts. This is likely caused by traffic engineering strategies. This problem can hopefully 
 * be mitigated by evaluating the stretch length in order to re-build routes at tree growth such 
 * that a same IP only occurs at a same unique hop count.
 *
 * Route cycling is a rarer phenomenon in which a same interface occurs several times in a 
 * same route, again likely caused by traffic engineering strategies. It can be mitigated too by 
 * removing the route segment that cycles and replacing it with the cycled IP.
 *
 * This class also mitigates another phenomenon: the "fake" last hop. If the subnet is accurate or 
 * odd and the last hop belongs to the subnet itself without being a contra-pivot IP of the same 
 * subnet, then this last hop was likely edited by the last hop router. Some routers (Cisco or 
 * Juniper) reply for IPs they provide access to and before decrementing the TTL value, hiding the 
 * interface that actually replied in the process. It therefore hides an interface in the process 
 * (which cannot be seen) and causes the last hop to be the probed IP. This problem is mitigated 
 * by removing the last hop (the before last hop should belong to the incriminated router as well).
 *
 * The goal of putting this part of the program in a separate class rather than in ClassicGrower 
 * is to both isolate this part of the processing and be able to easily insert it in another piece 
 * of software which handles similar structures.
 *
 * Route post-processing is 100% offline work.
 */

#ifndef ROUTEPOSTPROCESSOR_H_
#define ROUTEPOSTPROCESSOR_H_

#include "../../../TreeNETEnvironment.h"

class RoutePostProcessor
{
public:
    
    RoutePostProcessor(TreeNETEnvironment *env); // Access to subnets and IP table through env
    ~RoutePostProcessor();
    
    void process();
    
private:
    
    TreeNETEnvironment *env;
    bool printSteps; // To display steps of each route post-processing (slightly verbose mode)
    
    /*
     * Detection of routing anomalies and their mitigation are separated in two private methods 
     * for the sake of clarity.
     */
    
    void detect();
    void mitigate();
    
    /*
     * Additionnal methods to analyze the route of each subnet and what kind of post-processing 
     * each route needs.
     */
    
    void checkForCycles(SubnetSite *ss);
    unsigned short countCycles(SubnetSite *ss, unsigned short *longest);
    
    bool needsPostProcessing(SubnetSite *ss);
    bool needsLastHopCorrection(SubnetSite *ss);
    bool needsCyclingMitigation(SubnetSite *ss);
    bool needsStretchingMitigation(SubnetSite *ss);
    
    bool hasCycle(RouteInterface *route, unsigned short size);
    bool hasStretch(RouteInterface *route, unsigned short size);
    
    /*
     * Method to find the route prefix of the soonest occurrence (in TTL) of a stretched route 
     * hop among all routes. The size of that prefix is passed by pointer.
     */
    
    RouteInterface *findPrefix(InetAddress stretched, unsigned short *size);
};

#endif /* ROUTEPOSTPROCESSOR_H_ */
