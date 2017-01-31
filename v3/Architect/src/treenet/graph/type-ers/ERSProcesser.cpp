/*
 * ERSProcesser.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in ERSProcesser.h (see this file to learn further about the goals 
 * of such class).
 */

#include <fstream>
using std::ofstream;
#include <sstream>
using std::stringstream;
#include <sys/stat.h> // For CHMOD edition

#include "ERSProcesser.h"
#include "proj-r/RouterGraph.h"
#include "proj-s/SubnetGraph2.h"

ERSProcesser::ERSProcesser(TreeNETEnvironment *env)
{
    this->env = env;
    this->soilRef = NULL;
    this->ERResult = NULL;
    this->RSResult = NULL;
}

ERSProcesser::~ERSProcesser()
{
}

void ERSProcesser::process(Soil *fromSoil)
{
    this->soilRef = fromSoil;
    this->ERResult = new ERGraph();
    this->RSResult = new RSGraph();
    list<NetworkTree*> *roots = fromSoil->getRootsList();
    
    if(roots->size() > 1)
    {
        for(list<NetworkTree*>::iterator i = roots->begin(); i != roots->end(); ++i)
        {
            this->skipTrunkAndStart((*i));
        }
    }
    else if(roots->size() == 1)
    {
        this->skipTrunkAndStart(roots->front());
    }
    
    this->ERResult->sortEdges();
    this->RSResult->sortEdges();
    this->soilRef = NULL;
}

void ERSProcesser::output(string filename)
{
    if(this->ERResult == NULL || this->RSResult == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error: no graph to output as \"" << filename << "\"." << endl;
        return;
    }

    stringstream output;
    output << this->RSResult->partyOneToString() << "\n";
    output << this->RSResult->partyTwoToString() << "\n";
    if(this->ERResult->getEdges()->size() > 0)
        output << this->ERResult->edgesToString() << "\n";
    else
        output << "No edge on ER side.\n\n";
    output << this->RSResult->edgesToString() << "\n";
    
    output << "L2 (ethernet switch) - L3 (router) bipartite graph metrics\n\n";
    output << this->ERResult->getMetricsString(false) << "\n";
    
    output << "L3 (router) - subnet bipartite graph metrics\n\n";
    output << this->RSResult->getMetricsString();
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    // Cleans produced graphs
    delete this->ERResult;
    this->ERResult = NULL;
    
    delete this->RSResult;
    this->RSResult = NULL;
}

double ERSProcesser::check()
{
    if(this->ERResult == NULL || this->RSResult == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error (check() method): no complete ERS graph to check." << endl;
        return 0.0;
    }

    list<Vertice*> *eswitches = this->ERResult->getPartyOne();
    list<Vertice*> *routers = this->RSResult->getPartyOne();
    list<Vertice*> *subnets = this->RSResult->getPartyTwo();
    
    // N.B.: there can be no switch in an ERS graph.
    if(routers->size() == 0 || subnets->size() == 0)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error (check() method): one of the parties in RS graph is empty." << endl;
        return 0.0;
    }
    
    // Sets the arrays (and their size) used for checking
    nESwitches = eswitches->size();
    if(nESwitches > 0)
    {
        checkArrE = new bool[nESwitches];
        for(unsigned int i = 0; i < nESwitches; i++)
            checkArrE[i] = false;
    }
    
    nRouters = routers->size();
    checkArrR = new bool[nRouters];
    for(unsigned int i = 0; i < nRouters; i++)
        checkArrR[i] = false;
    
    nSubnets = subnets->size();
    checkArrS = new bool[nSubnets];
    for(unsigned int i = 0; i < nSubnets; i++)
        checkArrS[i] = false;
    
    // Prepares the maps for the routers in both graphs
    routersER = new L3Device*[nRouters];
    routersRS = new L3Device*[nRouters];
    list<Vertice*> *routersBis = this->ERResult->getPartyTwo();
    list<Vertice*>::iterator it1 = routersBis->begin();
    list<Vertice*>::iterator it2 = routers->begin();
    for(unsigned int i = 0; i < nRouters; i++)
    {
        routersER[i] = (L3Device*) (*it1);
        routersRS[i] = (L3Device*) (*it2);
    
        it1++;
        it2++;
    }
    
    // Visits the graph
    visit(routers->front());
    
    unsigned int visitedESwitches = 0;
    if(nESwitches > 0)
        for(unsigned int i = 0; i < nESwitches; i++)
            if(checkArrE[i])
                visitedESwitches++;
    
    unsigned int visitedRouters = 0;
    for(unsigned int i = 0; i < nRouters; i++)
        if(checkArrR[i])
            visitedRouters++;
    
    unsigned int visitedSubnets = 0;
    for(unsigned int i = 0; i < nSubnets; i++)
        if(checkArrS[i])
            visitedSubnets++;
    
    // Frees the arrays
    delete[] checkArrE;
    delete[] checkArrR;
    delete[] checkArrS;
    checkArrE = NULL;
    checkArrR = NULL;
    checkArrS = NULL;
    
    delete[] routersER;
    delete[] routersRS;
    routersER = NULL;
    routersRS = NULL;
    
    // Computes final result
    double res = (((double) (visitedESwitches + visitedRouters + visitedSubnets)) / ((double) (nESwitches + nRouters + nSubnets))) * 100;
    nESwitches = 0;
    nRouters = 0;
    nSubnets = 0;
    return res;
}

void ERSProcesser::outputSubnetProjection(string filename)
{
    if(this->ERResult == NULL || this->RSResult == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error: no graph to project on subnets in an output file ";
        (*out) << "\"" << filename << "\"." << endl;
        return;
    }
    
    SubnetGraph2 *projection = new SubnetGraph2(this->ERResult, this->RSResult);
    
    stringstream output;
    output << projection->verticesToString() << "\n";
    output << projection->edgesToString() << "\n";
    output << projection->getMetricsString();
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    // Deletes the projection
    delete projection;
}

void ERSProcesser::outputRouterProjection(string filename)
{
    if(this->ERResult == NULL || this->RSResult == NULL)
    {
        ostream *out = this->env->getOutputStream();
        (*out) << "Critical error: no graph to project on subnets in an output file ";
        (*out) << "\"" << filename << "\"." << endl;
        return;
    }
    
    RouterGraph *projection = new RouterGraph(this->ERResult, this->RSResult);
    
    stringstream output;
    output << projection->verticesToString() << "\n";
    output << projection->edgesToString() << "\n";
    output << projection->getMetricsString();
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    // Deletes the projection
    delete projection;
}

void ERSProcesser::skipTrunkAndStart(NetworkTree *tree)
{
    // Gets to the first internal node with more than one child.
    NetworkTreeNode *entryPoint = tree->getRoot();
    unsigned short depth = 0;
    while(entryPoint->getChildren()->size() == 1)
    {
        entryPoint = entryPoint->getChildren()->front();
        depth++;
    }
    
    // If the node is an Hedera, we will start from the first parent which is not one.
    while(entryPoint->isHedera())
    {
        entryPoint = entryPoint->getParent();
        depth--;
    }
    
    // Processes first the RS part, then the ER part (after copying routers of RS graph)
    this->processRecursiveRS(entryPoint, depth);
    this->ERResult->copyRouters(this->RSResult);
    this->processRecursiveER(entryPoint, depth);
}

void ERSProcesser::processRecursiveRS(NetworkTreeNode *cur, unsigned short depth)
{
    RSGraph *bipGraphRS = this->RSResult;
    list<NetworkTreeNode*> *children = cur->getChildren();
    list<InetAddress> *labels = cur->getLabels();
    list<Router*> *routers = cur->getInferredRouters();
    
    // Puts direct neighbor subnets in a list and puts the T_NEIGHBORHOOD nodes in another one
    list<SubnetSite*> childrenL;
    list<NetworkTreeNode*> childrenI;
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if((*i)->isLeaf())
            childrenL.push_back((*i)->getAssociatedSubnet());
        else if((*i)->isInternal())
            childrenI.push_back((*i));
    }
    list<SubnetSite*> backUpSubnets(childrenL);
    
    // Goes deeper in the tree first
    for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
    {
        this->processRecursiveRS((*i), depth + 1);
    }
    
    // Case where current node is an hedera (i.e., it has multiple labels)
    if(cur->isHedera())
    {
        for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
        {
            InetAddress ingressInterface = (*i);
            
            bool isMissingInterface = false;
            if(ingressInterface == InetAddress(0))
            {
                /* 
                 * !!! Unique case (fixed on January 31, 2017) !!!
                 *
                 * It is possible, but not common, that the last route hop to some subnets is 
                 * missing (this amounts to 0.0.0.0 as a InetAddress object). Simply skipping the
                 * case will result in the routers associated with these subnets not appearing in
                 * the final graph, while the same subnets will be connected with an imaginary 
                 * component. Therefore, we raise a special flag to keep running the next 
                 * instructions except those related to the ingressRouter.
                 */
                
                isMissingInterface = true;
            }
            
            Router *ingressRouter = NULL;
            if(!isMissingInterface)
            {
                ingressRouter = cur->getRouterHaving(ingressInterface);
            
                // The ingress router should exist by construction. If not, we skip to next label.
                if(ingressRouter == NULL)
                    continue;
            }
            
            // Lists children subnets which last label in the route is ingressInterface
            list<SubnetSite*> curChildrenL;
            for(list<SubnetSite*>::iterator j = childrenL.begin(); j != childrenL.end(); ++j)
            {
                if((*j)->hasRouteLabel(ingressInterface, depth))
                {
                    curChildrenL.push_back((*j));
                    childrenL.erase(j--);
                }
            }
            
            /*
             * Lists children internals which the routes to the leaves contain at least one 
             * occurrence of ingressInterface at this depth. Indeed, there is no point in creating 
             * a link between this router and other internals if the traceroutes used to build 
             * their respective branches do not match.
             */
            
            list<NetworkTreeNode*> curChildrenI;
            for(list<NetworkTreeNode*>::iterator j = childrenI.begin(); j != childrenI.end(); ++j)
            {
                if((*j)->hasPreviousLabel(ingressInterface))
                {
                    curChildrenI.push_back((*j));
                    
                    // No erasure, because this node might be accessed by another router
                }
            }
            
            if(!isMissingInterface)
                this->connectToRelatedSubnets(ingressRouter, &curChildrenL);
            
            /*
             * The code now looks for routers which give access to the remaining subnets. It will 
             * connect these subnets with the selected routers.
             */
            
            if(curChildrenL.size() > 0 && ((!isMissingInterface && routers->size() > 1) || (routers->size() > 0)))
            {
                for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                {
                    Router *curRouter = (*j);
                
                    if(!isMissingInterface && curRouter == ingressRouter)
                        continue;
                    
                    // Router must give access to at least one listed subnet to continue
                    bool givesAccessToASubnet = false;
                    for(list<SubnetSite*>::iterator k = curChildrenL.begin(); k != curChildrenL.end(); ++k)
                    {
                        if(this->areRelated(curRouter, (*k)))
                        {
                            givesAccessToASubnet = true;
                            break;
                        }
                    }
                    
                    if(givesAccessToASubnet)
                        this->connectToRelatedSubnets(curRouter, &curChildrenL);
                }
            }
            
            if(curChildrenL.size() > 0)
            {
                this->connectRemainingSubnets(cur, &curChildrenL);
            }
            
            // Connects with child internal nodes
            if(curChildrenI.size() > 0)
            {
                for(list<NetworkTreeNode*>::iterator j = curChildrenI.begin(); j != curChildrenI.end(); ++j)
                {
                    // Case where we have a "true" ingress router
                    if(!isMissingInterface)
                    {
                        this->connectToChildInternal(ingressRouter, backUpSubnets, (*j));
                    }
                    // Case where the current interface is a missing one
                    else
                    {
                        this->connectToChildInternal(cur, backUpSubnets, (*j));
                    }
                }
            }
        }
        
        // Connects the remaining subnets (if any) with an imaginary router.
        if(childrenL.size() > 0)
            for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
                bipGraphRS->createEdge(cur, (*i));
    }
    // Case where there is a single interface and at least one router
    else if(routers->size() > 0)
    {
        list<SubnetSite*> backUpSubnets(childrenL); // Copy of childrenL for last step
        InetAddress ingressInterface = labels->front();
        Router *ingressRouter = NULL; // Stays null if an imaginary router is required
        
        // Creates the router (+ links) corresponding to the single label of this node
        if(ingressInterface != InetAddress(0))
        {
            ingressRouter = cur->getRouterHaving(ingressInterface);
            
            if(ingressRouter == NULL) // If no router matching the label, we do the same as in the last case
            {
                for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
                    bipGraphRS->createEdge(cur, (*i));
            
                if(childrenI.size() > 0)
                    for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
                        this->connectToChildInternal(cur, childrenL, (*i));
                
                return;
            }
            
            this->connectToRelatedSubnets(ingressRouter, &childrenL);
        }
        
        // Creates router - subnet links of other routers (if any)
        if(routers->size() > 1 || (ingressInterface == InetAddress(0) && routers->size() == 1))
        {
            for(list<Router*>::iterator i = routers->begin(); i != routers->end(); ++i)
            {
                if((*i) == ingressRouter)
                    continue;
                this->connectToRelatedSubnets((*i), &childrenL);
            }
        }
        
        // Connects remaining subnets (if any) via an imaginary router (might already exist)
        if(childrenL.size() > 0)
        {
            this->connectRemainingSubnets(cur, &childrenL);
        }
        
        // Connects this neighborhood with the children that are internals
        if(childrenI.size() > 0)
        {
            for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
            {
                if(ingressRouter == NULL)
                    this->connectToChildInternal(cur, backUpSubnets, (*i));
                else
                    this->connectToChildInternal(ingressRouter, backUpSubnets, (*i));
            }
        }
    }
    // Case where there is no router at all
    else
    {
        // Connects all subnets to current neighborhood (via an imaginary router)
        for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
            bipGraphRS->createEdge(cur, (*i));
        
        // Connects current neighborhood with child internals
        if(childrenI.size() > 0)
            for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
                this->connectToChildInternal(cur, childrenL, (*i));
    }
}

void ERSProcesser::processRecursiveER(NetworkTreeNode *cur, unsigned short depth)
{
    ERGraph *bipGraphER = this->ERResult;
    list<NetworkTreeNode*> *children = cur->getChildren();

    // Goes deeper in the tree first
    for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
    {
        if(!(*i)->isLeaf())
            this->processRecursiveER((*i), depth + 1);
    }
    
    // Case where current node is an hedera (i.e., it has multiple labels)
    if(cur->isHedera())
    {
        list<InetAddress> *labels = cur->getLabels();
        list<Router*> *routers = cur->getInferredRouters();
        
        // Lists child leaves
        list<SubnetSite*> childrenL;
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            if((*i)->isLeaf())
                childrenL.push_back((*i)->getAssociatedSubnet());
        }
        
        // Process the hedera, label by label
        for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
        {
            InetAddress ingressInterface = (*i);

            bool isMissingInterface = false;
            if(ingressInterface == InetAddress(0))
            {
                /* 
                 * !!! Unique case (fixed on January 31, 2017) !!!
                 *
                 * To take account of routers which connect to subnets which the last route hop is 
                 * a missing interface (see also processRecursiveRS()).
                 */
                 
                isMissingInterface = true;
            }
            
            Router *ingressRouter = NULL;
            if(!isMissingInterface)
            {
                ingressRouter = cur->getRouterHaving(ingressInterface);
                
                // The ingress router should exist by construction. If not, we skip to next label.
                if(ingressRouter == NULL)
                    continue;
            }
            
            // Lists children subnets which last label in the route is ingressInterface
            list<SubnetSite*> curChildrenL;
            for(list<SubnetSite*>::iterator j = childrenL.begin(); j != childrenL.end(); ++j)
            {
                if((*j)->hasRouteLabel(ingressInterface, depth))
                {
                    curChildrenL.push_back((*j));
                    childrenL.erase(j--);
                }
            }
            
            // Gets the routers which give access to each listed subnet
            list<Router*> routersToConnect;
            if(!isMissingInterface)
                routersToConnect.push_back(ingressRouter);
            if(curChildrenL.size() > 0 && routers->size() > 1)
            {
                for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                {
                    Router *curRouter = (*j);
                
                    if(!isMissingInterface && curRouter == ingressRouter)
                        continue;
                    
                    // Router must give access to at least one listed subnet to continue
                    bool toConnect = false;
                    for(list<SubnetSite*>::iterator k = curChildrenL.begin(); k != curChildrenL.end(); ++k)
                    {
                        if(this->areRelated(curRouter, (*k)))
                        {
                            toConnect = true;
                            curChildrenL.erase(k--);
                        }
                    }
                    
                    if(toConnect)
                        routersToConnect.push_back(curRouter);
                }
            }
            
            /*
             * Connects the routers together. Three cases:
             * 1) (ingress router exists) There is at least one router to connect and there 
             *    remains at least one subnet for which no router was found. In that case, we put 
             *    the imaginary component into the linkage process, so there will be at least two 
             *    routers to connect in such a case.
             * 2) (ingress router exists) We found a router for each listed subnet and have more 
             *    than one listed routers. In that case, we call the same method but we do not 
             *    take account of the imaginary component as it is unnecessary for this particular 
             *    case.
             * 3) (current interface is a missing one) There is at least one router to connect. In 
             *    that case, we put the imaginary component into the linkage process.
             */
            
            if(!isMissingInterface)
            {
                if(routersToConnect.size() >= 1 && curChildrenL.size() > 0)
                    bipGraphER->connectRouters(routersToConnect, cur);
                else if(routersToConnect.size() > 1)
                    bipGraphER->connectRouters(routersToConnect, NULL);
            }
            else if(routersToConnect.size() >= 1)
                bipGraphER->connectRouters(routersToConnect, cur);
        }
    }
    // Any other case: connects all routers related to this neighborhood with a single switch
    else
    {
        bipGraphER->linkAllRouters(cur);
    }
}

bool ERSProcesser::areRelated(Router *r, SubnetSite *ss)
{
    list<RouterInterface*> *interfaces = r->getInterfacesList();
    for(list<RouterInterface*>::iterator i = interfaces->begin(); i != interfaces->end(); ++i)
    {
        if(ss->contains((*i)->ip))
            return true;
    }
    return false;
}

void ERSProcesser::connectToRelatedSubnets(Router *r, list<SubnetSite*> *subnetList)
{
    RSGraph *bipGraphRS = this->RSResult;
    for(list<SubnetSite*>::iterator i = subnetList->begin(); i != subnetList->end(); ++i)
    {
        SubnetSite *ss = (*i);
        if(this->areRelated(r, ss))
        {
            bipGraphRS->createEdge(r, ss); // Also designed to prevent duplicates
            subnetList->erase(i--);
        }
    }
}

void ERSProcesser::connectRemainingSubnets(NetworkTreeNode *n, list<SubnetSite*> *subnetList)
{
    RSGraph *bipGraphRS = this->RSResult;
    for(list<SubnetSite*>::iterator i = subnetList->begin(); i != subnetList->end(); ++i)
    {
        bipGraphRS->createEdge(n, (*i));
        subnetList->erase(i--);
    }
}

list<InetAddress> ERSProcesser::filterLabels(NetworkTreeNode *child)
{
    list<InetAddress> *labels = child->getLabels();
    list<InetAddress> result;
    
    if(labels->size() == 1)
    {
        result.push_back(labels->front());
    }
    else
    {
        list<InetAddress> copyLabels((*labels));
        for(list<InetAddress>::iterator i = copyLabels.begin(); i != copyLabels.end(); ++i)
        {
            InetAddress curLabel = (*i);
            Router *ingressRouterChild = child->getRouterHaving(curLabel);
            
            if(ingressRouterChild != NULL)
            {
                bool skip = true; // To skip first iteration
                for(list<InetAddress>::iterator j = i; j != copyLabels.end(); ++j)
                {
                    if(skip)
                    {
                        skip = false;
                        continue;
                    }
                
                    if(ingressRouterChild->hasInterface((*j)))
                        copyLabels.erase(j--);
                }
            }
            
            result.push_back(curLabel);
        }
    }
    
    return result;
}

bool ERSProcesser::connectToCloseSubnet(list<SubnetSite*> closeSubnets, 
                                        NetworkTreeNode *child, 
                                        InetAddress labelChild, 
                                        Router *routerChild)
{
    RSGraph *bipGraphRS = this->RSResult;
    for(list<SubnetSite*>::iterator i = closeSubnets.begin(); i != closeSubnets.end(); ++i)
    {
        SubnetSite *ss = (*i);
        if(ss->contains(labelChild))
        {
            if(routerChild != NULL)
                bipGraphRS->createEdge(routerChild, ss); // Connects to inferred router
            else
                bipGraphRS->createEdge(child, ss); // Connects to imaginary router
            
            return true;
        }
    }
    return false;
}

SubnetSite* ERSProcesser::getValidConnectingSubnet(NetworkTreeNode *child, InetAddress labelChild)
{
    SubnetMapEntry *sme = this->soilRef->getSubnetContaining(labelChild);
    
    if(sme != NULL)
    {
        list<NetworkTreeNode*> *children = child->getChildren();
        for(list<NetworkTreeNode*>::iterator i = children->begin(); i != children->end(); ++i)
        {
            if((*i)->getAssociatedSubnet() == sme->subnet)
            {
                return NULL;
            }
        }
    }
    else
    {
        return NULL;
    }
    
    return sme->subnet;
}

void ERSProcesser::connectToChildInternal(Router *r, 
                                          list<SubnetSite*> closeSubnets, 
                                          NetworkTreeNode *child)
{
    RSGraph *bipGraphRS = this->RSResult;
    
    list<InetAddress> filteredLabels = this->filterLabels(child);
    for(list<InetAddress>::iterator it = filteredLabels.begin(); it != filteredLabels.end(); ++it)
    {
        InetAddress labelChild = (*it);
        
        if(labelChild == InetAddress(0))
        {
            bipGraphRS->createArtificialPath(r, child); // No other choice
            continue;
        }
        
        Router *ingressRouterChild = child->getRouterHaving(labelChild);
        
        bool successfullyConnected = this->connectToCloseSubnet(closeSubnets, 
                                                                child, 
                                                                labelChild, 
                                                                ingressRouterChild);
        
        if(!successfullyConnected)
        {
            SubnetSite *connectingSubnet = this->getValidConnectingSubnet(child, labelChild);
            
            if(connectingSubnet != NULL)
            {
                bipGraphRS->createEdge(r, connectingSubnet);
            
                if(ingressRouterChild != NULL)
                    bipGraphRS->createEdge(ingressRouterChild, connectingSubnet);
                else
                    bipGraphRS->createEdge(child, connectingSubnet);
            }
            else
            {
                if(ingressRouterChild != NULL)
                    bipGraphRS->createArtificialPath(r, ingressRouterChild);
                else
                    bipGraphRS->createArtificialPath(r, child);
            }
        }
    }
}

void ERSProcesser::connectToChildInternal(NetworkTreeNode *p, 
                                          list<SubnetSite*> closeSubnets, 
                                          NetworkTreeNode *child)
{
    RSGraph *bipGraphRS = this->RSResult;
    
    list<InetAddress> filteredLabels = this->filterLabels(child);
    for(list<InetAddress>::iterator it = filteredLabels.begin(); it != filteredLabels.end(); ++it)
    {
        InetAddress labelChild = (*it);
        
        if(labelChild == InetAddress(0))
        {
            bipGraphRS->createArtificialPath(p, child); // No other choice
            continue;
        }
        
        Router *ingressRouterChild = child->getRouterHaving(labelChild);
            
        bool successfullyConnected = this->connectToCloseSubnet(closeSubnets, 
                                                                child, 
                                                                labelChild, 
                                                                ingressRouterChild);
        
        if(!successfullyConnected)
        {
            SubnetSite *connectingSubnet = this->getValidConnectingSubnet(child, labelChild);
            
            if(connectingSubnet != NULL)
            {
                bipGraphRS->createEdge(p, connectingSubnet);
            
                if(ingressRouterChild != NULL)
                    bipGraphRS->createEdge(ingressRouterChild, connectingSubnet);
                else
                    bipGraphRS->createEdge(child, connectingSubnet);
            }
            else
            {
                if(ingressRouterChild != NULL)
                    bipGraphRS->createArtificialPath(ingressRouterChild, p);
                else
                    bipGraphRS->createArtificialPath(p, child);
            }
        }
    }
}

void ERSProcesser::visit(Vertice *node)
{
    if(L2Device *l2 = dynamic_cast<L2Device*>(node))
    {
        unsigned int ID = l2->getID();
        if(!checkArrE[ID - 1])
        {
            checkArrE[ID - 1] = true;
            list<Edge*> *edges = l2->getIncidentEdges();
            for(list<Edge*>::iterator it = edges->begin(); it != edges->end(); ++it)
            {
                L3Device *l3 = (L3Device*) (*it)->getVerticeTwo(); // The router is always second
                L3Device *eq = routersRS[l3->getID() - 1];
                visit(eq);
            }
        }
    }
    else if(L3Device *l3 = dynamic_cast<L3Device*>(node))
    {
        unsigned int ID = l3->getID();
        if(!checkArrR[ID - 1])
        {
            checkArrR[ID - 1] = true;
            list<Edge*> *edges = l3->getIncidentEdges();
            for(list<Edge*>::iterator it = edges->begin(); it != edges->end(); ++it)
            {
                visit((*it)->getVerticeTwo()); // The subnet is always the second vertice here
            }
            
            L3Device *eq = routersER[ID - 1];
            list<Edge*> *edgesBis = eq->getIncidentEdges();
            for(list<Edge*>::iterator it = edgesBis->begin(); it != edgesBis->end(); ++it)
            {
                visit((*it)->getVerticeOne()); // The switch is always first here
            }
        }
    }
    else if(Subnet *s = dynamic_cast<Subnet*>(node))
    {
        unsigned int ID = s->getID();
        if(!checkArrS[ID - 1])
        {
            checkArrS[ID - 1] = true;
            list<Edge*> *edges = s->getIncidentEdges();
            for(list<Edge*>::iterator it = edges->begin(); it != edges->end(); ++it)
            {
                visit((*it)->getVerticeOne()); // The router is always the first vertice here
            }
        }
    }
}
