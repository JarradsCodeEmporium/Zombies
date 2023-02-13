//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PooledActor.h"
#include "PooledActorSubsystem.generated.h"


USTRUCT()
struct FActivePooledActor
{
	GENERATED_BODY()
public:

	int32 Index = -1;

};

USTRUCT()
struct FInactivePooledActor
{
	GENERATED_BODY()
public:

	UPROPERTY()
	AActor* Actor = nullptr;

	int32 Index = -1;

	//RealTimeSeconds() since it was last used.
	double RealTimeAtLastActive = -1.f;

};

USTRUCT()
struct FPooledActorType
{
	GENERATED_BODY()
public:

	//Actors currently active in the game world
	UPROPERTY()
	TMap<AActor*, FActivePooledActor> Active;

	//Stack of inactive actors so that we don't cycle through and refresh expiry timers when we really don't need to.
	UPROPERTY()
	TArray<FInactivePooledActor> Inactive;

};




/**
 * Interface for spawning and destroying pooled actors in a game world.
 */
UCLASS()
class ZOMBIES_API UPooledActorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	//"Spawn" an actor using the actor pool.
	UFUNCTION(BlueprintCallable, Category = Game, Meta = (WorldContext = "WorldContextObject"))
	static FORCEINLINE AActor* SpawnPooledActor(UObject* WorldContextObject, TSubclassOf<class AActor> Class, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters)
	{
		return StaticSpawnPooledActor(WorldContextObject, Class, Transform, SpawnParameters);
	}

	template <typename T>
	static FORCEINLINE T* SpawnPooledActor(UObject* WorldContextObject, TSubclassOf<T> Class = nullptr, const FTransform& Transform = FTransform::Identity, const FPooledActorSpawnParameters& SpawnParameters = FPooledActorSpawnParameters{})
	{
		return Cast<T>(StaticSpawnPooledActor(WorldContextObject, Class.Get() ? Class : T::StaticClass(), Transform, SpawnParameters));
	}

	//"Destroy" an actor using the actor pool
	UFUNCTION(BlueprintCallable, Category = Game)
	static void DestroyPooledActor(AActor* Actor, EEndPlayReason::Type Reason);

protected:

	static AActor* StaticSpawnPooledActor(UObject* WorldContextObject, TSubclassOf<class AActor> Class, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters);

	//Spawn an actor using the pooling system
	AActor* SpawnPooledActorInternal(TSubclassOf<class AActor> Class, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters);

	//Destroy a pooled actor by returning it to the pool
	void DestroyPooledActorInternal(class AActor* Actor, EEndPlayReason::Type Reason);

	void SpawnedFromPool(class AActor* Actor, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters);

	void DestroyedToPool(class AActor* Actor, const FTransform& InactiveTransform, EEndPlayReason::Type Reason);

	//Remove expired pooled objects
	UFUNCTION()
	void Flush();

	//Assign world space slots at the edge of game world for pooled actors
	FTransform GetInactiveTransform(int32 Index);

	int32 GetNewIndex();

	int32 NextPooledActorIndex = 0;

	TArray<int32> UnusedIndices;

	UPROPERTY()
	TMap<UClass*, FPooledActorType> PooledActors;

	FTimerHandle FlushHandle;

};
