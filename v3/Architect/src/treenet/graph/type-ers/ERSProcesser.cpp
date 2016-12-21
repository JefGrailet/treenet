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
    output << this->ERResult->edgesToString() << "\n";
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
            
            // Skips null label if it exists (will be replaced by an imaginary router)
            if(ingressInterface == InetAddress(0))
                continue;
            
            Router *ingressRouter = cur->getRouterHaving(ingressInterface);
            
            // The ingress router should exist by construction. If not, we skip to next label.
            if(ingressRouter == NULL)
                continue;
            
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
            list<SubnetSite*> backUpSubnets(curChildrenL);
            
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
            
            this->connectToRelatedSubnets(ingressRouter, &curChildrenL);
            
            /*
             * The code now looks for routers which give access to the remaining subnets. It will 
             * connect these subnets with the selected routers.
             */
            
            if(curChildrenL.size() > 0 && routers->size() > 1)
            {
                for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                {
                    Router *curRouter = (*j);
                
                    if(curRouter == ingressRouter)
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
            
            // Connects this router with its child internals
            if(curChildrenI.size() > 0)
            {
                for(list<NetworkTreeNode*>::iterator j = curChildrenI.begin(); j != curChildrenI.end(); ++j)
                {
                    this->connectToChildInternal(ingressRouter, backUpSubnets, (*j));
                }
            }
        }
        
        /*
         * Finally, connects the remaining subnets (which the last route step is probably 0.0.0.0) 
         * with an imaginary router.
         */
        
        if(childrenL.size() > 0)
        {
            // Creates edges with imaginary router
            for(list<SubnetSite*>::iterator i = childrenL.begin(); i != childrenL.end(); ++i)
                bipGraphRS->createEdge(cur, (*i));
            
            // Lists child internal nodes which the previous label(s) contain 0.0.0.0
            list<NetworkTreeNode*> lastChildrenI;
            for(list<NetworkTreeNode*>::iterator j = childrenI.begin(); j != childrenI.end(); ++j)
                if((*j)->hasPreviousLabel(InetAddress(0)))
                    lastChildrenI.push_back((*j));
            
            // Connects with the child internal nodes
            if(lastChildrenI.size() > 0)
                for(list<NetworkTreeNode*>::iterator i = lastChildrenI.begin(); i != lastChildrenI.end(); ++i)
                    this->connectToChildInternal(cur, childrenL, (*i));
        }
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
        if(routers->size() > 1)
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
            this->connectRemainingSubnets(cur, &childrenL);
        
        // Connects this neighborhood with the children that are internals
        if(childrenI.size() > 0)
        {
            for(list<NetworkTreeNode*>::iterator i = childrenI.begin(); i != childrenI.end(); ++i)
            {
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

            // Skips 0.0.0.0 label if it exists
            if(ingressInterface == InetAddress(0))
                continue;
            
            Router *ingressRouter = cur->getRouterHaving(ingressInterface);
            
            // The ingress router should exist by construction. If not, we skip to next label.
            if(ingressRouter == NULL)
                continue;
            
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
            
            list<Router*> routersToConnect;
            routersToConnect.push_back(ingressRouter);
            if(curChildrenL.size() > 0 && routers->size() > 1)
            {
                for(list<Router*>::iterator j = routers->begin(); j != routers->end(); ++j)
                {
                    Router *curRouter = (*j);
                
                    if(curRouter == ingressRouter)
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
            
            if(routersToConnect.size() > 1)
            {
                if(curChildrenL.size() > 0)
                    bipGraphER->connectRouters(routersToConnect, cur);
                else
                    bipGraphER->connectRouters(routersToConnect, NULL);
            }
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
