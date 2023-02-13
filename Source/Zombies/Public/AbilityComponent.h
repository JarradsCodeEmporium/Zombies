//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "AbilityCommon.h"
#include "AbilityComponent.generated.h"



//Allows pawns to perform abilities using a queue of AAbility actors
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UAbilityComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAbilityComponent();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& Container) const override;

	//Checks whether an ability of the given class could be activated.
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool CanBeginAbilityByClass(TSubclassOf<class AAbility> AbilityClass) const;

	//Checks whether an ability instance can be activated.
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool CanBeginAbility(class AAbility* Ability) const;

	//Tries to create and activate an ability of the given class.
	//@return: Whether the ability was successfully created and activated. Can fail if already active.
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool BeginAbilityByClass(TSubclassOf<class AAbility> AbilityClass);

	//Tries to activate an ability.
	//@return: Whether the ability was successfully activated. Can fail if already active.
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool BeginAbility(class AAbility* Ability);

	//Ends a currently active ability.
	//@param Reason: Controls how the ability system will react to the end of this ability.
	//@return: Whether the ability existed and was active.
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool EndAbility(class AAbility* Ability, EEndAbilityReason Reason);

	//Ends all abilities that have these tags
	//@return: The number of abilities that were ended.
	UFUNCTION(BlueprintCallable, Category = Ability)
	int32 EndAbilitiesWithTags(const FGameplayTagContainer& EndTags, EEndAbilityReason Reason);

	//Ends all currently active abilities.
	//@return: The number of abilities that were ended
	UFUNCTION(BlueprintCallable, Category = Ability)
	int32 EndAllAbilities(EEndAbilityReason Reason);

	FORCEINLINE const auto& GetActiveAbilities() const { return ActiveAbilities; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	bool IsAbilityActive(class AAbility* Ability) const;

	template <typename T>
	T* GetAvatarActor() const { return Cast<T>(GetAvatarActor()); }

	//Returns the actor that represents the high level "owner" of this ability component.
	UFUNCTION(BlueprintCallable, Category = Ability)
	virtual AActor* GetAvatarActor() const;

	template <typename T>
	T* GetController() const { return Cast<T>(GetController()); }

	UFUNCTION(BlueprintCallable, Category = Ability)
	AController* GetController() const;

	//Additional tags on this component that are manually managed by external code
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ability)
	FGameplayTagContainer CustomOwnedTags;

	//Get the automatically located unit stats component on the same actor as this ability component (if any).
	UFUNCTION(BlueprintCallable, Category = Ability)
	FORCEINLINE class UUnitStatsComponent* GetUnitStatsComponent() const { return UnitStatsComponent; }

	//Get the automatically located skeletal mesh component on the same actor as this ability component (if any).
	UFUNCTION(BlueprintCallable, Category = Ability)
	FORCEINLINE class USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

protected:

	//Abilities that have actually begun.
	UPROPERTY(BlueprintReadOnly, Transient, Meta = (AllowPrivateAccess = "True"))
	TArray<class AAbility*> ActiveAbilities;

	//The cached unit stats component on the same actor
	UPROPERTY(Transient)
	class UUnitStatsComponent* UnitStatsComponent;

	//The cached skeletal mesh component on the same actor
	UPROPERTY(Transient)
	class USkeletalMeshComponent* SkeletalMeshComponent;

	bool bIsEndingPlay = false;
};
