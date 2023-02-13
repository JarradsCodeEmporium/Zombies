//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTags.h"
#include "AbilityCommon.h"
#include "AbilityAction.h"
#include "Navigation/PathFollowingComponent.h"
#include "Ability.generated.h"

//Generic event on an ability.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityGenericDelegate, class AAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEndAbilityDelegate, class AAbility*, Ability, EEndAbilityReason, Reason);


//Base class for unit abilities.
UCLASS()
class ZOMBIES_API AAbility : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbility();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/*
	player command queue is abilities that start immediately, and then they have targeting/configuration stuff 
	and then use an action to wait for the head of the queue before running and waiting on inner abilities
	these inner abilities are the things that actually cause the unit to do the command.
	e.g. a grenade throw ability. player presses button to start a grenade throw ability
	the ability uses player input to specify a throw from position and a throw to position
	once these are confirmed, the ability adds itself to the queue, then waits for it to be at the head of the queue
	once at the head of the queue, it builds and begins a move to ability that actually drives the unit movements
	the ability waits for the move to complete (or fail) and then starts a grenade throw ability
	once the grenade throw ability is complete, the command ability releases itself from the head of the queue
	*/

	template <typename T>
	FORCEINLINE static T* CreateAbility(UObject* WorldContextObject, TSubclassOf<AAbility> Class = T::StaticClass())
	{
		if (!Class.Get() || !Class->IsChildOf<T>())
			//Cast would fail since Class is not a subclass of T, so don't construct an ability that would immediately become orphaned.
			return nullptr;

		return Cast<T>(CreateAbility(WorldContextObject, Class));
	}

	//Create an ability. This function should be used rather than SpawnActor().
	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (WorldContext = "WorldContextObject"))
	static AAbility* CreateAbility(UObject* WorldContextObject, TSubclassOf<AAbility> Class);

	//Initializes this ability as belonging to an ability component.
	//Called by ability component when created
	void SetAbilityComponent(class UAbilityComponent* InAbilityComponent);

	UFUNCTION(BlueprintCallable, Category = Ability)
	FORCEINLINE class UAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	//Set the command that this ability should implement (if any)
	FORCEINLINE void SetCommand(class ACommand* InCommand) { Command = InCommand; }

	//Gets the command that this ability is implementing (if any)
	UFUNCTION(BlueprintCallable, Category = Ability)
	FORCEINLINE class ACommand* GetCommand() const { return Command; }


	//Called by ability component when it is done with the ability, or by creator if discarding the ability before activating it.
	//Default implementation just destroys it.
	//@todo: This should change if abilities are moved to pooled actor lifetime.
	virtual void DestroyAbility();

	//Determines if the ability can be activated at this moment.
	//@param OwningAbilityComponent: The ability component which wants to try activating this ability
	//@return: Whether the ability can begin.
	UFUNCTION(BlueprintNativeEvent, Category = Ability)
	bool CanBeginAbility(const class UAbilityComponent* OwningAbilityComponent) const;

	//Called when the ability is linked to an ability component and begins its actions
	UFUNCTION(BlueprintNativeEvent, Category = Ability)
	void OnBeginAbility();

	//Called when the ability ends for any reason.
	UFUNCTION(BlueprintNativeEvent, Category = Ability)
	void OnEndAbility(EEndAbilityReason Reason);

	//Convenience function to end self.
	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (DefaultToSelf = "True", HideSelfPin = "True"))
	void EndAbility(EEndAbilityReason Reason);

	//@todo: input events like confirm, cancel, cursor position, other stuff...
	//@todo: possible visualization events like show/hide range radius, navigation path, ability icons etc. based on whether unit is selected?

	//Tags that are applied to the ability component when this ability is active
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ability, Meta = (AllowPrivateAccess = "True"))
	FGameplayTagContainer GrantedTags;

	//Tags on the ability component that are required in order for this ability to activate
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ability, Meta = (AllowPrivateAccess = "True"))
	FGameplayTagContainer RequiredTags;

	//Tags on the ability component that can't be present in order for this ability to activate
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ability, Meta = (AllowPrivateAccess = "True"))
	FGameplayTagContainer BlockedTags;

	//Tags that this ability itself has
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ability, Meta = (AllowPrivateAccess = "True"))
	FGameplayTagContainer AbilityTags;

	//This ability interrupts other abilities with these tags
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ability, Meta = (AllowPrivateAccess = "True"))
	FGameplayTagContainer InterruptAbilityTags;

	//Called by ability action to register itself as belonging to this ability
	void RegisterAbilityAction(class UAbilityAction* AbilityAction);

	//Called by ability action to unregister itself when complete or cancelled
	void UnregisterAbilityAction(class UAbilityAction* AbilityAction);

	FORCEINLINE const auto& GetAbilityActions() const { return AbilityActions; }

	//Cancels all actions with the given name.
	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (DefaultToSelf = "True", HideSelfPin = "True"))
	void CancelActionsByName(FName ActionName);

	//Cancels all actions
	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (DefaultToSelf = "True", HideSelfPin = "True"))
	void CancelAllActions();

	//Called after ability ends
	UPROPERTY(BlueprintAssignable, Category = Ability)
	FAbilityGenericDelegate OnBeginAbilityEvent;

	UPROPERTY(BlueprintAssignable, Category = Ability)
	FEndAbilityDelegate OnEndAbilityEvent;

protected:

	UPROPERTY(Transient)
	class UAbilityComponent* AbilityComponent;

	//Optional command that this ability should follow
	UPROPERTY(Transient)
	class ACommand* Command;

	//Ability actions that are currently tied to this ability.
	UPROPERTY(Transient)
	TArray<class UAbilityAction*> AbilityActions;

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityActionTest, EPathFollowingResult::Type, Result);

UCLASS()
class UTestAbilityAction : public UAbilityAction
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (BlueprintInternalUseOnly = "True", WorldContext = "WorldContextObject"))
	static UTestAbilityAction* TestAsyncAbilityAction(UObject* WorldContextObject, FName ActionName, FVector MoveTarget);

	virtual void Activate() override;

	//virtual void Cancel() override;

	UFUNCTION()
	void ReceiveMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UPROPERTY(BlueprintAssignable)
	FAbilityActionTest OnMoveComplete;

	FVector MoveTarget;

	FAIRequestID MoveRequestID;
};