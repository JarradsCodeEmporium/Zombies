//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects which are interactable such as via the context action from the players world cursor
 * Note that objects that use this interface do not also need to implement IGameplaySelectable. They are distinct concepts.
 */
class ZOMBIES_API IInteractable
{
	GENERATED_BODY()
public:

	//Whether the player pawn can interact with the object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interactable)
	bool CanInteract(const class APawn* Pawn) const;

	//Player pawn wants to interact with the object.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interactable)
	void Interact(class APawn* Pawn);

};
