#include "AIPathNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Helpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include <set>

//
// AIPathNetwork
//

AAIPathNetwork::AAIPathNetwork()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;


	// Warning : do not remove #if WITH_EDITOR without removing evrything inside of it or package errors will occur!
#if WITH_EDITOR
	if (GEngine)
	{
		// when deleting object from world debug visualizations would stay in the world
		// this fixes that issue
		GEngine->OnLevelActorDeleted().AddUObject(this, &AAIPathNetwork::HandleDelete);
	}
	
	// when closing blueprint editor ( would clear debug visualization )
	// this fixes that issue
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &AAIPathNetwork::HandleCleanup);
#endif // WITH_EDITOR

}

void AAIPathNetwork::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Initialize();
	DebugDraw();
}

void AAIPathNetwork::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
}

// DEBUG - EDITOR only

#if WITH_EDITOR
/// <summary>
/// when something specific has to happen when a property changes (for debug purposes only)
/// </summary>
/// <param name="propertyChangedEvent">reference to what property has changed</param>
void AAIPathNetwork::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	Super::PostEditChangeProperty(propertyChangedEvent);

	if (propertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AAIPathNetwork, m_NodeContainer))
	{
		LogText(ELogVerbosity::Log, L"AAIPathNetwork::PostEditChangeProperty m_NodeContainer size was changed!");
		InitializeStoredPathData();
		m_AmountOfNodes = m_NodeContainer.Num();
		DebugDraw();
	}

	if (propertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FAIPathNode, m_Location))
	{
		InitializeStoredPathData();
	}
}
#endif // WITH_EDITOR



/// <summary>
/// Function called to draw all debug information of the AIPathNetwork.
/// </summary>
void AAIPathNetwork::DebugDraw()
{
	if (GetWorld() == nullptr) return;

	UKismetSystemLibrary::FlushPersistentDebugLines(this->GetWorld());

	int32 amountOfNodes = m_NodeContainer.Num();
	FTransform actorTransform = this->GetTransform();
	for (FAIPathNode& node : m_NodeContainer)
	{
		for (int nodeIndex : node.m_ConnectedNodeIndexes)
		{
			const FAIPathNode& otherNode = m_NodeContainer[nodeIndex];
			DrawDebugLines(actorTransform, node, otherNode);
			DrawDebugArrow(actorTransform, node, otherNode);
		}
	}
}



/// <summary>
/// Draws a debug line from (Kistmet library) from nodeA to nodeB
/// </summary>
/// <param name="actorTransform">world location of own AIPathNetwork object</param>
/// <param name="nodeA">begin node to draw from</param>
/// <param name="nodeB">end node to draw from</param>
void AAIPathNetwork::DrawDebugLines(const FTransform& actorTransform, const FAIPathNode& nodeA, const FAIPathNode& nodeB)
{
	FVector transformedPosNodeA = actorTransform.TransformPosition(nodeA.m_Location);
	FVector transformedPosNodeB = actorTransform.TransformPosition(nodeB.m_Location);

	UKismetSystemLibrary::DrawDebugLine(this->GetWorld(), transformedPosNodeA, transformedPosNodeB, m_DebugLineColor, FLT_MAX, m_DebugLineWidth);
}



/// <summary>
/// Draws an debug arrow from (Kistmet library) from nodeA to nodeB
/// </summary>
/// <param name="actorTransform">world location of own AIPathNetwork object</param>
/// <param name="nodeA">begin node to draw from</param>
/// <param name="nodeB">end node to draw from</param>
void AAIPathNetwork::DrawDebugArrow(const FTransform& actorTransform, const FAIPathNode& nodeA, const FAIPathNode& nodeB)
{
	FVector transformedPosNodeA = actorTransform.TransformPosition(nodeA.m_Location);
	FVector transformedPosNodeB = actorTransform.TransformPosition(nodeB.m_Location);

	FVector dirAToB = {};
	float lengthAToB = 0.0f;
	(transformedPosNodeB - transformedPosNodeA).ToDirectionAndLength(dirAToB, lengthAToB);

	FVector posBegin = transformedPosNodeA + m_DebugArrowOffset;
	FVector posEnd = posBegin + dirAToB * (lengthAToB * m_DebugArrowLength);

	UKismetSystemLibrary::DrawDebugArrow(this->GetWorld(), posBegin, posEnd, m_DebugArrowSize, m_DebugArrowColor, FLT_MAX, m_DebugArrowWidth);
}



// helper functions

/// <summary>
/// Initializes all data that the AIPathNetwork objects will need later on
/// </summary>
void AAIPathNetwork::Initialize()
{
	InitializeNodes();
	InitializeStoredPathData();
	m_AmountOfNodes = m_NodeContainer.Num();
}



/// <summary>
/// Initializes all data that the FAIPathNode objects will need later on
/// </summary>
void AAIPathNetwork::InitializeNodes()
{
	for (FAIPathNode& node : m_NodeContainer)
	{
		node.SetNetworkReference(this);
		node.Initialize();
	}
}



/// <summary>
/// Makes sure m_storedPathData has the correct size and has no data inside of it yet
/// </summary>
void AAIPathNetwork::InitializeStoredPathData()
{
	TArray<FAIPathData> emptyArr{};
	m_StoredPathData.clear();
	for (int i = 0; i < m_AmountOfNodes; i++)
	{
		m_StoredPathData.emplace(std::make_pair(i, emptyArr));
	}
}



/// <summary>
/// This function calculates all shortest paths from beginNode till any node that it can possibly
/// reach in the node network. It uses dijkstra to achieve this, after it has calculated the shortest path
/// for node at index "beginNode" then it stores it at m_StoredPathData["beginNode"].
/// </summary>
/// <param name="beginNode">The node index from wich all paths will be calculated from</param>
void AAIPathNetwork::CalculatePathData(int beginNode)
{
	// calculating path data using dijkstra

	// distance, nodeIndex
	std::set<std::pair<float, int>> toCheck;
	std::vector<std::pair<float, int>> distances(m_AmountOfNodes, std::make_pair(FLT_MAX, -1));
	distances[beginNode] = std::make_pair(0.0f, beginNode);
	toCheck.insert(distances[beginNode]);

	while (!toCheck.empty())
	{
		std::pair<float, int> currentCheck = *(toCheck.begin());
		toCheck.erase(toCheck.begin());

		int currentIndex = currentCheck.second;
		FAIPathNode currentNode = m_NodeContainer[currentIndex];
		int32 amountOfConnectedNodes = currentNode.m_ConnectedNodeIndexes.Num();

		for (int32 i = 0; i < amountOfConnectedNodes; i++)
		{
			int otherIndex = currentNode.m_ConnectedNodeIndexes[i];
			float otherWeight = currentNode.GetConnectedNodeWeight(i);

			// if shorter path update it
			if (distances[otherIndex].first > (distances[currentIndex].first + otherWeight))
			{
				// if not max its in our toCheck already
				// so reinsert it with the new smaller weight
				if (distances[otherIndex].first != FLT_MAX)
				{
					toCheck.erase(toCheck.find(std::make_pair(distances[otherIndex].first, otherIndex)));
				}

				distances[otherIndex] = std::make_pair(distances[currentIndex].first + otherWeight, currentIndex);
				toCheck.insert(std::make_pair(distances[otherIndex].first, otherIndex));
			}
		}
	}

	TArray<FAIPathData>& storedPathDataRef = m_StoredPathData[beginNode];
	storedPathDataRef.Empty();
	for (int i = 0; i < m_AmountOfNodes; i++)
	{
		storedPathDataRef.Add(FAIPathData(distances[i].first, distances[i].second));
	}
}

// callable functions

/// <summary>
/// This function returns the stored path data if the begin node was already asked for before.
/// If not asked for before it will calculate all the posible shortest paths from the begin node to each node in the network
/// and store this in m_StoredPathData.
/// </summary>
/// <param name="beginNode">the node where the path data begins from</param>
/// <returns>stored path data from the beginNode</returns>
TArray<FAIPathData> AAIPathNetwork::GetPathData(int beginNode)
{
	if (m_StoredPathData[beginNode].Num() == m_AmountOfNodes) // means it already was calculated and stored
		return m_StoredPathData[beginNode];

	CalculatePathData(beginNode);

	return m_StoredPathData[beginNode];
}



/// <summary>
/// This function gets called when deleting this AIPathNetwork object.
/// Thix fixes kismet debug lines + arrows staying after deleting the object.
/// </summary>
/// <param name="toDelete">unused parameter</param>
void AAIPathNetwork::HandleDelete(AActor* toDelete)
{
	UWorld* pWorld = GetWorld();
	if (pWorld != nullptr)
		UKismetSystemLibrary::FlushPersistentDebugLines(pWorld);
}



/// <summary>
/// This function gets called when bleuprint editor viewport gets closed.
/// This fixes the kismet debug lines + arrows disapearing after closing the bleuprint editor viewport.
/// </summary>
/// <param name="pWorld">unused parameter</param>
/// <param name="b1">unused parameter</param>
/// <param name="b2">unused parameter</param>
void AAIPathNetwork::HandleCleanup(UWorld* pWorld, bool b1, bool b2)
{
	DebugDraw();
}



/// <summary>
/// Does some preChecks to see if a path is possible if so it will return a valid path 
/// using the given data in order from closest node to the end node.
/// </summary>
/// <param name="pathData">the pathdata gotten from GetPathData(index) using the given index</param>
/// <param name="toNode">node index of the node you want to move towards</param>
/// <returns>Returns the path to traverse to get to the given toNode index</returns>
FAIPath AAIPathNetwork::GetPathFromTo(const TArray<FAIPathData>& pathData, int toNode) const
{
	FAIPath path{};
	int32 pathDataSize = pathData.Num();

	// pre checks
	if (pathDataSize == 0)
	{
		LogText(ELogVerbosity::Warning, "AAIPathNetwork::GetPathFromTo Can't get path on a network of size 0");
		return path;
	}

	if (pathDataSize != m_AmountOfNodes)
	{
		LogText(ELogVerbosity::Error, "AAIPathNetwork::GetPathFromTo pathData incorrect size! This shouldnt happen!");
		return path;
	}

	if (pathData[toNode].m_PreviousNodeIndex == -1)
	{
		// non existent path
		return path;
	}

	// when all prechecks are done we know we have a valid path for sure
	path.m_bIsValid = true;
	int currentToNode = toNode;
	while (currentToNode != pathData[currentToNode].m_PreviousNodeIndex)
	{
		path.m_Path.Add(currentToNode);
		currentToNode = pathData[currentToNode].m_PreviousNodeIndex;
	}
	path.m_Path.Add(currentToNode); // adding the final node ( first node )
	std::reverse(std::begin(path.m_Path), std::end(path.m_Path));

	return path;
}



/// <summary>
/// Calculates the closest node in this node network from the given vector "Location".
/// </summary>
/// <param name="location">worldposition of an object</param>
/// <returns>the node index of the closest node</returns>
int AAIPathNetwork::LocationToNodeIndex(const FVector& location) const
{
	int nodeIndex = -1;
	float distanceSquared = FLT_MAX;
	FVector ownLocation = this->GetActorLocation();

	for (int i = 0; i < m_AmountOfNodes; i++)
	{
		float sqDistCalc = FVector::DistSquared(ownLocation + m_NodeContainer[i].m_Location, location);
		if (sqDistCalc < distanceSquared)
		{
			nodeIndex = i;
			distanceSquared = sqDistCalc;
		}
	}

	// if this triggers this means you have a AIPathNetwork with 0 nodes and are calling this function!
	ensure(nodeIndex != -1);
	return nodeIndex;
}


//
// AIPathNode
//

FAIPathNode::FAIPathNode()
{
}



// helper functions

void FAIPathNode::Initialize()
{
	check(m_pNetworkReference != nullptr);
	CalculateSquareDistances();
}

/// <summary>
/// This function calculates all the squared distances towards all connected nodes of this node
/// </summary>
void FAIPathNode::CalculateSquareDistances()
{
	if (m_pNetworkReference == nullptr)
		return LogText(ELogVerbosity::Warning, L"FAIPathNode::CalculateSquareDistances Network reference was nullptr!");

	// makes sure the vector is empty to then fill it with the squared distances to each connected node
	m_ConnectedSquaredDistances.Empty();
	int32 nrOfConnectedNodes = m_ConnectedNodeIndexes.Num();
	int32 nrOfNodes = m_pNetworkReference->m_NodeContainer.Num();

	m_ConnectedSquaredDistances.Reserve(nrOfConnectedNodes);
	for (int32 i = 0; i < nrOfConnectedNodes; i++)
	{
		check(m_ConnectedNodeIndexes[i] < nrOfNodes);
		FAIPathNode& other = m_pNetworkReference->m_NodeContainer[m_ConnectedNodeIndexes[i]];
		m_ConnectedSquaredDistances.Add(FVector::DistSquared(this->m_Location, other.m_Location));
	}
}



// setters & getters

/// <summary>
/// Sets the network reference to be used in calculating the squared distances
/// </summary>
/// <param name="pNetworkRef">reference of the owning AIPathNetwork object</param>
void FAIPathNode::SetNetworkReference(AAIPathNetwork* pNetworkRef)
{
	// the given pointer shouldn't be a nullptr
	ensure(pNetworkRef != nullptr);
	m_pNetworkReference = pNetworkRef;
}



/// <summary>
/// returns the calculated squared distances using the given node index
/// </summary>
/// <param name="nodeIndex">other node index to get distance to from</param>
/// <returns>squared distance towards nodeIndex from self</returns>
float FAIPathNode::GetConnectedNodeWeight(int nodeIndex) const
{
	check(nodeIndex < m_ConnectedSquaredDistances.Num());
	return m_ConnectedSquaredDistances[nodeIndex];
}

//
// AIPath
//

FAIPath::FAIPath()
{
}

//
// AIPathData
//

FAIPathData::FAIPathData(float dist, int prevNode)
	: m_SquaredDistance{ dist }
	, m_PreviousNodeIndex{ prevNode }
{
}
