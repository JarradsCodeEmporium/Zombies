//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "AbilityCommon.generated.h"

class AAbility;
class UAbilityComponent;

UENUM(BlueprintType)
enum class EEndAbilityReason : uint8
{
	//Ability finished normally.
	Finished,

	//Ability was interrupted by another ability.
	Interrupted,

	//Ability was intentionally cancelled by user.
	Cancelled
};
