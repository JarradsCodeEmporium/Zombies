//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AbilityAction.generated.h"

//Simple generic delegate with no arguments for pins that don't need any.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenericAbilityActionDelegate);

/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (ExposedAsyncProxy = Action))
class ZOMBIES_API UAbilityAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:

	//Create a new action. World context object must be the AAbility that owns the action.
	//Intended for use inside derived class factory functions.
	template <typename T>
	static T* NewAction(UObject* WorldContextObject, FName ActionName = NAME_None, UClass* Class = nullptr) 
	{ 
		if (Class && !Class->IsChildOf(T::StaticClass()))
			return nullptr;

		return Cast<T>(NewAction(WorldContextObject, ActionName, Class ? Class : T::StaticClass())); 
	}

	//Create a new action. World context object must be the AAbility that owns the action.
	//Intended for use inside derived class factory functions.
	static UAbilityAction* NewAction(UObject* WorldContextObject, FName ActionName, UClass* Class);

	//Unregisters the action with the owning ability.
	virtual void SetReadyToDestroy() override;

	//Gets the ability that this action is owned by. 
	class AAbility* GetAbility() const;

	//Gets the ability component that owns the ability and therefore this action.
	class UAbilityComponent* GetAbilityComponent() const;

	//Cancel the action, stopping whatever processes it is doing and prevents delegates from being fired.
	UFUNCTION(BlueprintCallable, Category = AbilityAction)
	virtual void Cancel();

	//When action is no longer valid, it should not broadcast its delegates
	virtual bool ShouldBroadcastDelegates() const;

	//Whether this action is registered with an ability
	bool IsRegistered() const;

	//Gets the timer manager for the owning abilities world
	class FTimerManager* GetTimerManager() const;

	UFUNCTION(BlueprintCallable, Category = AbilityAction)
	FORCEINLINE FName GetActionName() const { return ActionName; }

protected:

	FName ActionName;

};
