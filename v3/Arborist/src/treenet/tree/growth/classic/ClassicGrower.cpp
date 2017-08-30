/*
 * ClassicGrower.cpp
 *
 *  Created on: Sept 9, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in ClassicGrower.h (see this file to learn further about the goals 
 * of such class).
 */

#include <list>
using std::list;

#include "../../../../common/thread/Thread.h"
#include "ClassicGrower.h"
#include "ParisTracerouteTask.h"
#include "AnonymousChecker.h"
#include "RoutePostProcessor.h"

ClassicGrower::ClassicGrower(TreeNETEnvironment *env) : Grower(env)
{
    /*
     * About maxDepth parameter: it is the size of the longest route to a subnet which should be 
     * inserted in the tree. It is used as the size of the depthMap array (i.e. one list of nodes 
     * per depth level), which should be maintained throughout the life of the tree to ease the 
     * insertion step (re-building the whole map at each insertion is costly).
     */

    unsigned short maxDepth = env->getSubnetSet()->getMaximumDistance();
    this->depthMap = new list<NetworkTreeNode*>[maxDepth];
    this->tree = NULL;
}

ClassicGrower::~ClassicGrower()
{
    delete[] depthMap;
}

unsigned int ClassicGrower::countIncompleteRoutes()
{
    list<SubnetSite*> *ssList = env->getSubnetSet()->getSubnetSiteList();
    unsigned int nbIncompleteRoutes = 0;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        if(curSubnet->getRoute() != NULL)
        {
            if(curSubnet->hasIncompleteRoute())
                nbIncompleteRoutes++;
        }
    }
    return nbIncompleteRoutes;
}

unsigned short ClassicGrower::repairRouteOffline(SubnetSite *ss)
{
    unsigned short routeSize = ss->getRouteSize();
    RouteInterface *route = ss->getRoute();
    list<SubnetSite*> *ssList = env->getSubnetSet()->getSubnetSiteList();
    
    /*
     * Exceptional case: there is only one hop. In that case, we fix it only if there is a single 
     * option among all routes (except when these routes are incomplete as well).
     */
    
    if(routeSize == 1)
    {
        list<InetAddress> options;
        for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        {
            SubnetSite *ss2 = (*it);
            unsigned short routeSize2 = ss2->getRouteSize();
            if(routeSize2 > 0)
            {
                RouteInterface *route2 = ss2->getRoute();
                options.push_back(route2[0].ip);
            }
        }
        
        // Filtering out duplicates
        options.sort(InetAddress::smaller);
        InetAddress prev(0);
        for(list<InetAddress>::iterator it = options.begin(); it != options.end(); ++it)
        {
            if((*it) == prev)
                options.erase(it--);
            else
                prev = (*it);
        }
        
        if(options.size() == 1)
        {
            route[0].repair(options.front());
            return 1;
        }
    
        return 0;
    }
    
    unsigned short nbReplacements = 0;
    for(unsigned short i = 0; i < routeSize - 1; i++) // We don't fix offline a last hop (risky)
    {
        if(route[i].ip != InetAddress(0))
            continue;
        
        InetAddress hopBefore(0), hopAfter(0);
        if(i > 0)
            hopBefore = route[i - 1].ip;
        hopAfter = route[i + 1].ip;
        
        if((i > 0 && hopBefore == InetAddress(0)) || hopAfter == InetAddress(0))
            continue;
        
        // Lists the options
        list<InetAddress> options;
        for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        {
            SubnetSite *ss2 = (*it);
            unsigned short routeSize2 = ss2->getRouteSize();
            if(routeSize2 > i + 1)
            {
                RouteInterface *route2 = ss2->getRoute();
                if(route2[i].ip == InetAddress(0))
                    continue;
                if((i == 0 || hopBefore == route2[i - 1].ip) && hopAfter == route2[i + 1].ip)
                    options.push_back(route2[i].ip);
            }
        }
        
        // Filtering out duplicates
        options.sort(InetAddress::smaller);
        InetAddress prev(0);
        for(list<InetAddress>::iterator it = options.begin(); it != options.end(); ++it)
        {
            if((*it) == prev)
                options.erase(it--);
            else
                prev = (*it);
        }
        
        // Replaces if and only if there is a single option.
        if(options.size() == 1)
        {
            route[i].repair(options.front());
            nbReplacements++;
        }
    }
    
    return nbReplacements;
}

void ClassicGrower::prepare()
{
    /*
     * PARIS TRACEROUTE
     *
     * The preparation of the classic growth method consists in computing the route to each 
     * inferred subnet, no matter how it was classified during previous algorithmic steps.
     *
     * The route computation itself is base on the same ideas as Paris Traceroute, and is 
     * parallelized to quickly obtain all routes.
     */
    
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnets = env->getSubnetSet();
    unsigned short nbThreads = env->getMaxThreads();
    unsigned short displayMode = env->getDisplayMode();
    
    list<SubnetSite*> *ssList = subnets->getSubnetSiteList();

    (*out) << "Getting the route to each subnet...\n" << endl;
    
    // Lists subnets for which we would like a route
    list<SubnetSite*> toSchedule;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        unsigned short status = (*it)->getStatus();
        if(status == SubnetSite::ACCURATE_SUBNET || 
           status == SubnetSite::SHADOW_SUBNET || 
           status == SubnetSite::ODD_SUBNET)
        {
            toSchedule.push_back((*it));
        }
    }
    
    // Size of the thread array
    unsigned short sizeParisArray = 0;
    if((unsigned long) toSchedule.size() > (unsigned long) nbThreads)
        sizeParisArray = nbThreads;
    else
        sizeParisArray = (unsigned short) toSchedule.size();
    
    // Creates thread(s)
    Thread **th = new Thread*[sizeParisArray];
    for(unsigned short i = 0; i < sizeParisArray; i++)
        th[i] = NULL;
    
    while(toSchedule.size() > 0)
    {
        unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
        range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
        range /= sizeParisArray;
        
        for(unsigned short i = 0; i < sizeParisArray && toSchedule.size() > 0; i++)
        {
            SubnetSite *curSubnet = toSchedule.front();
            toSchedule.pop_front();
            
            unsigned short lowBound = (i * range);
            unsigned short upBound = lowBound + range - 1;
            
            Runnable *task = NULL;
            try
            {
                task = new ParisTracerouteTask(env, 
                                               curSubnet, 
                                               DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowBound, 
                                               DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upBound, 
                                               DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                               DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);

                th[i] = new Thread(task);
            }
            catch(SocketException &se)
            {
                // Cleaning remaining threads (if any is set)
                for(unsigned short j = 0; j < sizeParisArray; j++)
                {
                    delete th[j];
                    th[j] = NULL;
                }
                
                delete[] th;
                
                throw StopException();
            }
            catch(ThreadException &te)
            {
                (*out) << "Unable to create more threads." << endl;
            
                delete task;
            
                // Cleaning remaining threads (if any is set)
                for(unsigned short j = 0; j < sizeParisArray; j++)
                {
                    delete th[j];
                    th[j] = NULL;
                }
                
                delete[] th;
                
                throw StopException();
            }
        }

        // Launches thread(s) then waits for completion
        for(unsigned short i = 0; i < sizeParisArray; i++)
        {
            if(th[i] != NULL)
            {
                th[i]->start();
                Thread::invokeSleep(env->getProbeThreadDelay());
            }
        }
        
        for(unsigned short i = 0; i < sizeParisArray; i++)
        {
            if(th[i] != NULL)
            {
                th[i]->join();
                delete th[i];
                th[i] = NULL;
            }
        }
        
        if(env->isStopping())
            break;
    }
    
    delete[] th;
    
    /*
     * If we are in laconic display mode, we add a line break before the next message to keep 
     * the display airy enough.
     */
    
    if(displayMode == TreeNETEnvironment::DISPLAY_MODE_LACONIC)
        (*out) << "\n";
    
    if(env->isStopping())
    {
        throw StopException();
    }
    
    (*out) << "Finished probing to get routes." << endl;
    
    env->recordRouteStepsInDictionnary();
    
    /*
     * ROUTE REPAIRMENT
     * 
     * Because of security policies, there are several possibilities of getting an incomplete 
     * route. A route is said to be incomplete when one or several hops (consecutive or not) 
     * could not be obtained because of a timeout (note: the code let also the possibility that 
     * the reply gives 0.0.0.0 as replying interface, though this is unlikely). Timeouts in this 
     * context can be caused by:
     * -permanently "anonymous" routers (routers that route packets, but drop packets when TTL 
     *  expires without a reply), 
     * -a router already identified via previous traceroute and which stops replying after a 
     *  certain rate of probes, 
     * -a firewall which does the same, more or less.
     *
     * Before getting to the growth of the tree, one should mitigate these "holes" as much as 
     * possible, because taking 0.0.0.0 in or out of the insertion point search can drastically 
     * change the structure of the final tree. Therefore:
     * 1) the code first checks if there are subnets with incomplete routes at all.
     * 2) then, it checks if there are any "unavoidable" missing hop. For instance, if the third 
     *    step of all routes (min. length = 9, for instance) is 0.0.0.0, this cannot be fixed.
     *    These steps are then replaced by placeholders IPs from 0.0.0.0/24 to continue.
     * 3) the code performs offline repairment, i.e., for each missing interface, it looks for a 
     *    similar route (i.e., a route where the hops just before and after are identical) or 
     *    checks what is the typical interface at the same amount of hops. When there is one and 
     *    only one possibility, the missing hop is replaced by the typical interface.
     * 4) if there remains missing interfaces, the code reprobes at the right TTL each route 
     *    target for each incomplete route. A delay of one minute is left between each reprobing 
     *    and the operation is carried out 3 times.
     * 
     * The process stops once there are no longer incomplete routes (besides unavoidable anonymous 
     * hops), so not all steps mentioned above will necessarily be performed. No matter when it 
     * stops, all placeholder IPs are changed back to 0.0.0.0 and the route post-processing phase 
     * is launched (i.e., detecting and mitigating last hop issue and route cycling/stretching).
     */
    
    // Step 1
    unsigned int nbIncomplete = countIncompleteRoutes();
    if(nbIncomplete == 0)
    {
        (*out) << "All routes are complete.\n" << endl;
        this->postProcessRoutes();
        return;
    }
    
    (*out) << "There are incomplete routes." << endl;
    
    // Step 2
    unsigned short minLength = 255;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        if((*it)->getRouteSize() > 0 && (*it)->getRouteSize() < minLength)
            minLength = (*it)->getRouteSize();
    
    unsigned short permanentlyAnonymous = 0;
    for(unsigned short i = 0; i < minLength; i++)
    {
        bool anonymous = true;
        for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        {
            if((*it)->hasValidRoute())
            {
                RouteInterface *routeSs = (*it)->getRoute();
                if(routeSs[i].ip != InetAddress(0))
                {
                    anonymous = false;
                    break;
                }
            }
        }
        
        if(anonymous)
        {
            permanentlyAnonymous++;
            for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
            {
                if((*it)->hasValidRoute())
                {
                    RouteInterface *routeSs = (*it)->getRoute();
                    routeSs[i].ip = InetAddress(permanentlyAnonymous);
                }
            }
        }
    }
    
    if(permanentlyAnonymous > 0)
    {
        if(permanentlyAnonymous > 1)
        {
            (*out) << "Found " << permanentlyAnonymous << " unavoidable missing hops. ";
            (*out) << "These hops will be considered as regular interfaces during repairment." << endl;
        }
        else
        {
            (*out) << "Found one unavoidable missing hop. This hop will be considered as a ";
            (*out) << "regular interface during repairment." << endl;
        }
    }
    
    nbIncomplete = countIncompleteRoutes();
    if(nbIncomplete == 0)
    {
        (*out) << "There is no other missing hop. No repairment will occur.\n" << endl;
        this->postProcessRoutes();
        return;
    }
    
    // Step 3
    if(nbIncomplete > 1)
        (*out) << "Found " << nbIncomplete << " incomplete routes. Starting offline repairment..." << endl;
    else
        (*out) << "Found one incomplete route. Starting offline repairment..." << endl;
    
    unsigned int nbRepairments = 0; // Amount of 0.0.0.0's being replaced
    unsigned int fullyRepaired = 0; // Amount of routes fully repaired
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *curSubnet = (*it);
        if(curSubnet->getRoute() != NULL)
        {
            if(curSubnet->hasIncompleteRoute())
            {
                nbRepairments += this->repairRouteOffline(curSubnet);
                if(curSubnet->hasCompleteRoute())
                    fullyRepaired++;
            }
        }
    }
     
    if(nbRepairments == 0)
    {
        (*out) << "Could not fix incomplete routes with offline repairment." << endl;
    }
    else
    {
        if(nbRepairments > 1)
            (*out) << "Repaired " << nbRepairments << " hops." << endl;
        else
            (*out) << "Repaired a single hop." << endl;
        
        if(fullyRepaired > 1)
            (*out) << "Fully repaired " << fullyRepaired << " routes." << endl;
        else if(fullyRepaired == 1)
            (*out) << "Fully repaired one route." << endl;
    
        nbIncomplete = countIncompleteRoutes();
        if(nbIncomplete == 0)
        {
            (*out) << "All routes are now complete.\n" << endl;
            this->postProcessRoutes();
            return;
        }
    }
    
    // Step 4
    if(nbIncomplete > 1)
        (*out) << "There remain " << nbIncomplete << " incomplete routes. Starting online repairment..." << endl;
    else
        (*out) << "There remains one incomplete route. Starting online repairment..." << endl;
     
    nbRepairments = 0;
    fullyRepaired = 0;
    AnonymousChecker *checker = NULL;
    
    Thread::invokeSleep(TimeVal(60, 0)); // Pause of 1 minute before probing again
    
    try
    {
        checker = new AnonymousChecker(env);
        checker->probe();
        
        nbRepairments = checker->getTotalSolved();
        fullyRepaired = checker->getTotalFullyRepaired();
        
        float ratioSolved = checker->getRatioSolvedHops();
        
        if(ratioSolved > 0.4)
        {
            (*out) << "Repaired " << (ratioSolved * 100) << "\% of missing hops.";
            
            if(ratioSolved < 1.0)
            {
                (*out) << " Starting a second opinion..." << endl;
                
                Thread::invokeSleep(TimeVal(60, 0));
                
                checker->reload();
                checker->probe();
            }
            else
                (*out) << endl;
            
            nbRepairments += checker->getTotalSolved();
            fullyRepaired += checker->getTotalFullyRepaired();
        }
        
        delete checker;
        checker = NULL;
    }
    catch(StopException &se)
    {
        if(checker != NULL)
        {
            delete checker;
            checker = NULL;
        }
        throw StopException();
    }
    
    if(nbRepairments == 0)
    {
        (*out) << "Could not fix incomplete routes with online repairment." << endl;
    }
    else
    {
        if(nbRepairments > 1)
            (*out) << "Repaired " << nbRepairments << " hops." << endl;
        else
            (*out) << "Repaired a single hop." << endl;
        
        if(fullyRepaired > 1)
            (*out) << "Fully repaired " << fullyRepaired << " routes." << endl;
        else if(fullyRepaired == 1)
            (*out) << "Fully repaired one route." << endl;
    
        nbIncomplete = countIncompleteRoutes();
        if(nbIncomplete == 0)
        {
            (*out) << "All routes are now complete.\n" << endl;
            this->postProcessRoutes();
            return;
        }
    }
    
    (*out) << endl; // Additionnal line break before analyzing route anomalies
    this->postProcessRoutes();
}

void ClassicGrower::postProcessRoutes()
{
    // Before actual post-processing, changes placeholder IPs back to 0.0.0.0
    SubnetSiteSet *subnets = env->getSubnetSet();
    list<SubnetSite*> *ssList = subnets->getSubnetSiteList();
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        unsigned short routeSize = (*it)->getRouteSize();
        if(routeSize > 0)
        {
            RouteInterface *routeSs = (*it)->getRoute();
            for(unsigned short i = 0; i < routeSize; ++i)
            {
                if(routeSs[i].ip >= InetAddress(1) && routeSs[i].ip <= InetAddress(255))
                    routeSs[i].ip = InetAddress(0);
            }
        }
    }

    RoutePostProcessor *postProcessor = new RoutePostProcessor(env);
    postProcessor->process();
    delete postProcessor;
}

void ClassicGrower::grow()
{
    ostream *out = env->getOutputStream();
    SubnetSiteSet *subnets = env->getSubnetSet();

    subnets->removeArtifacts();
    subnets->sortByRoute();
    
    this->tree = new NetworkTree();

    // Inserting subnets with a complete route first
    SubnetSite *toInsert = subnets->getValidSubnet();
    while(toInsert != NULL)
    {
        this->insert(toInsert);
        toInsert = subnets->getValidSubnet();
    }
    (*out) << "Subnets with complete route inserted." << endl;
    
    // Then, subnets with an incomplete route even after repairment/post-processing
    toInsert = subnets->getValidSubnet(false);
    while(toInsert != NULL)
    {
        this->insert(toInsert);
        toInsert = subnets->getValidSubnet(false);
    }
    (*out) << "Subnets with incomplete route inserted." << endl;
    
    // Creates the new Soil object and adds the produced tree in it
    this->result = new Soil();
    this->result->insertTree(this->tree);
    
    // Adds the subnet map entries in the Soil object
    this->result->insertMapEntries(newSubnetMapEntries);
    this->result->sortMapEntries();
    newSubnetMapEntries.clear();
}

void ClassicGrower::insert(SubnetSite *subnet)
{
    // Gets root of the tree
    NetworkTreeNode *rootNode = this->tree->getRoot();
    list<NetworkTreeNode*> *map = this->depthMap;

    // Gets (final) route information of the new subnet
    unsigned short routeSize;
    RouteInterface *route = subnet->getFinalRoute(&routeSize);
    
    // Finds the deepest node which occurs in the route of current subnet
    NetworkTreeNode *insertionPoint = NULL;
    unsigned insertionPointDepth = 0;
    for(unsigned short d = routeSize; d > 0; d--)
    {
        // if(route[d - 1].state == RouteInterface::MISSING || route[d - 1].state == RouteInterface::ANONYMOUS)
            // continue;
    
        for(list<NetworkTreeNode*>::iterator i = map[d - 1].begin(); i != map[d - 1].end(); ++i)
        {
            if((*i)->hasLabel(route[d - 1].ip))
            {
                insertionPoint = (*i);
                insertionPointDepth = d;
                break;
            }
        }
        
        if(insertionPoint != NULL)
            break;
    }
    
    if(insertionPoint == NULL)
        insertionPoint = rootNode;
    
    // Creates the new branch
    NetworkTreeNode *subTree = createBranch(subnet, insertionPointDepth + 1);
    if(insertionPoint != rootNode)
        subTree->addPreviousLabel(route[insertionPointDepth - 1].ip);
    insertionPoint->addChild(subTree);

    // Puts the new internal nodes inside the depth map
    NetworkTreeNode *next = subTree;
    NetworkTreeNode *subnetNode = NULL; // For subnet map entry
    if(next->isInternal())
    {
        unsigned short curDepth = insertionPointDepth;
        do
        {
            map[curDepth].push_back(next);
            curDepth++;
            
            list<NetworkTreeNode*> *children = next->getChildren();
            if(children != NULL && children->size() == 1)
            {
                if(children->front()->isInternal())
                {
                    next = children->front();
                }
                else
                {
                    subnetNode = children->front();
                    next = NULL;
                }
            }
            else
                next = NULL;
        }
        while(next != NULL);
    }
    
    /*
     * If nodes "above" the insertion point do not share the exact same route as the one we 
     * obtained for this subnet, we add the interfaces at a same TTL (i.e. depth) that differ 
     * from what we already have.
     */
    
    NetworkTreeNode *cur = insertionPoint->getParent();
    NetworkTreeNode *child = insertionPoint;
    for(unsigned short d = insertionPointDepth; d > 1; d--)
    {
        child->addPreviousLabel(route[d - 2].ip);
    
        if(!cur->hasLabel(route[d - 2].ip))
        {
            cur->addLabel(route[d - 2].ip);
            
            /*
             * Look in depth map for a node at same depth sharing the new label. Indeed, if such
             * node exists, we should merge it with the current node, otherwise we will have 2 
             * nodes sharing a same label in the tree, which should not occur if we want the tree 
             * to be fidel to the topology.
             */
            
            NetworkTreeNode *toMerge = NULL;
            for(list<NetworkTreeNode*>::iterator i = map[d - 2].begin(); i != map[d - 2].end(); ++i)
            {
                if((*i) != cur && (*i)->hasLabel(route[d - 2].ip))
                {
                    toMerge = (*i);
                    break;
                }
            }
            
            if(toMerge != NULL)
            {
                cur->merge(toMerge);
                
                // Add labels in toMerge absent from cur
                list<InetAddress> *labels1 = cur->getLabels();
                list<InetAddress> *labels2 = toMerge->getLabels();
                labels1->merge((*labels2));
                labels1->sort(InetAddress::smaller);
                InetAddress prev(0);
                bool first = true; // Avoids incoherent behavior with labels 0.0.0.0
                for(list<InetAddress>::iterator i = labels1->begin(); i != labels1->end(); ++i)
                {
                    if(!first)
                    {
                        if((*i) == prev)
                        {
                            labels1->erase(i--);
                        }
                    }
                    prev = (*i);
                    first = false;
                }
                
                this->prune(toMerge, NULL, d - 2);
                
                // Sorts parent's children to keep the good order (it can change after merging)
                cur->getParent()->sortChildren();
            }
        }
    
        child = cur;
        cur = cur->getParent();
    }
    
    // Last step: creates an entry for the subnet map (for look-up)
    newSubnetMapEntries.push_back(new SubnetMapEntry(subnet, tree, subnetNode));
}

void ClassicGrower::prune(NetworkTreeNode *cur, NetworkTreeNode *prev, unsigned short depth)
{
    list<NetworkTreeNode*> *map = this->depthMap;

    if(cur->isLeaf())
    {
        if(prev != NULL)
            delete prev;
        return;
    }
    else if(cur->isRoot())
    {
        if(prev != NULL)
        {
            list<NetworkTreeNode*> *children = cur->getChildren();
            for(list<NetworkTreeNode*>::iterator i = map[0].begin(); i != map[0].end(); ++i)
            {
                if((*i) == prev)
                {
                    map[0].erase(i);
                    break;
                }
            }
        
            // Erases prev from the children list (of this node) and stops
            for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
            {
                if((*i) == prev)
                {
                    delete (*i);
                    children->erase(i);
                    return;
                }
            }
        
            delete prev;
        }
        return;
    }
    
    list<NetworkTreeNode*> *children = cur->getChildren();

    // Current node has multiple children; keep it but remove prev
    if(children->size() > 1)
    {
        // Erases prev from the depth map
        unsigned dPrev = depth + 1;
        for(list<NetworkTreeNode*>::iterator i = map[dPrev].begin(); i != map[dPrev].end(); ++i)
        {
            if((*i) == prev)
            {
                map[dPrev].erase(i);
                break;
            }
        }
    
        // Erases prev from the children list (of this node) and stops
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            if((*i) == prev)
            {
                delete (*i);
                children->erase(i);
                return;
            }
        }
    }
    // Current node has a single child: prev. Deletes it and moves up in the tree.
    else if(children->size() == 1)
    {
        // Erases prev from the depth map
        unsigned dPrev = depth + 1;
        for(list<NetworkTreeNode*>::iterator i = map[dPrev].begin(); i != map[dPrev].end(); ++i)
        {
            if((*i) == prev)
            {
                map[dPrev].erase(i);
                break;
            }
        }
    
        delete prev;
        cur->getChildren()->clear();
        this->prune(cur->getParent(), cur, depth - 1);
    }
    // Current node has no longer children. Moves up in the tree.
    else if(children->size() == 0)
    {
        this->prune(cur->getParent(), cur, depth - 1);
    }
}

NetworkTreeNode *ClassicGrower::createBranch(SubnetSite *subnet, unsigned short depth)
{
    unsigned short routeSize = 0;
    RouteInterface *route = subnet->getFinalRoute(&routeSize);
    
    // If current depth minus 1 equals the route size, then we just have to create a leaf
    if((depth - 1) == routeSize)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(subnet);
        return newNode;
    }

    // Creates the branch for this new subnet, as a cascade of new nodes with missing interfaces
    NetworkTreeNode *newRoot = new NetworkTreeNode(route[depth - 1].ip);
    NetworkTreeNode *curNode = newRoot;
    for(int i = depth + 1; i <= routeSize; i++)
    {
        NetworkTreeNode *newNode = new NetworkTreeNode(route[i - 1].ip);
        newNode->addPreviousLabel(route[i - 2].ip);
        curNode->addChild(newNode);
        curNode = newNode;
    }
    curNode->addChild(new NetworkTreeNode(subnet));
    
    return newRoot;
}

void ClassicGrower::listSubnetsRecursive(list<SubnetSite*> *subnetsList, NetworkTreeNode *cur)
{
    // Subnets are stored in leaves
    if(cur->isLeaf())
    {
        subnetsList->push_back(cur->getAssociatedSubnet());
        return;
    }
    
    // Goes deeper in the tree
    list<NetworkTreeNode*> *children = cur->getChildren();
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        listSubnetsRecursive(subnetsList, (*i));
    }
}
