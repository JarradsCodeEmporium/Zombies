//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "Templates/SharedPointer.h" 
#include "UnitStatsCommon.generated.h"

/*
so different stat types are defined by gameplay tag e.g.
Stat.Health, Stat.MaxHealth, Stat.MoveSpeed, Stat.IncomingDamage, Stat.WeaponDamage, Stat.Strength etc.

a stat has a base value and a modified value. the base value just defines the initial value of the stat, such as inherent unit health without any buffs from level or gear
the modified value is the value after all the stat modifiers have been applied.

a stat modifier is a singleton object that implements changes to a set of unit stats



*/

class UUnitStatsModifier;
class UUnitStatsComponent;

constexpr auto UnitStatsSPMode = ESPMode::NotThreadSafe;

//Defines the lifetime and calculation method of a stat mod
UENUM(BlueprintType)
enum class EStatModLifetime : uint8
{
	//The modification is applied at one instant in time.
	//Changes to modified stat values will only last for the execution of the instant calculation.
	Instant,

	//The modification periodically applies instant modifications.
	Periodic,

	//The modification is applied at all times. Duration modifiers cannot modify base values.
	Duration,
};

//Stats have a permanent base value, and a temporary modified value.
UENUM(BlueprintType)
enum class EStatValueType : uint8
{
	Base,
	Modified,
};

//Selects whether to use current values, or previously captured values.
UENUM(BlueprintType)
enum class EStatValueCapture : uint8
{
	//The current value of the stat
	Current,

	//The value of the stat when it was captured some time in the past
	Captured,
};

//In a stat mod context, there are two participants.
UENUM(BlueprintType)
enum class EStatModUnit : uint8
{
	//The unit that is responsible for instigating the stat mod effect
	Source,
	//The unit that is receiving the stat mod effect.
	Target
};

//The value of an individual stat such as Health, MaxHealth, Strength, etc..
USTRUCT(BlueprintType)
struct FUnitStatValue
{
	GENERATED_BODY()
public:

	//The base value of the stat. Changes to this are permanent.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	float BaseValue = 0.f;

	//The modified value of this stat. Changes are temporary.
	UPROPERTY(Transient, BlueprintReadWrite, Category = Stats)
	float ModifiedValue = 0.f;

	FORCEINLINE bool operator==(const FUnitStatValue& Other) const { return BaseValue == Other.BaseValue && ModifiedValue == Other.ModifiedValue; }
	FORCEINLINE bool operator!=(const FUnitStatValue& Other) const { return !(*this == Other); }

	float GetValue(EStatValueType Type) const;

	void SetValue(EStatValueType Type, float Value);

	//Reset the modified stat value to its base value
	FORCEINLINE void ResetModifiers() { ModifiedValue = BaseValue; }
	
};

//Represents a collection of stat values.
USTRUCT(BlueprintType)
struct FUnitStats
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	TMap<FGameplayTag, FUnitStatValue> StatValues;

	float GetValue(FGameplayTag Tag, EStatValueType Type) const;

	void SetValue(FGameplayTag Tag, EStatValueType Type, float Value);

	void ResetModifiers();
};

//Represents an instance of a UUnitStatMod that is ready to be applied.
USTRUCT(BlueprintType)
struct FStatModParams
{
	GENERATED_BODY()
public:

	//The stat mod class that this is an instance of
	UPROPERTY(BlueprintReadWrite, Category = Stats)
	class UUnitStatsModifier* Modifier = nullptr;
	
	//The relevant stats from the source that have been captured at the time this instance was created
	UPROPERTY(BlueprintReadWrite, Category = Stats)
	FUnitStats SourceCapturedStats;

	//The stats component that is the source of this instance.
	UPROPERTY(BlueprintReadWrite, Category = Stats)
	class UUnitStatsComponent* SourceStatsComponent = nullptr;

	//The actor that represents the instigator in the world. 
	//For direct unit attacks, this will be the unit actor itself. For thrown objects like grenades, this will be the grenade actor that was thrown.
	UPROPERTY(BlueprintReadWrite, Category = Stats)
	class AActor* SourceActor = nullptr;

	//Extra generic data that can be used by stat mod calculations
	UPROPERTY(BlueprintReadWrite, Category = Stats)
	TMap<FGameplayTag, float> Magnitudes;

};


//Represents an instance of a UUnitStatMod that has been applied (and possibly expired)
//Is owned by shared ptr FStatModHandle and uses FGCObject to manage its UObject references.
struct FStatModInstance : public FGCObject, public TSharedFromThis<FStatModInstance, UnitStatsSPMode>
{
	//The stat mod class that this is an instance of
	class UUnitStatsModifier* Modifier = nullptr;

	//The relevant stats from the source that have been captured at the time this instance was created
	FUnitStats SourceCapturedStats;

	//The stats component that is the source of this instance.
	class UUnitStatsComponent* SourceStatsComponent = nullptr;

	//The actor that represents the instigator in the world. 
	//For direct unit attacks, this will be the unit actor itself. For thrown objects like grenades, this will be the grenade actor that was thrown.
	class AActor* SourceActor = nullptr;

	//Extra data for stat calculations set by caller
	TMap<FGameplayTag, float> Magnitudes;

	//The relevant stats from the target that have been captured at the time this instance was applied
	FUnitStats TargetCapturedStats;

	//The target component that this stat mod is currently applied to (or was applied to if expired)
	class UUnitStatsComponent* TargetStatsComponent = nullptr;

	//If this is a periodic or duration modifier, this is the timer to application and/or expiration events.
	FTimerHandle TimerHandle;

	//Time at the last "tick" of a persistent modifier.
	double GameTimeAtLastTick = -1.0;

	//If this is set, the modifier will expire when the game time reaches this value.
	//If it is not set, then the modifier never expires by duration timeout.
	TOptional<double> GameTimeAtExpiry;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FString GetReferencerName() const override;

};

//Handle to a stat mod instance that has been applied (and possibly now expired)
//The handle itself can be valid even after the modifier has expired, 
//but the actual components and actors referenced within it are not guaranteed to still exist past the frame that the modifier expired.
//E.G. A handle can still refer to a modifier on a unit that has already been killed and deleted, but the unit actor itself will not be valid.
USTRUCT(BlueprintType)
struct FStatModHandle
#if CPP
	: public TSharedPtr<FStatModInstance, UnitStatsSPMode>
#endif
{
	GENERATED_BODY()
public:

	using Super = TSharedPtr<FStatModInstance, UnitStatsSPMode>;

	using Super::Super;
};

//Handle to a callback that watches the modification of a stat on a component.
USTRUCT(BlueprintType)
struct FStatChangeObserverHandle
{
	GENERATED_BODY()
public:

	FORCEINLINE class UUnitStatsComponent* GetComponent() const { return Component; }

	FORCEINLINE int32 GetID() const { return ID; }

	FORCEINLINE FGameplayTag GetTag() const { return Tag; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = Stats, Meta = (AllowPrivateAccess = "True"))
	class UUnitStatsComponent* Component = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Stats, Meta = (AllowPrivateAccess = "True"))
	int32 ID = -1;

	UPROPERTY(BlueprintReadOnly, Category = Stats, Meta = (AllowPrivateAccess = "True"))
	FGameplayTag Tag;

	friend class UUnitStatsComponent;
};

FORCEINLINE uint32 GetTypeHash(const FStatChangeObserverHandle& Handle)
{
	//ID is unique per component, so we only need to hash these two members.
	return HashCombineFast(GetTypeHash(Handle.GetComponent()), GetTypeHash(Handle.GetID()));
}

//Data required for stat calculation inside a stat modifier.
//WARNING: Do not store this as a member, it should only exist as a parameter to CalculateModifier()
USTRUCT(BlueprintType)
struct FStatModCalcContext
{
	GENERATED_BODY()
public:

	//Routes stat read/writes to the specified data
	//We need this indirection in the case where we want to only check whether a stat change has certain results, without actually applying it.
	//e.g. when we want to check if the unit has enough mana to use an ability
	struct FUnit
	{
		class UUnitStatsComponent* Component = nullptr;

		FUnitStats* CurrentStats = nullptr;

		const FUnitStats* CapturedStats = nullptr;
		
		//If capture is specified but the captured stats do not contain that stat, then the current value is returned instead.
		float GetValue(FGameplayTag Tag, EStatValueCapture Capture, EStatValueType Type) const;

	};

	FUnit Source;

	FUnit Target;

	const TMap<FGameplayTag, float>* Magnitudes;

	//The duration in seconds that remains for this modifier
	//If not set, then duration is infinite
	TOptional<float> RemainingDuration;

	//The calculation phase of this stat modification.
	FGameplayTag CalculationPhase;

	//Cached IUnitInterface::GetUnitTags() from each components owner.
	mutable TMap<class UObject*, FGameplayTagContainer> UnitTags;

	//World that is executing this stat calculation
	UWorld* World = nullptr;

	//Is false for duration effects. 
	//Modifies SetTargetCurrentValue() behaviour so that the same calculation code can apply to instant/periodic/duration modifiers,
	//and also prevents accidentally setting base values for duration modifiers.
	bool bAllowBaseValueModification = true;

	//Gets the unit tags from the source/target
	//Stat modifiers must use these values rather than getting the tags themselves from the interface
	const FGameplayTagContainer& GetUnitTags(EStatModUnit Unit) const;

	float GetValue(FGameplayTag Tag, EStatModUnit Unit, EStatValueCapture Capture, EStatValueType Type) const;

	//Set the value of the targets current stats. This is the only unit and capture combo that can be written by modifiers during calculation.
	//Additionally, Type argument may be ignored and instead always equal to EStatValueType::Modified if bAllowBaseValueModification is false.
	//This is the case for duration effects which can never apply permanent changes.
	void SetTargetCurrentValue(FGameplayTag Tag, EStatValueType Type, float Value) const;

};

