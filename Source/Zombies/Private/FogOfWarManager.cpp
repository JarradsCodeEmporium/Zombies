//Copyright Jarrad Alexander 2022


#include "FogOfWarManager.h"
#include "EngineUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BoxComponent.h"
#include "FogOfWarUtils.h"
#include "FogOfWarOccluderSubsystem.h"
#include "FogOfWarActor.h"
#include "DrawDebugHelpers.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h" 
#endif


// Sets default values
AFogOfWarManager::AFogOfWarManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DiscoveredAreaBounds = CreateDefaultSubobject<UBoxComponent>("DiscoveredAreaBounds");

	SetRootComponent(DiscoveredAreaBounds);

	DiscoveredAreaBounds->SetAbsolute(false, true, false);

	//DiscoveredAreaBounds->SetBoxExtent(FVector{}, false);
	DiscoveredAreaBounds->InitBoxExtent({10000.0, 10000.0, 1000.0});

	//StaticOccluders.SetTransform(FVector2D::Zero(), FVector2D{ 500.0, 500.0 });
}

// Called when the game starts or when spawned
void AFogOfWarManager::BeginPlay()
{
	Super::BeginPlay();

	//Allow any begin play construction to also be considered in static occluders
	//GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AFogOfWarManager::GatherStaticOccluders);



	//FVector2D Scale{ 100,100 };

	//Testo.SetTransform(FVector2D::ZeroVector, Scale);

	//auto RandomBox = [&]() -> FBox2D
	//{
	//	FVector2D Origin{ FMath::FRandRange(-3000.0, 3000.0), FMath::FRandRange(-3000.0, 3000.0) };

	//	FVector2D Size{ FMath::FRandRange(10.0, 500.0), FMath::FRandRange(10.0, 500.0) };

	//	return Testo.WorldToLocal(FBox2D{ Origin, Origin + Size });
	//};

	//auto DrawBox = [&](FBox2D Box, double Z, FColor Color)
	//{
	//	Box = Testo.LocalToWorld(Box);

	//	FVector2D Center, Extent;

	//	Box.GetCenterAndExtents(Center, Extent);

	//	DrawDebugBox(GetWorld(), FVector{ Center, Z }, FVector{ Extent, 0.0 }, Color, true);
	//};

	//TArray<int32> Indices;

	//for (int32 i = 0; i < 100; ++i)
	//{
	//	auto Box = RandomBox();
	//	
	//	DrawBox(Box, 100.0, FColor::Blue);

	//	Indices.Add(Testo.Add(Box));
	//}

	//FBox2D QueryBox = Testo.WorldToLocal({ FVector2D{-1000}, FVector2D{1000} });

	//DrawBox(QueryBox, 140.0, FColor::Magenta);

	//for (auto It = Testo.CreateConstIterator(QueryBox); It; ++It)
	//{
	//	DrawBox(It.GetElementBounds(), 150.0, FColor::Green);
	//}

}

// Called every frame
void AFogOfWarManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//UpdateFogOfWarActors();

	//for (auto& Element : FogOfWarActors.GetElements())
	//	DrawDebugPoint(GetWorld(), FVector{ FogOfWarActors.LocalToWorld(Element.Geometry), 200.0 }, 20.f, FColor::Blue, false, DeltaTime);
}


AFogOfWarManager* AFogOfWarManager::GetFogOfWarManager(UObject* WorldContext)
{
	check(WorldContext);

	int32 NumManagers = 0;

	AFogOfWarManager* Result = nullptr;

	for (TActorIterator<AFogOfWarManager> It(WorldContext->GetWorld()); It; ++It)
	{
		if (!IsValid(*It))
			continue;

		++NumManagers;

		//Always choose the first visited manager, since that happens be more stable
		if (!Result)
			Result = *It;
	}

#if WITH_EDITOR
	if (NumManagers == 0)
		FMessageLog(FName("MapCheck")).Error(FText::FromString(TEXT("Failed to find AFogOfWarManager in level. Did you forget to place one?")));
	else if (NumManagers > 1)
		FMessageLog(FName("MapCheck")).Error(FText::FromString(TEXT("Found too many AFogOfWarManager in level. There should be only one in the persistent level.")));
#endif

	return Result;
}

UTextureRenderTarget2D* AFogOfWarManager::CreateDiscoveredAreaRenderTarget(UObject* Outer) const
{
	return FogOfWarUtils::CreateRenderTarget(Outer, DiscoveredAreasResolution);
}

FTransform AFogOfWarManager::GetDiscoveredAreaCanvasTransform() const
{
	return FogOfWarUtils::GetCanvasTransform(DiscoveredAreaBounds);
}

void AFogOfWarManager::RegisterVisionComponent(UFogOfWarVisionComponent* Component)
{
	if (!Component)
		return;

	VisionComponents.Add(Component);
}

void AFogOfWarManager::UnregisterVisionComponent(UFogOfWarVisionComponent* Component)
{
	if (!Component)
		return;

	VisionComponents.Remove(Component);
}
//
//void AFogOfWarManager::RegisterFogOfWarActor(AActor* Actor)
//{
//	if (!Actor || !Actor->Implements<UFogOfWarActor>())
//		return;
//
//	if (IFogOfWarActor::Execute_GetFogOfWarActorID(Actor) >= 0)
//		return;
//
//	auto ID = FogOfWarActors.AddElementWorldSpace(FVector2D{ Actor->GetActorLocation() }, Actor);
//
//	IFogOfWarActor::Execute_SetFogOfWarActorID(Actor, ID);
//}
//
//void AFogOfWarManager::UnregisterFogOfWarActor(AActor* Actor)
//{
//	if (!Actor || !Actor->Implements<UFogOfWarActor>())
//		return;
//
//	auto ID = IFogOfWarActor::Execute_GetFogOfWarActorID(Actor);
//
//	FogOfWarActors.RemoveElement(ID);
//
//	IFogOfWarActor::Execute_SetFogOfWarActorID(Actor, -1);
//}
//
//void AFogOfWarManager::UpdateFogOfWarActors()
//{
//	//Safe to modify position while iterating elements (but not cells)
//	for (auto It = FogOfWarActors.GetElements().CreateConstIterator(); It; ++It)
//		if (It->Value.IsValid())
//			FogOfWarActors.MoveElementWorldSpace(It.GetIndex(), FVector2D{ It->Value->GetActorLocation() });
//	
//}
//
//void AFogOfWarManager::GatherStaticOccluders()
//{
//	StaticOccluders.Empty();
//
//	auto DrawBox = [&](FBox2D Box, double Z, FColor Color)
//	{
//
//		FVector2D Center, Extent;
//
//		Box.GetCenterAndExtents(Center, Extent);
//
//		DrawDebugBox(GetWorld(), FVector{ Center, Z }, FVector{ Extent, 0.0 }, Color, true);
//	};
//
//	auto GI = GetWorld()->GetGameInstance();
//
//	if (!GI)
//		return;
//
//	auto Subsystem = GI->GetSubsystem<UFogOfWarOccluderSubsystem>();
//
//	if (!Subsystem)
//		return;
//
//	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
//	{
//		if (It->GetRootComponent() && It->GetRootComponent()->Mobility == EComponentMobility::Movable)
//			continue;
//
//		for (auto Component : It->GetComponents())
//		{
//			auto StaticMeshComponent = Cast<UStaticMeshComponent>(Component);
//
//			if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
//				continue;
//
//			if (StaticMeshComponent->GetCollisionResponseToChannel(StaticFogOfWarOccluderChannel) == ECollisionResponse::ECR_Ignore)
//				continue;
//
//			auto OccluderMesh = Subsystem->GetOccluderMesh(StaticMeshComponent->GetStaticMesh());
//
//			if (!OccluderMesh)
//				continue;
//
//			FFogOfWarOccluderInstance Instance;
//
//			Instance.Transform = StaticMeshComponent->GetComponentTransform();
//
//			Instance.OccluderMesh = OccluderMesh;
//
//			auto& Bounds = Instance.OccluderMesh->Bounds;
//
//			FBox2D WorldBounds = FogOfWarUtils::TransformBox2D(Instance.Transform, Instance.OccluderMesh->Bounds);
//
//			StaticOccluders.AddElementWorldSpace(WorldBounds, MoveTemp(Instance));
//		}
//	}
//
//}

