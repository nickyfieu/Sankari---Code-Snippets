#include "AIPathNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "../Helpers.h"

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
	for (int32 i = 0; i < amountOfNodes; i++)
	{
		const FAIPathNode& currentNode = m_NodeContainer[i];
		for (int32 nodeIndex : currentNode.m_ConnectedNodeIndexes)
		{
			const FAIPathNode& otherNode = m_NodeContainer[nodeIndex];
			DrawDebugLines(actorTransform, currentNode, otherNode);
			DrawDebugArrow(actorTransform, currentNode, otherNode);
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
	m_AmountOfNodes = m_NodeContainer.Num(); // do not move this line below InitializeStoredPathData or there will be some issues
	InitializeNodes();
	InitializeStoredPathData();
}



/// <summary>
/// Initializes all data that the FAIPathNode objects will need later on
/// </summary>
void AAIPathNetwork::InitializeNodes()
{
	for (int32 i = 0; i < m_AmountOfNodes; i++)
	{
		m_NodeContainer[i].SetNetworkReference(this);
		m_NodeContainer[i].Initialize();
	}
}



/// <summary>
/// Makes sure m_storedPathData has the correct size and has no data inside of it yet
/// </summary>
void AAIPathNetwork::InitializeStoredPathData()
{
	TArray<FAIPathData> emptyArr{};
	m_StoredPathData.Empty(); // making sure the stored path data is empty
	m_StoredPathData.Reserve(m_AmountOfNodes); // pre alocates enough memory for the array
	for (int32 i = 0; i < m_AmountOfNodes; i++)
	{
		m_StoredPathData.Add(TPair<int32, TArray<FAIPathData>>(i, emptyArr));
	}
}



/// <summary>
/// This function calculates all shortest paths from beginNode till any node that it can possibly
/// reach in the node network. It uses dijkstra to achieve this, after it has calculated the shortest path
/// for node at index "beginNode" then it stores it at m_StoredPathData["beginNode"].
/// </summary>
/// <param name="beginNode">The node index from wich all paths will be calculated from</param>
void AAIPathNetwork::CalculatePathData(int32 beginNode)
{
	// calculating path data using dijkstra

	// distance, nodeIndex
	TSet<TPair<float, int32>> toCheck{};

	TPair<float, int32> intializer = TPair<float, int32>(FLT_MAX, -1);
	TArray<TPair<float, int32>> distances{};
	
	// initializing the distances array
	distances.Reserve(m_AmountOfNodes);
	for (int32 i = 0; i < m_AmountOfNodes; i++)
	{
		distances.Add(intializer);
	}
	
	// setting up start of algorithm
	distances[beginNode] = TPair<float, int32>(0.0f, beginNode);
	toCheck.Add(distances[beginNode]);

	while (toCheck.Num() != 0) // this means we still have paths to check
	{
		TPair<float, int32> currentCheck = *toCheck.begin();
		toCheck.Remove(currentCheck);
		toCheck.Shrink();

		int32 currentIndex = currentCheck.Value;
		const FAIPathNode& currentNode = m_NodeContainer[currentIndex];
		int32 amountOfConnectedNodes = currentNode.m_ConnectedNodeIndexes.Num();

		for (int32 i = 0; i < amountOfConnectedNodes; i++)
		{
			int otherIndex = currentNode.m_ConnectedNodeIndexes[i];
			float otherWeight = currentNode.GetConnectedNodeWeight(i);

			if (!(distances[otherIndex].Key > (distances[currentIndex].Key + otherWeight)))
			{
				continue;
			}
			// if shorter path update it
			
			// if not max its in our toCheck already
			// so reinsert it with the new smaller weight
			if (distances[otherIndex].Key != FLT_MAX)
			{
				toCheck.Remove(TPair<float, int32>(distances[otherIndex].Key, otherIndex));
				toCheck.Shrink();
			}

				distances[otherIndex] = TPair<float, int32>(distances[currentIndex].Key + otherWeight, currentIndex);
				toCheck.Add(TPair<float, int32>(distances[otherIndex].Key, otherIndex));
		}
	}

	TArray<FAIPathData>& storedPathDataRef = m_StoredPathData[beginNode];
	storedPathDataRef.Empty(); // makes sure TArray is empty 
	for (int32 i = 0; i < m_AmountOfNodes; i++)
	{
		storedPathDataRef.Add(FAIPathData(distances[i].Key, distances[i].Value));
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
TArray<FAIPathData>& AAIPathNetwork::GetPathData(int32 beginNode)
{
	if (m_StoredPathData[beginNode].Num() == m_AmountOfNodes) // means it already was calculated and stored
	{
		return m_StoredPathData[beginNode];
	}

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
	{
		UKismetSystemLibrary::FlushPersistentDebugLines(pWorld);
	}
}



/// <summary>
/// This function gets called when bleuprint editor viewport gets closed.
/// This fixes the kismet debug lines + arrows disapearing after closing the bleuprint editor viewport.
/// </summary>
/// <param name="pWorld">Unused parameter</param>
/// <param name="b1">Unused parameter</param>
/// <param name="b2">Unused parameter</param>
void AAIPathNetwork::HandleCleanup(UWorld* pWorld, bool b1, bool b2)
{
	DebugDraw();
}



/// <summary>
/// Does some preChecks to see if a path is possible if so it will return a valid path 
/// using the given data in order from closest node to the end node.
/// </summary>
/// <param name="pathData">The pathdata gotten from GetPathData(index) using the given index</param>
/// <param name="toNode">Node index of the node you want to move towards</param>
/// <returns>Returns the path to traverse to get to the given toNode index</returns>
FAIPath AAIPathNetwork::GetPathFromTo(const TArray<FAIPathData>& pathData, int32 toNode) const
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
		LogText(ELogVerbosity::Warning, "AAIPathNetwork::GetPathFromTo cannot reach targetNode [ " + FString::FromInt(toNode) + " ]");
		return path;
	}

	// when all prechecks are done we know we have a valid path for sure
	path.m_bIsValid = true;
	int32 currentToNode = toNode;

	// adding all the nodes to traverse to an array
	while (currentToNode != pathData[currentToNode].m_PreviousNodeIndex)
	{
		path.m_Path.Add(currentToNode);
		currentToNode = pathData[currentToNode].m_PreviousNodeIndex;
	}
	path.m_Path.Add(currentToNode); // adding the final node ( first node )

	Algo::Reverse(path.m_Path); // reversing the path so we start with the begin node
	return path;
}



/// <summary>
/// Calculates the closest node in this node network from the given vector "Location".
/// </summary>
/// <param name="location">Worldposition of an object</param>
/// <returns>The node index of the closest node</returns>
int32 AAIPathNetwork::LocationToNodeIndex(const FVector& location) const
{
	int32 nodeIndex = -1;
	float distanceSquared = FLT_MAX;
	FVector ownLocation = this->GetActorLocation();

	for (int32 i = 0; i < m_AmountOfNodes; i++)
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
/// This function calculates all the squared distances towards all connected nodes of this node.
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
		if (m_ConnectedNodeIndexes[i] >= nrOfNodes)
		{
			check(false);
			LogText(ELogVerbosity::Error, "FAIPathNode::CalculateSquareDistances invalid connected node index[ " + FString::FromInt(m_ConnectedNodeIndexes[i]) + " ] setting it to 0");
			m_ConnectedNodeIndexes[i] = 0;
		}
		else
		{
			FAIPathNode& other = m_pNetworkReference->m_NodeContainer[m_ConnectedNodeIndexes[i]];
			m_ConnectedSquaredDistances.Add(FVector::DistSquared(this->m_Location, other.m_Location));
		}
	}
}



// setters & getters

/// <summary>
/// Sets the network reference to be used in calculating the squared distances.
/// </summary>
/// <param name="pNetworkRef">Reference of the owning AIPathNetwork object</param>
void FAIPathNode::SetNetworkReference(AAIPathNetwork* pNetworkRef)
{
	if (pNetworkRef != nullptr)
	{
		m_pNetworkReference = pNetworkRef;
		return;
	}
	// the given pointer shouldn't be a nullptr
	check(false);
	return LogText(ELogVerbosity::Warning, "FAIPathNode::SetNetworkReference the given network reference was nullptr!");
}



/// <summary>
/// Returns the calculated squared distances using the given node index.
/// Returns float max when invalid node.
/// </summary>
/// <param name="nodeIndex">Other node index to get distance to from</param>
/// <returns>Squared distance towards nodeIndex from self</returns>
float FAIPathNode::GetConnectedNodeWeight(int32 nodeIndex) const
{
	return IsValidIndex(nodeIndex, m_ConnectedSquaredDistances) ? m_ConnectedSquaredDistances[nodeIndex] : FLT_MAX;
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

FAIPathData::FAIPathData(float dist, int32 prevNode)
	: m_SquaredDistance{ dist }
	, m_PreviousNodeIndex{ prevNode }
{
}
