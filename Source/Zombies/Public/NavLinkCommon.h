//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "NavLinkCommon.generated.h"


USTRUCT(BlueprintType)
struct FFollowNavLinkParams
{
	GENERATED_BODY()
public:

	//Generic tag that specifies the type of the link. It is up to the unit to decide exactly how to handle links of this kind.
	UPROPERTY(BlueprintReadWrite, Category = Unit)
	FGameplayTag LinkTag;

	//The location that the unit should be in at the end of the nav link follow
	UPROPERTY(BlueprintReadWrite, Category = Unit)
	FVector GoalLocation;

	//The nav link component that is being followed. Units can downcast to extract more info if necessary.
	UPROPERTY(BlueprintReadWrite, Category = Unit)
	class UActorComponent* NavLinkComponent = nullptr;
};