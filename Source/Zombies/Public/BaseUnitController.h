//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTags.h"
#include "BaseUnitController.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIES_API ABaseUnitController : public AAIController
{
	GENERATED_BODY()
	
protected:

	void BeginPlay() override;

public:


	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetTeamTag(FGameplayTag NewTeamTag);

	FORCEINLINE FGameplayTag GetTeamTag() const { return TeamTag; }

protected:
	//Tag based team alignment, maps to FGenericTeamId in current game mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetTeamTag, Category = Team, Meta = (AllowPrivateAccess = "True"))
	FGameplayTag TeamTag;

};
