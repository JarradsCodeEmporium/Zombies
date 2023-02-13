//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Target.h"
#include "FocusComponent.generated.h"



USTRUCT(BlueprintType)
struct ZOMBIES_API FFocusLevel
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	FGameplayTag Tag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	float InterpSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	bool bConstantSpeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	FTarget Target;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UFocusComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFocusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	//BP helper to make a focus target that looks at a fixed world location
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Focus)
	static FORCEINLINE FTarget MakeFocusTargetWorldLocation(const FVector& InWorldLocation) { return FTarget{ InWorldLocation }; }

	//BP helper to make a focus target that looks at a relative location on a component
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Focus)
	static FORCEINLINE FTarget MakeFocusTargetRelativeLocation(USceneComponent* InComponent, const FVector& InRelativeLocation) { return FTarget{ InComponent, InRelativeLocation }; }

	//BP helper to make a focus target that looks at a relative location on an actors root component
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Focus)
	static FORCEINLINE FTarget MakeFocusTargetActorRelativeLocation(AActor* InActor, const FVector& InRelativeLocation) { return FTarget{ InActor, InRelativeLocation }; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Focus)
	static FORCEINLINE FVector GetFocusTargetLocation(const FTarget& FocusTarget) { return FocusTarget.GetLocation(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Focus)
	static FORCEINLINE USceneComponent* GetFocusTargetComponent(const FTarget& FocusTarget) { return FocusTarget.GetComponent(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Focus)
	static FORCEINLINE AActor* GetFocusTargetActor(const FTarget& FocusTarget) { return FocusTarget.GetActor(); }

	//Does the component currently have any focus target?
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE bool HasFocus() const { return GetActiveFocusLevel() != nullptr; }

	//Whether the component is currently facing its active focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool IsFacingFocus(double AngleErrorDegrees = 5.0) const;

	//Whether the component is facing its active focus target on the horizontal plane. Z is ignored.
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool IsFacingFocus2D(double AngleErrorDegrees = 5.0) const;

	//Is a specific focus level the active focus target right now?
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE bool IsFocusLevelActive(FGameplayTag Tag) const { return GetActiveFocusLevel() == GetFocusLevel(Tag); }

	//Gets the tag of the focus level that is currently being focused
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE FGameplayTag GetActiveFocusLevelTag() const 
	{ 
		if (auto Level = GetActiveFocusLevel()) 
			return Level->Tag;

		return {};
	}

	UFUNCTION(BlueprintCallable, Category = Focus)
	FTarget GetFocusAtLevel(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE FVector GetFocusLocationAtLevel(FGameplayTag Tag) const { return GetFocusAtLevel(Tag).GetLocation(); }

	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE USceneComponent* GetFocusComponentAtLevel(FGameplayTag Tag) const { return GetFocusAtLevel(Tag).GetComponent(); }

	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE AActor* GetFocusActorAtLevel(FGameplayTag Tag) const { return GetFocusAtLevel(Tag).GetActor(); }

	//Gets the current active focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	FTarget GetFocus() const;

	//Gets the location from the current active focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE FVector GetFocusLocation() const { return GetFocus().GetLocation(); }

	//Gets the component from the current active focus target if it is a relative target
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE USceneComponent* GetFocusComponent() const { return GetFocus().GetComponent(); }

	//Gets the owning actor from the current active focus target if it is a relative target
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE AActor* GetFocusActor() const { return GetFocus().GetActor(); }

	//Gets the type of the active focus
	UFUNCTION(BlueprintCallable, Category = Focus)
	FORCEINLINE ETargetType GetFocusType() const { return GetFocus().Type; }

	//Set a focus level to use a focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool SetFocus(FGameplayTag Tag, const FTarget& InFocusTarget);

	//Set a focus level to use a world location as its focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool SetFocusWorldLocation(FGameplayTag Tag, const FVector& InWorldLocation);

	//Set a focus level to use a component relative location as its focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool SetFocusRelativeLocation(FGameplayTag Tag, USceneComponent* InComponent, const FVector& InRelativeLocation);

	//Set a focus level to use an actors root component relative location as its focus target
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool SetFocusActorRelativeLocation(FGameplayTag Tag, AActor* InActor, const FVector& InRelativeLocation);

	//Remove the target from a focus level
	UFUNCTION(BlueprintCallable, Category = Focus)
	bool ClearFocus(FGameplayTag Tag);

	//Removes all focus targets from all levels.
	//Note that if bDrivePathfindingFocus, the pathfinding focus level will still be automatically set in the next tick.
	UFUNCTION(BlueprintCallable, Category = Focus)
	void ClearAllFocus();

	//Order list of each focus level that this focus component supports
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	TArray<FFocusLevel> FocusLevels;

	//If true, will look up the ownership heirarchy to find an AI controller and set its gameplay focus to match this component
	//Note that AIController has similar functionality, but it is very old (like UE2 at least), inflexible, and not exposed to blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	bool bDriveOwnersFocus = false;

	//If true, automatically sets the focus level specified in PathfindingFocusTag to the AI controllers currently followed path.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	bool bDrivePathfindingFocus = true;

	//This tag level will be automatically driven by owners path following component. Requires bDrivePathfindingFocus to do anything.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	FGameplayTag PathfindingFocusTag;

protected:

	void InterpolateToFocus(float DeltaTime);

	const FFocusLevel* GetActiveFocusLevel() const;

	const FFocusLevel* GetFocusLevel(FGameplayTag Tag) const;

	FFocusLevel* GetActiveFocusLevel();

	FFocusLevel* GetFocusLevel(FGameplayTag Tag);
};
