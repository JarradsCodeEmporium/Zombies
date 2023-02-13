//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NavLinkCommon.h"
#include "NavLinkAbility.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNavLinkAbility : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for abilities that can implement following a nav link.
 * e.g. an open door ability, jump over low obstacle ability, etc..
 */
class ZOMBIES_API INavLinkAbility
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = NavLink)
	void SetFollowNavLinkParams(const FFollowNavLinkParams& Params);
};
