//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "NavLinkHostInterface.h"
#include "NavLinkCustomInterface.h"
#include "GameplayTags.h"
#include "UnitNavLinkComponent.generated.h"

/**
 * Nav link component for unit pathfinding.
 */
UCLASS(ClassGroup = (Navigation), Meta = (BlueprintSpawnableComponent), HideCategories = (Activation))
class ZOMBIES_API UUnitNavLinkComponent : public UPrimitiveComponent, public INavLinkHostInterface, public INavLinkCustomInterface
{
	GENERATED_BODY()

public:

	UUnitNavLinkComponent();

	//Specifies a generic tag describing how the unit should navigate this link.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Navigation)
	FGameplayTag LinkTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Navigation)
	FNavigationLink Link;

	// BEGIN INavLinkCustomInterface
	virtual void GetLinkData(FVector& Left, FVector& Right, ENavLinkDirection::Type& Direction) const override;
	virtual void GetSupportedAgents(FNavAgentSelector& OutSupportedAgents) const override;
	virtual TSubclassOf<UNavArea> GetLinkAreaClass() const override;
	virtual uint32 GetLinkId() const override;
	virtual void UpdateLinkId(uint32 NewUniqueId) override;
	virtual bool IsLinkPathfindingAllowed(const UObject* Querier) const override;
	virtual bool OnLinkMoveStarted(UObject* PathFollowingComponent, const FVector& GoalLocation) override;
	virtual void OnLinkMoveFinished(UObject* PathFollowingComponent) override;
	// END INavLinkCustomInterface

	virtual void GetNavigationData(FNavigationRelevantData& Data) const override;
	virtual bool IsNavigationRelevant() const override;

	virtual bool GetNavigationLinksClasses(TArray<TSubclassOf<class UNavLinkDefinition> >& OutClasses) const override { return false; }
	virtual bool GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const override;

	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }

	FNavigationLink GetNavigationLink() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual void PostEditImport() override;
#endif // WITH_EDITOR

	//virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = Test)
	FORCEINLINE FString GetLinkIdName() const { return FUniqueLinkId{ LinkId }.ToString(); }

protected:

	void InitializeLinksAreaClasses();




	/*
	Unreal has a design flaw in its nav link id system.
	It creates a simple monotonic global counter as a unique id to keep track of the mapping between link components and the nav mesh edge they represent.
	The counter is typed as uint32 in the interface but internally does narrowing conversions to int32 in many places, changing the actual value that should have been preserved.
	The counter is reset to zero both on app exit and when any level is beginning, which means pressing play in editor will break the required uniqueness property.
	This means that when components in the editor world need a new id there is a chance that they collide with the ids for another already existing link.
	The register link function does check for a collision, but only increments the given id by 1 without checking if that also collides.
	If it does, then it silently stomps on whatever data that id originally represented and then two links point to the same id.
	This could theoretically be fixed in a navigation baking step that also ensures all nav components are re-registered at once,
	but it can subtly and randomly break nav link notifies during development.
	Since the navmesh implementation shares a global id space, composing sub levels baked at different times could also cause collisions.
	So what we do is hash the level name since we can't know what levels will eventually be loaded at the same time.
	Then we can differentiate different links in each level by an index that we can check is actually unique (within the level)
	Then we pack it into the lower 31 bits of a uint32 (because of the aforementioned hidden narrowing conversions, the sign bit can't be used)
	*/

	//Property that allows the engine to serialize the link id and keep it consistent with the nav data.
	//It's available in register for PIE and packaged worlds, but in editor when re-running construction scripts it is invalid.
	UPROPERTY(VisibleAnywhere, NonPIEDuplicateTransient, Category = Test)
	uint32 LinkId = FNavigationLink::InvalidUserId;

	//stable path name for identifying the same object (actual address is not stable)
	FName CachedObjectPathName;


	//Helper struct to manage link ids
	union FUniqueLinkId
	{
		static constexpr uint32 NumIndexBits = 12;
		static constexpr uint32 NumHashBits = 31 - NumIndexBits;

		//A single level can contain this many different nav links
		static constexpr uint32 NumIndices = 1 << NumIndexBits;
		static constexpr uint32 NumHashes = 1 << NumHashBits;

		//Number of levels that are used to compute the chance of collision
		static constexpr uint32 NumLevels = 100;

		//The chance that this many levels will have a hash collision
		static constexpr double Collisions = double(NumLevels * (NumLevels - 1)) / double(2 * NumHashes);

		FUniqueLinkId() = default;

		FORCEINLINE FUniqueLinkId(uint32 InLinkId) : LinkId(InLinkId) {};

		FORCEINLINE FUniqueLinkId(const UUnitNavLinkComponent* InComponent, uint32 InIndex) : FUniqueLinkId(InComponent->GetComponentLevel(), InIndex) {};

		FORCEINLINE FUniqueLinkId(const ULevel* InLevel, uint32 InIndex) : FUniqueLinkId(TruncatedLevelNameHash(InLevel), InIndex) {};

		FORCEINLINE FUniqueLinkId(uint32 InLevelHash, uint32 InIndex) : Index(InIndex), LevelHash(InLevelHash), SignBit(0)
		{
			check(InLevelHash < NumHashes);
			check(InIndex < NumIndices);
		};

		FString ToString() const;

		//Generates the hash for a level name and truncates it to the available number of hash bits
		static uint32 TruncatedLevelNameHash(const ULevel* InLevel);

		uint32 LinkId = FNavigationLink::InvalidUserId;

		struct {

			//Guaranteed unique index, since all links in level are available at once we can check for uniqueness
			uint32 Index : NumIndexBits;

			//Hash of level name, the best we can do is make it unlikely that indices from two different levels will overlap
			//Hash is placed in the high bits to reduce the chance of overlaps on built in generated ids (which are usually very small)
			uint32 LevelHash : NumHashBits;

			//Can't use sign bit because Unreal truncates link ids in narrowing conversions all over the place.
			uint32 SignBit : 1;
		};
		
	};

	struct FLevelLinkIndices
	{
		//Maps a level relative path name for an object to the known FUniqueLinkId index that it should use
		TMap<FName, uint32> PathNameToIndex;

		//Indices that have been used and appear as a value in PathNameToIndex
		TSet<uint32> UsedIndices;
		//TArray<uint32> UsedIndices;

		//Minimum potentially unused index. Is checked against UsedIndices to find a free index.
		uint32 MinimumPotentiallyUnusedIndex = 0;
	};

	//Maps a truncated level hash to the indices that have been used in it
	static TMap<uint32, FLevelLinkIndices> LevelLinkIndices;

	void SetLinkId(uint32 NewLinkId);

	//Finds the link id for this component
	//Returns whether this component found a valid id to use
	bool FindLinkId();

	//Finds existing link id or creates new if there is none
	//Returns whether this component contains a valid id
	bool AcquireLinkId();

	//If this has a valid link id, ensures it exists in the link indices map
	void AddLinkId();

};