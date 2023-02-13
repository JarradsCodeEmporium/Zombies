//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PooledActor.generated.h"

USTRUCT(BlueprintType)
struct FPooledActorSpawnParameters
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = Game)
	AActor* Owner = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Game)
	APawn* Instigator = nullptr;

};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UPooledActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ZOMBIES_API IPooledActor
{
	GENERATED_BODY()

public:

	//Called when actor enters gameplay.
	UFUNCTION(BlueprintNativeEvent, Category = Game)
	void BeginPlayPooled();

	//Called when actor leaves gameplay. Should reset gameplay values ready for its next use
	UFUNCTION(BlueprintNativeEvent, Category = Game)
	void EndPlayPooled(EEndPlayReason::Type Reason);

};
