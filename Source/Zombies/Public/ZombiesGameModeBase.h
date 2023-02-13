// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTags.h"
#include "ZombiesGameModeBase.generated.h"

USTRUCT(BlueprintType)
struct ZOMBIES_API FTeamTag
{
	GENERATED_BODY()
public:

	//The tag for this team. When a unit has a team tag, it belongs to the first matching team.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teams)
	FGameplayTag TeamTag;

	//Other teams which this team likes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teams)
	FGameplayTagContainer Friendly;

	//Other teams which this team hates
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teams)
	FGameplayTagContainer Hostile;
};

/**
 * 
 */
UCLASS()
class ZOMBIES_API AZombiesGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	//@todo: define team relationships as part of this object (the handler uses a weak this ptr to get the current game mode object)

	//Maps a tag to its FGenericTeamId used by builtin UE systems like AI
	UFUNCTION(BlueprintCallable, Category = Teams)
	FGenericTeamId GetGenericTeamIdForTag(FGameplayTag Tag) const;

	//Static team id attitude solver installed in InitGame()
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Teams)
	ETeamAttitude::Type GetTeamAttitude(FGenericTeamId TeamA, FGenericTeamId TeamB) const;

	//Configures teams in the game.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teams)
	TArray<FTeamTag> TeamTags;

protected:

	void UpdateAttitudeTable();

	//Precomputed attitude table, saves having to do all the map lookups and tag matching when the perception system is trying to tick perceptions
	TArray<uint8> AttitudeTable;

	//The stride for the attitude table
	uint8 AttitudeTableStride = 0;
};
