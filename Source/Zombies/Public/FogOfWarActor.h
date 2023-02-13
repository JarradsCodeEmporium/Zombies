//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FogOfWarActor.generated.h"

UINTERFACE(MinimalAPI)
class UFogOfWarActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ZOMBIES_API IFogOfWarActor
{
	GENERATED_BODY()
public:

	//Actor should keep hold of this ID to identify itself in the fog of war actors spatial hash map
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = FogOfWar)
	void SetFogOfWarActorID(int32 NewFogOfWarID);

	//Get the fog of war actor ID. Should default to -1.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = FogOfWar)
	int32 GetFogOfWarActorID() const;

	//Whether this actor is visible to the given component.
	//Should generally just use return UFogOfWarVisionComponent::CanSeePoint(GetActorLocation());
	//Note that the actor needs a component that overlaps with the vision channel of the vision component in order to be detected in the first place
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = FogOfWar)
	bool IsFogOfWarVisible(const class UFogOfWarVisionComponent* Component) const;

	//Notifies the actor that they have become visible or not visible to a specific vision component.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = FogOfWar)
	void SetFogOfWarVisionVisibility(class UFogOfWarVisionComponent* Component, bool bIsVisible);

	//Notifies the actor that they have become visible or not visible to at least one of the vision components of a display component.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = FogOfWar)
	void SetFogOfWarDisplayVisibility(class UFogOfWarDisplayComponent* Component, bool bIsVisible);

};
