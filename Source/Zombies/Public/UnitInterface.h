//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTags.h"
#include "NavLinkCommon.h"
#include "GameplaySelectable.h"
#include "UnitInterface.generated.h"


UINTERFACE(MinimalAPI, BlueprintType)
class UUnitInterface : public UGameplaySelectable
{
	GENERATED_BODY()
};

/**
 * Interface for game units like player pawns, enemies, friendly npc's, etc.
 */
class ZOMBIES_API IUnitInterface : public IGameplaySelectable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Unit)
	class UAbilityComponent* GetAbilityComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Unit)
	class UUnitStatsComponent* GetUnitStatsComponent() const;

	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Unit)
	//class UCooldownComponent* GetCooldownComponent() const;

	//Unit should perform an action that implements following the given nav link.
	//e.g. use a "open door" ability if the nav link tag represents a door
	//@param Params: see struct definition for details on each member.
	//@return: Whether the unit has begun to follow the nav link.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Unit)
	bool FollowNavLink(const FFollowNavLinkParams& Params);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Unit)
	bool IsDead() const;

};
