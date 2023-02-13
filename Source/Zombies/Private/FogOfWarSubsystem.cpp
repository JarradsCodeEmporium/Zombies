//Copyright Jarrad Alexander 2022


#include "FogOfWarSubsystem.h"
#include "FogOfWarActor.h"
#include "FogOfWarOccluderSubsystem.h"
#include "EngineUtils.h"
#include "FogOfWarUtils.h"
#include "CollisionChannels.h"

UFogOfWarSubsystem::UFogOfWarSubsystem()
{

}

void UFogOfWarSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	//Allow any begin play construction to run first so it can be considered in static occluders
	InWorld.GetTimerManager().SetTimerForNextTick(this, &UFogOfWarSubsystem::GatherStaticOccluders);
}

void UFogOfWarSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateFogOfWarActors();
}

void UFogOfWarSubsystem::RegisterFogOfWarActor(AActor* Actor)
{
	if (!Actor || !Actor->Implements<UFogOfWarActor>())
		return;

	if (IFogOfWarActor::Execute_GetFogOfWarActorID(Actor) >= 0)
		return;

	auto ID = FogOfWarActors.AddElementWorldSpace(FVector2D{ Actor->GetActorLocation() }, Actor);

	IFogOfWarActor::Execute_SetFogOfWarActorID(Actor, ID);
}

void UFogOfWarSubsystem::UnregisterFogOfWarActor(AActor* Actor)
{
	if (!Actor || !Actor->Implements<UFogOfWarActor>())
		return;

	auto ID = IFogOfWarActor::Execute_GetFogOfWarActorID(Actor);

	FogOfWarActors.RemoveElement(ID);

	IFogOfWarActor::Execute_SetFogOfWarActorID(Actor, -1);
}

void UFogOfWarSubsystem::UpdateFogOfWarActors()
{
	//Safe to modify position while iterating elements (but not cells)
	for (auto It = FogOfWarActors.GetElements().CreateConstIterator(); It; ++It)
		if (It->Value.IsValid())
			FogOfWarActors.MoveElementWorldSpace(It.GetIndex(), FVector2D{ It->Value->GetActorLocation() });
	
}

void UFogOfWarSubsystem::GatherStaticOccluders()
{
	StaticOccluders.Empty();

	auto DrawBox = [&](FBox2D Box, double Z, FColor Color)
	{

		FVector2D Center, Extent;

		Box.GetCenterAndExtents(Center, Extent);

		DrawDebugBox(GetWorld(), FVector{ Center, Z }, FVector{ Extent, 0.0 }, Color, true);
	};

	auto GI = GetWorld()->GetGameInstance();

	if (!GI)
		return;

	auto Subsystem = GI->GetSubsystem<UFogOfWarOccluderSubsystem>();

	if (!Subsystem)
		return;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetRootComponent() && It->GetRootComponent()->Mobility == EComponentMobility::Movable)
			continue;

		for (auto Component : It->GetComponents())
		{
			auto StaticMeshComponent = Cast<UStaticMeshComponent>(Component);

			if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
				continue;

			if (StaticMeshComponent->GetCollisionResponseToChannel(ECollisionChannel::ECC_FogOfWar) == ECollisionResponse::ECR_Ignore)
				continue;
			
			auto OccluderMesh = Subsystem->GetOccluderMesh(StaticMeshComponent->GetStaticMesh());

			if (!OccluderMesh)
				continue;

			FFogOfWarOccluderInstance Instance;

			Instance.Transform = StaticMeshComponent->GetComponentTransform();

			Instance.OccluderMesh = OccluderMesh;

			auto& Bounds = Instance.OccluderMesh->Bounds;

			FBox2D WorldBounds = FogOfWarUtils::TransformBox2D(Instance.Transform, Instance.OccluderMesh->Bounds);

			StaticOccluders.AddElementWorldSpace(WorldBounds, MoveTemp(Instance));
		}
	}

}

