//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UnitInterface.h"
#include "GameplaySelectable.h"
#include "Interactable.h"
#include "GameplayTagAssetInterface.h"
#include "FogOfWarActor.h"
#include "GenericTeamAgentInterface.h"
#include "BaseUnit.generated.h"


//Base class for player and NPC units
UCLASS()
class ZOMBIES_API ABaseUnit : public ACharacter, 
	public IUnitInterface,
	//public IGameplaySelectable,
	public IInteractable,
	public IGameplayTagAssetInterface,
	public IFogOfWarActor,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseUnit();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void SetOwner(AActor* NewOwner) override;

	//Begin IGameplayTagAssetInterface

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& Container) const override;

	//End IGameplayTagAssetInterface


	//Begin IUnitInterface

	FORCEINLINE virtual class UAbilityComponent* GetAbilityComponent_Implementation() const override { return AbilityComponent; }

	FORCEINLINE virtual class UUnitStatsComponent* GetUnitStatsComponent_Implementation() const override { return UnitStatsComponent; }

	//FORCEINLINE class UCooldownComponent* GetCooldownComponent_Implementation() const override { return CooldownComponent; }

	virtual bool FollowNavLink_Implementation(const FFollowNavLinkParams& Params) override;

	virtual bool IsDead_Implementation() const override;

	//End IUnitInterface

	//FORCEINLINE const auto& GetCommandQueueRef() const { return CommandQueue; }


	//Begin IGameplaySelectable

	virtual UGameplaySelectableComponent* GetSelectableComponent_Implementation() const;

	//End IGameplaySelectable

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Unit | FogOfWar")
	void InitSelection();

	//Begin IInteractable

	FORCEINLINE virtual bool CanInteract_Implementation(const class APawn* Pawn) const override { return false; }

	FORCEINLINE virtual void Interact_Implementation(class APawn* Pawn) override {}

	//End IInteractable


	//Begin IFogOfWarActor

	FORCEINLINE virtual void SetFogOfWarActorID_Implementation(int32 NewID) override { FogOfWarActorID = NewID; }

	FORCEINLINE virtual int32 GetFogOfWarActorID_Implementation() const override { return FogOfWarActorID; }

	virtual bool IsFogOfWarVisible_Implementation(const class UFogOfWarVisionComponent* Component) const override;

	virtual void SetFogOfWarVisionVisibility_Implementation(class UFogOfWarVisionComponent* Component, bool bIsVisible) override;

	virtual void SetFogOfWarDisplayVisibility_Implementation(class UFogOfWarDisplayComponent* Component, bool bIsVisible) override;

	//End IFogOfWarActor

	//Begin IGenericTeamAgentInterface

	FORCEINLINE virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	FORCEINLINE virtual FGenericTeamId GetGenericTeamId() const override;

	//End IGenericTeamAgentInterface



	//Tag that specifies the category of this unit when box selecting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Selection")
	FGameplayTag SelectionTypeTag;

	//The way this unit is selected when using box select.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Selection")
	EGameplaySelectionBoxRule SelectionBoxRule = EGameplaySelectionBoxRule::CenterPoint;

	//The highlight settings to use for displaying highlights on the unit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Selection")
	class UHighlightSettings* HighlightSettings;
	
	//Color parameter to set on the selection indicator material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Selection")
	FName SelectionIndicatorMaterialColorParameter;

	//Sets the highlighting on the relevant primitives of this actor
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Unit | Selection")
	void SetHighlighting(EHighlightType HighlightType);

	//The default ability to use when following a nav link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Navigation2")
	TSubclassOf<class AAbility> DefaultNavLinkAbilityClass;

	//Map specific nav link type tags to the ability that implements the follow behaviour.
	//e.g. "NavLink.Door" -> AOpenDoorAbility
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Navigation2")
	TMap<FGameplayTag, TSubclassOf<class AAbility>> NavLinkAbilityClasses;

	//Helper function to get the nav link ability class for the given follow params. 
	//The default implementation uses the NavLinkAbilityClasses map.
	virtual TSubclassOf<class AAbility> GetNavLinkAbilityClass(const FFollowNavLinkParams& Params) const;

	//The behavior tree to run when not following any commands
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Command")
	class UBehaviorTree* DefaultBehaviorTree;

	//Hide/Show visuals to the given player
	//@note: currently there is only ever one player so can just set whole actor hidden/visible
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Unit | FogOfWar")
	void SetFogOfWarVisibility(bool bIsVisible);

	//Start with visuals hidden from all players
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Unit | FogOfWar")
	void InitFogOfWarVisibility();

	FORCEINLINE auto GetFocusComponent() const { return FocusComponent; }

	//Ability to run when health stat reaches zero. This ability implements the concept of "Dying" for the unit. Ability must immediately grant Status.Dead tag to the unit.
	//E.G. Zombies can just immediately die and despawn after a while. Player units can be downed and have a time limit for being picked up before perma-death.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Lifetime")
	TSubclassOf<class AAbility> DeathAbility;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UFocusComponent* FocusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UGameplaySelectableComponent* SelectableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UCommandFollowerComponent* CommandFollowerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UStaticMeshComponent* SelectionIndicatorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UAbilityComponent* AbilityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UUnitStatsComponent* UnitStatsComponent;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	//class UCooldownComponent* CooldownComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UAIPerceptionStimuliSourceComponent* StimuliComponent;

	//If true, this unit will automatically register itself as belonging to the first player in the game world
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit | Misc")
	bool bAutoRegisterPlayerOwnership = false;

	UFUNCTION(BlueprintCallable, Category = "Unit | Command")
	void RunDefaultBehaviorTree();

	UFUNCTION()
	void DisplayTypeChangedHandler(UGameplaySelectableComponent* Selectable, UGameplaySelectorComponent* Selector, EGameplaySelectionDisplayType NewDisplayType);

	//Checks health in tick rather than having a stat change delegate.
	//This way, last moment health recovery effects have a chance to trigger before actually dying.
	void UpdateIsDead();

	//Drive movement component walking speed based on Stats.MoveSpeed
	void UpdateMoveSpeed();

	//Number of player vision components that can see this unit, used for visibility detection.
	int32 FogOfWarVisionRefCount = 0;

	//Spatial hash ID for fog of war actor queries.
	int32 FogOfWarActorID = -1;

};
