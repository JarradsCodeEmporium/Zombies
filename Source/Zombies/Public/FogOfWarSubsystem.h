//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpatialHashMap.h"
#include "FogOfWarCommon.h"
#include "FogOfWarSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIES_API UFogOfWarSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:

	UFogOfWarSubsystem();

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	virtual void Tick(float DeltaTime) override;

	FORCEINLINE virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UFogOfWarSubsystem, STATGROUP_Tickables);};

	FORCEINLINE const auto& GetStaticOccluders() const { return StaticOccluders; }

	FORCEINLINE const auto& GetFogOfWarActors() const { return FogOfWarActors; }

	void RegisterFogOfWarActor(AActor* Actor);

	void UnregisterFogOfWarActor(AActor* Actor);

	void UpdateFogOfWarActors();

protected:

	//Gathers all static meshes in the level that overlap the fog of war collision channel
	void GatherStaticOccluders();

	TSpatialHashMap<FBox2D, FFogOfWarOccluderInstance> StaticOccluders;

	TSpatialHashMap<FVector2D, TWeakObjectPtr<AActor>> FogOfWarActors;
};
