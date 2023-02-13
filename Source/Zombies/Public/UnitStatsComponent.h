//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitStatsCommon.h"
#include "GameplayTagAssetInterface.h"
#include "UnitStatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FStatModChangeDelegate, class UUnitStatsComponent*, Component, const FStatModHandle&, Handle, const FUnitStats&, PreviousStats, const FUnitStats&, CurrentStats);
DECLARE_DYNAMIC_DELEGATE_FourParams(FStatChangeDelegate, class UUnitStatsComponent*, Component, FGameplayTag, Tag, FUnitStatValue, OldValue, FUnitStatValue, NewValue);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UUnitStatsComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUnitStatsComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	virtual void InitializeComponent() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& Container) const override;

	//Whether the component has the given stat.
	UFUNCTION(BlueprintCallable, Category = Stats)
	FORCEINLINE bool HasStat(FGameplayTag StatTag) const { return CurrentStats.StatValues.Contains(StatTag); }

	//Adds a stat to this component and gives it a default value.
	//Does not trigger any modifier recalculation, so this is mainly only for initialization.
	UFUNCTION(BlueprintCallable, Category = Stats)
	void InitializeStat(FGameplayTag StatTag, float DefaultValue);

	//Captures the stats from this component that match the tags in the container.
	//E.G. A container with {"Stats.Combat"} will capture the current values of Stats.Combat.AttackPower and Stats.Combat.RemainingAmmo
	UFUNCTION(BlueprintCallable, Category = Stats)
	FUnitStats MakeCapturedStats(const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	FStatModParams MakeStatModParamsByClass(TSubclassOf<class UUnitStatsModifier> ModifierClass);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	FStatModParams MakeStatModParams(class UUnitStatsModifier* Modifier);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static bool IsValidParams(const FStatModParams& Params);

	//Calculates the stat results of applying the given modifier without actually appling it.
	//For periodic modifiers, this is the result of applying a tick at this instant.
	//@param Params: The modifier params to calculate.
	//@param SkipCalculationPhases: A container of calculation phase tags that should be skipped. Generally only used to skip min clamping phase for resource checks.
	//@return: The stat values that this component would have if the instance was actually applied.
	UFUNCTION(BlueprintCallable, Category = Stats)
	FUnitStats CalculateModifier(const FStatModParams& Params, const FGameplayTagContainer& SkipCalculationPhases);

	//Adds the given modifier to the stat component
	//@param Params: The modifier to add and its captured stats. Created by calls like MyOtherComponent->MakeStatModParams(MyModifier).
	//@return: A handle to the applied stat modifier instance. For instant modifiers, this handle will already be expired. Can be invalid if Params are invalid.
	UFUNCTION(BlueprintCallable, Category = Stats)
	FStatModHandle AddModifier(const FStatModParams& Params);

	//Remove an existing modifier.
	//@return: Whether the modifier was valid and removed.
	UFUNCTION(BlueprintCallable, Category = Stats)
	static bool RemoveModifier(const FStatModHandle& Handle);

	//Whether this handle refers to a valid mod instance.
	//The mod instance may be expired.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static bool IsValidMod(const FStatModHandle& Handle);

	//Whether the handle refers to a valid and active mod
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static bool IsActiveMod(const FStatModHandle& Handle);

	//Sets the modifiers remaining duration back to its initial value
	UFUNCTION(BlueprintCallable, Category = Stats)
	static void RefreshRemainingDuration(const FStatModHandle& Handle);

	//Sets the modifiers remaining duration in seconds to the given value.
	//Zero or negative durations will cause it to expire next tick.
	UFUNCTION(BlueprintCallable, Category = Stats)
	static void SetRemainingDuration(const FStatModHandle& Handle, float RemainingDuration);

	//Sets the modifier to never expire.
	UFUNCTION(BlueprintCallable, Category = Stats)
	static void SetInfiniteDuration(const FStatModHandle& Handle);

	//Gets the remaining duration of a modifier in seconds.
	//Returns 0 for infinite durations. IsInfiniteDuration() should generally be checked first.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static float GetRemainingDuration(const FStatModHandle& Handle);

	//Whether the modifier currently has an infinite duration.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static bool IsInfiniteDuration(const FStatModHandle& Handle);

	//Gets the modifier object for this instance
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE class UUnitStatsModifier* GetModifier(const FStatModHandle& Handle)
	{
		return Handle.IsValid() ? Handle->Modifier : nullptr;
	}

	//Gets the component that is the source of the modifier
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE UUnitStatsComponent* GetSourceComponent(const FStatModHandle& Handle)
	{
		return Handle.IsValid() ? Handle->SourceStatsComponent : nullptr;
	}

	//Gets the actor that is the source of the modifier
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE class AActor* GetSourceActor(const FStatModHandle& Handle)
	{
		return Handle.IsValid() ? Handle->SourceActor : nullptr;
	}

	//Gets the component that is the target of the modifier
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE UUnitStatsComponent* GetTargetComponent(const FStatModHandle& Handle)
	{
		return Handle.IsValid() ? Handle->TargetStatsComponent : nullptr;
	}

	//Called after a duration/periodic modifier is added to this component
	UPROPERTY(BlueprintAssignable, Category = Stats)
	FStatModChangeDelegate OnTargetPersistentModifierAdded;

	//Called after a duration/periodic modifier is removed from this component
	UPROPERTY(BlueprintAssignable, Category = Stats)
	FStatModChangeDelegate OnTargetPersistentModifierRemoved;

	//Called after an instant modifier is applied or a periodic modifier tick occurs on this component
	UPROPERTY(BlueprintAssignable, Category = Stats)
	FStatModChangeDelegate OnTargetInstantaneousModifierApplied;

	//Called after this component adds a duration/periodic modifier to another component
	UPROPERTY(BlueprintAssignable, Category = Stats)
	FStatModChangeDelegate OnSourcePersistentModifierAdded;

	//Called after the end of a duration/periodic modifier that was added by this component
	UPROPERTY(BlueprintAssignable, Category = Stats)
	FStatModChangeDelegate OnSourcePersistentModifierRemoved;

	//Called after an instant or periodic tick modifier sent by this component has occurred
	UPROPERTY(BlueprintAssignable, Category = Stats)
	FStatModChangeDelegate OnSourceInstantaneousModifierApplied;

	//Watch for changes on a given stat. Is called after any change on the given stat.
	UFUNCTION(BlueprintCallable, Category = Stats)
	FStatChangeObserverHandle AddStatChangeObserver(FGameplayTag Tag, const FStatChangeDelegate& Delegate);

	//Remove the given stat change delegate.
	UFUNCTION(BlueprintCallable, Category = Stats)
	static bool RemoveStatChangeObserver(const FStatChangeObserverHandle& Handle);

	//Whether the handle refers to a currently active stat change observer
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static bool IsValidChangeHandle(const FStatChangeObserverHandle& Handle);

	FORCEINLINE const auto& GetCurrentStats() const { return CurrentStats; }

	//Applied stat modifiers that have a non-instant lifetime, such as duration and periodic modifiers
	FORCEINLINE const auto& GetPersistentModifiers() const { return PersistentModifiers; }

protected:

	//Stat calculations are basically a directed acyclic graph where vertices are stat values and edges are calculations
	//The problem with this is that it would be very easy to accidentally make a cyclic dependency given what arbitrary modifiers are applied at each instant.
	//This directed acyclic graph is also equivalent to a partial ordering of operations required in order to compute the final stats.
	//So we statically define an ordered partition of this partial ordering, which constrains the possible stat calculation graphs to a particular valid subset.
	//The CalculationPhases array defines this ordered partition, giving a gameplay tag name to each unordered set in the list.
	//Modifier calculations within the same phase are unordered with regard to each other.
	//This typically means that any modifier operations in the same phase should use exclusively addition, or exclusively multiplication,
	//since these operations are effectively order independent due to their commutative nature.

	//The inherent stat values of this component.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Stats, Meta = (AllowPrivateAccess = "True"))
	FUnitStats CurrentStats;

	//Ordered list of calculation phases. A UUnitStatsModifier can apply its changes in one or more calculation phases.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Stats, Meta = (AllowPrivateAccess = "True"))
	TArray<FGameplayTag> CalculationPhases;

	//Applied stat modifiers that have a non-instant lifetime, such as duration and periodic modifiers
	UPROPERTY(BlueprintReadOnly, Category = Stats, Meta = (AllowPrivateAccess = "True"))
	TSet<FStatModHandle> PersistentModifiers;

	//Fills in basic state into a new FStatModInstance, and returns its handle.
	FStatModHandle MakeStatModInstance(const FStatModParams& Params);

	//Specifies the context in which the modifiers are being applied
	enum EApplyModifierFlags : uint32
	{
		AM_None = 0,
		AM_PersistentAdd = 1 << 0,
		AM_PersistentRemove = 1 << 1,
		AM_Instantaneous = 1 << 2,
	};

	//Applies all modifiers, with a specific instance being the instigator of the change
	void ApplyModifiers(const FStatModHandle& Handle, EApplyModifierFlags Flags);

	//Calculate the result of all duration modifiers on the given stats, and optionally include an instant or periodic modifier.
	void CalculateStats(FUnitStats& Stats, const FStatModInstance* InstantModifier = nullptr, const FGameplayTagContainer& SkipCalculationPhases = FGameplayTagContainer{});

	//Fills in the final context info and executes a single modifier.
	void DispatchCalculateModifier(FStatModCalcContext& Context, const FStatModInstance& Instance) const;

	bool bIsCalculatingStats = false;

	//Sets the timer for a periodic/duration modifier based on time at last tick and time at expiry
	void DispatchTickPersistentModifier(const FStatModHandle &Handle);

	//Executes a periodic tick. Since the actual UWorld::GetTimeSeconds() is quantized at frame rate intervals, 
	//we pass the expected game time at this tick in order to calculate what to do in the next tick. Only approximate scheduling is done with the GetTimeSeconds() value.
	void TickPersistentModifier(FStatModHandle Handle, double GameTimeAtTick, bool bApplyPeriodicModifier, bool bExpireModifier);

	//Maps stat tag to the observers watching for changes in that stat
	TMap<FGameplayTag, TMap<int32, FStatChangeDelegate>> StatChangeObservers;

	//Captures the subset of stats that are watched by a change observer
	FUnitStats CaptureObservedStats(const FUnitStats& Stats);

	//Compares OldStats to NewStats and triggers the relevant stat observer delegates
	//@param InstantModifier: The instant/periodic modifier that triggered this stat change (if any)
	void BroadcastChangedStats(const FUnitStats& OldStats, const FUnitStats& NewStats);

	int32 NextStatChangeObserverHandleID = 0;

	FStatChangeObserverHandle NewStatChangeObserverHandle(FGameplayTag Tag);

};
