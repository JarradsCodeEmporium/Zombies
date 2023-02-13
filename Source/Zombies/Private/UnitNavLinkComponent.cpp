//Copyright Jarrad Alexander 2022


#include "UnitNavLinkComponent.h"
#include "NavLinkRenderingProxy.h"
#include "AI/NavigationSystemHelpers.h"
#include "NavigationSystemTypes.h"
#include "NavAreas/NavArea_Default.h"
#include "Engine/CollisionProfile.h"
#include "NavigationSystem.h"
#include "UnitInterface.h"
#include "Navigation/PathFollowingComponent.h"

UUnitNavLinkComponent::UUnitNavLinkComponent()
{
	Mobility = EComponentMobility::Stationary;

	BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	SetGenerateOverlapEvents(false);

	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::EvenIfNotCollidable;

	bCanEverAffectNavigation = true;

	bNavigationRelevant = true;

	Link.SetAreaClass(UNavArea_Default::StaticClass());

	bWantsInitializeComponent = true;

	if (!HasAnyFlags(RF_ClassDefaultObject))
		CachedObjectPathName = *GetPathName(GetComponentLevel());

}

void UUnitNavLinkComponent::GetLinkData(FVector& Left, FVector& Right, ENavLinkDirection::Type& Direction) const
{
	Left = Link.Left;
	Right = Link.Right;
	Direction = Link.Direction;
}

void UUnitNavLinkComponent::GetSupportedAgents(FNavAgentSelector& OutSupportedAgents) const
{
	OutSupportedAgents = Link.SupportedAgents;
}

TSubclassOf<UNavArea> UUnitNavLinkComponent::GetLinkAreaClass() const
{
	return Link.GetAreaClass();
}

uint32 UUnitNavLinkComponent::GetLinkId() const
{
	return LinkId;
}

void UUnitNavLinkComponent::UpdateLinkId(uint32 NewUniqueId)
{
	//UE_LOG(LogTemp, Warning, TEXT("Link %s externally updated id %s to %s"), *GetPathName(), *FUniqueLinkId{ LinkId }.ToString(), *FUniqueLinkId{ NewUniqueId }.ToString());

	SetLinkId(NewUniqueId);

}

bool UUnitNavLinkComponent::IsLinkPathfindingAllowed(const UObject* Querier) const
{
	return true;
}

bool UUnitNavLinkComponent::OnLinkMoveStarted(UObject* PathFollowingComponent, const FVector& GoalLocation)
{
	//UE_LOG(LogTemp, Warning, TEXT("%s starting link move to %s"), PathFollowingComponent ? *PathFollowingComponent->GetPathName() : TEXT("null"), *GoalLocation.ToString());

	auto Component = Cast<UPathFollowingComponent>(PathFollowingComponent);

	if (!Component)
		return false;

	AActor* InterfaceTarget = nullptr;

	//Get unit interface from actor owner or owners controlled pawn
	if (Component->GetOwner()->Implements<UUnitInterface>())
	{
		InterfaceTarget = Component->GetOwner();
	}
	else if (auto Controller = Cast<AController>(Component->GetOwner()))
	{
		if (auto Pawn = Controller->GetPawn())
			if (Pawn->Implements<UUnitInterface>())
				InterfaceTarget = Pawn;
	}

	if (!InterfaceTarget)
		return false;

	FFollowNavLinkParams Params;

	Params.NavLinkComponent = this;

	Params.GoalLocation = GoalLocation;

	Params.LinkTag = LinkTag;

	IUnitInterface::Execute_FollowNavLink(InterfaceTarget, Params);

	return true;
}

void UUnitNavLinkComponent::OnLinkMoveFinished(UObject* PathFollowingComponent)
{
	//UE_LOG(LogTemp, Warning, TEXT("%s finished link move"), PathfindingComponent ? *PathfindingComponent->GetPathName() : TEXT("null"));

}

FBoxSphereBounds UUnitNavLinkComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox LocalBounds(ForceInit);

	LocalBounds += Link.Left;
	LocalBounds += Link.Right;

	auto WorldBounds = LocalBounds.TransformBy(LocalToWorld);

	return WorldBounds;
}

void UUnitNavLinkComponent::GetNavigationData(FNavigationRelevantData& Data) const
{
	NavigationHelper::ProcessNavLinkAndAppend(&Data.Modifiers, NavigationHelper::FNavLinkOwnerData(*this), { GetNavigationLink()});
}

bool UUnitNavLinkComponent::IsNavigationRelevant() const
{
	return true;
}

bool UUnitNavLinkComponent::GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const
{
	OutLink.Add(GetNavigationLink());
	return true;
}

FPrimitiveSceneProxy* UUnitNavLinkComponent::CreateSceneProxy()
{
	return new FNavLinkRenderingProxy(this);
}

FNavigationLink UUnitNavLinkComponent::GetNavigationLink() const
{
	FNavigationLink Result = Link;

	//No matter what shenanigans unreal is up to with changing the property under our noses,
	//it will always match the id presented to the navigation system
	Result.UserId = LinkId;

	return Result;
}

#if WITH_EDITOR

void UUnitNavLinkComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UUnitNavLinkComponent, Link))
	{
		InitializeLinksAreaClasses();
		UpdateBounds();
	}

}

void UUnitNavLinkComponent::PostEditUndo()
{
	Super::PostEditUndo();

	InitializeLinksAreaClasses();

}

void UUnitNavLinkComponent::PostEditImport()
{
	Super::PostEditImport();

	InitializeLinksAreaClasses();
}

#endif // WITH_EDITOR



TMap<uint32, UUnitNavLinkComponent::FLevelLinkIndices> UUnitNavLinkComponent::LevelLinkIndices;

void UUnitNavLinkComponent::OnRegister()
{
	Super::OnRegister();

	InitializeLinksAreaClasses();

	FNavigationSystem::OnComponentRegistered(*this);

	if (AcquireLinkId())
		UNavigationSystemV1::RequestCustomLinkRegistering(*this, this);
	
}

void UUnitNavLinkComponent::OnUnregister()
{
	Super::OnUnregister();

	FNavigationSystem::OnComponentUnregistered(*this);

	//if (LinkId != FNavigationLink::InvalidUserId)
	UNavigationSystemV1::RequestCustomLinkUnregistering(*this, this);

}

void UUnitNavLinkComponent::PostLoad()
{
	Super::PostLoad();

	AddLinkId();

}

void UUnitNavLinkComponent::InitializeLinksAreaClasses()
{
	Link.InitializeAreaClass();
	
}

void UUnitNavLinkComponent::SetLinkId(uint32 NewLinkId)
{
	LinkId = NewLinkId;
	Link.UserId = NewLinkId;
}

bool UUnitNavLinkComponent::FindLinkId()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
		return false;
	
	auto Level = GetComponentLevel();

	if (!Level)
		return false;

	FUniqueLinkId UniqueLinkId{ Level, 0 };

	auto LinkIds = LevelLinkIndices.Find(UniqueLinkId.LevelHash);

	if (!LinkIds)
		return false;

	auto Index = LinkIds->PathNameToIndex.Find(CachedObjectPathName);

	if (!Index)
		return false;

	UniqueLinkId.Index = *Index;

	SetLinkId(UniqueLinkId.LinkId);

	return true;
}

bool UUnitNavLinkComponent::AcquireLinkId()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
		return false;

	auto Level = GetComponentLevel();

	if (!Level)
		return false;

	FUniqueLinkId UniqueLinkId{ Level, 0 };

	auto& LinkIds = LevelLinkIndices.FindOrAdd(UniqueLinkId.LevelHash);

	if (auto Index = LinkIds.PathNameToIndex.Find(CachedObjectPathName))
	{
		//Link already exists, use it
		UniqueLinkId.Index = *Index;

		if (LinkId != FNavigationLink::InvalidUserId && UniqueLinkId.LinkId != LinkId)
		{
			//UE_LOG(LogTemp, Error, TEXT("Link %s AcquireLinkId() acquired a different value. Current: %s, Acquired: %s"), *GetPathName(), *FUniqueLinkId{ LinkId }.ToString(), *UniqueLinkId.ToString());
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Link %s AcquireLinkId() acquired existing id %s"), *GetPathName(), *UniqueLinkId.ToString());
		}

		SetLinkId(UniqueLinkId.LinkId);

		return true;
	}
	

	//Link does not exist, need to gather a new index

	if (LinkIds.UsedIndices.Num() >= FUniqueLinkId::NumIndices)
	{
		UE_LOG(LogTemp, Error, TEXT("Link %s AcquireLinkId() exceeded maximum index of %u. Path following notifies on this link will not work"), *GetPathName(), FUniqueLinkId::NumIndices - 1);
		
		return false;
	}

	//@todo: i am too tired to write this
	//for (uint32 MinIndex = 0, MaxIndex = LinkIds.UsedIndices.Num(); MinIndex < MaxIndex;)
	//{
	//	uint32 CenterIndex = (MaxIndex - MinIndex) / 2 + MinIndex;

	//	if ()

	//}

	//simple linear search that i am not too tired to write
	//guaranteed to end because of previous check for completely full index space
	while (LinkIds.UsedIndices.Contains(LinkIds.MinimumPotentiallyUnusedIndex))
		LinkIds.MinimumPotentiallyUnusedIndex = (LinkIds.MinimumPotentiallyUnusedIndex + 1) % FUniqueLinkId::NumIndices;

	UniqueLinkId.Index = LinkIds.MinimumPotentiallyUnusedIndex;


	LinkIds.PathNameToIndex.Add(CachedObjectPathName, UniqueLinkId.Index);
	LinkIds.UsedIndices.Add(UniqueLinkId.Index);

	//UE_LOG(LogTemp, Warning, TEXT("Link %s AcquireLinkId() acquired new id %s"), *GetPathName(), *UniqueLinkId.ToString());

	SetLinkId(UniqueLinkId.LinkId);

	return true;
}

void UUnitNavLinkComponent::AddLinkId()
{
	if (LinkId == FNavigationLink::InvalidUserId || HasAnyFlags(RF_ClassDefaultObject))
		return;

	FUniqueLinkId UniqueLinkId{ LinkId };

	auto &LinkIds = LevelLinkIndices.FindOrAdd(UniqueLinkId.LevelHash);

	if (auto Existing = LinkIds.PathNameToIndex.Find(CachedObjectPathName); Existing && *Existing != UniqueLinkId.Index)
	{
		//UE_LOG(LogTemp, Error, TEXT("Link %s AddLinkId() ID mismatch. Existing: %s, Adding: %s"), *GetPathName(), *FUniqueLinkId{ UniqueLinkId.LevelHash, *Existing }.ToString() , *FUniqueLinkId{ LinkId }.ToString());
	}
	else if (Existing)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Link %s AddLinkId() adding already existing id %s"), *GetPathName(), *FUniqueLinkId{ LinkId }.ToString());
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Link %s AddLinkId() adding new id %s"), *GetPathName(), *FUniqueLinkId{ LinkId }.ToString());
	}

	LinkIds.PathNameToIndex.Add(CachedObjectPathName, UniqueLinkId.Index);
	LinkIds.UsedIndices.Add(UniqueLinkId.Index);

}

uint32 UUnitNavLinkComponent::FUniqueLinkId::TruncatedLevelNameHash(const ULevel* InLevel)
{
	//If we use a full path for the hash, will have to deal with PIE name mangling.
	//Currently using just the level object name seems to work.

	FUniqueLinkId Temp;

	//Bit field applies the appropriate truncation
	Temp.LevelHash = GetTypeHash(InLevel->GetName());

	return Temp.LevelHash;
}

FString UUnitNavLinkComponent::FUniqueLinkId::ToString() const
{
	return FString::Printf(TEXT("{LevelHash: %u, Index %u, Sign: %u}"), LevelHash, Index, SignBit);
}
