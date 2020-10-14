#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIPathNetwork.generated.h"

USTRUCT(BlueprintType)
struct FAIPathData
{
	GENERATED_BODY()

	FAIPathData(float dist = 0.0f, int32 prevNode = -1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AIPathNetwork", Meta = (DisplayName = "squared Distance"))
		float m_SquaredDistance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AIPathNetwork", Meta = (DisplayName = "previous Node Index"))
		int32 m_PreviousNodeIndex;
};

USTRUCT(BlueprintType)
struct FAIPathNode
{
	GENERATED_BODY()

	FAIPathNode();

	// MakeEditWidget allows for transforming the location of the node in the scene
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AIPathNetwork", Meta = (DisplayName = "NodeLocation", MakeEditWidget))
		FVector m_Location;

	// this contains all indexes of all connected nodes ( meaning you can move from this node to the connected node )
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AIPathNetwork", Meta = (DisplayName = "ConnectedNodeIndexes"))
		TArray<int32> m_ConnectedNodeIndexes;

	void Initialize();

	void SetNetworkReference(class AAIPathNetwork* pNetworkRef);

	float GetConnectedNodeWeight(int32 nodeIndex) const;

private:
	// used to get m_NodeContainer;
	class AAIPathNetwork* m_pNetworkReference = nullptr;

	// !!!index 0 of this array is same as 0 of m_ConnectedNodeIndexes!!!
	// this is the squaredDistance from self(x) to an other node in AAIPathNetwork::m_NodeContainer[m_ConnectedNodeIndexes[x]]
	TArray<float> m_ConnectedSquaredDistances;

	void CalculateSquareDistances();
};

USTRUCT(BlueprintType)
struct FAIPath
{
	GENERATED_BODY()

	FAIPath();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AIPathNetwork", Meta = (DisplayName = "valid path"))
		bool m_bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AIPathNetwork", Meta = (DisplayName = "path"))
		TArray<int32> m_Path;
};

UCLASS()
class SANKARI_API AAIPathNetwork : public AActor
{
	GENERATED_BODY()
	
public:	
	AAIPathNetwork();

	//contains all path nodes and can only be edited in the scene
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AIPathNetwork", Meta = (DisplayName = "Nodes"))
		TArray<FAIPathNode> m_NodeContainer;

#pragma region DebugVariables

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Line Width"))
		float m_DebugLineWidth = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Line Color"))
		FLinearColor m_DebugLineColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Arrow Length Percentage"))
		float m_DebugArrowLength = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Arrow Width"))
		float m_DebugArrowWidth = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Arrow Size"))
		float m_DebugArrowSize = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Arrow Offset"))
		FVector m_DebugArrowOffset = FVector(0.0f,0.0f,10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug_AIPathNetwork", Meta = (DisplayName = "Arrow Color"))
		FLinearColor m_DebugArrowColor = FLinearColor::Black;

#pragma endregion

	UFUNCTION(BlueprintCallable, Category = "AIPathNetwork")
		int32 LocationToNodeIndex(const FVector& location) const;

	// not const due to if not cached it will have to calculate the path and store it
	UFUNCTION(BlueprintCallable, Category = "AIPathNetwork")
		TArray<FAIPathData>& GetPathData(int32 beginNode);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AIPathNetwork")
		FAIPath GetPathFromTo(const TArray<FAIPathData>& pathData, int32 toNode) const;

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
		void HandleDelete(AActor* toDelete);

	UFUNCTION()
		void HandleCleanup(UWorld* pWorld, bool b1, bool b2);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR


private:
	// debug - EDITOR only
	void DebugDraw();
	void DrawDebugLines(const FTransform& actorTransform, const FAIPathNode& nodeA, const FAIPathNode& nodeB);
	void DrawDebugArrow(const FTransform& actorTransform, const FAIPathNode& nodeA, const FAIPathNode& nodeB);

	// initiallization
	void Initialize();
	void InitializeNodes();
	void InitializeStoredPathData();

	// helper functions
	void CalculatePathData(int32 beginNode);

	// storing the distance and the previous node towards current node
	// <current node, <distanceSquared, previous node towards current node>>
	// if "previous node towards current node" = -1 means its an imposible path!
	TMap<int32, TArray<FAIPathData>> m_StoredPathData; // TMAP is unreals version of std::unordered_map

	int32 m_AmountOfNodes = 0;
};
