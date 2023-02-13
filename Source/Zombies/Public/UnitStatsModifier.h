//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitStatsCommon.h"
#include "UnitStatsModifier.generated.h"

/**
 * Base class for unit stat modifiers.
 */
UCLASS(Blueprintable)
class ZOMBIES_API UUnitStatsModifier : public UDataAsset
{
	GENERATED_BODY()
public:

	//The type of this stat modifier. Controls the lifetime and calculation method for this modifier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	EStatModLifetime Lifetime;

	//Duration of the modifier. Only valid when not infinite duration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats, Meta = (ClampMin = "0", EditCondition = "Lifetime != EStatModLifetime::Instant && !bInfiniteDuration"))
	float Duration = 0.f;

	//Whether this modifier ever expires
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats, Meta = (ClampMin = "0", EditCondition = "Lifetime != EStatModLifetime::Instant"))
	bool bInfiniteDuration = 0.f;

	//How often the periodic modifier is applied. Use 0 to apply every frame.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats, Meta = (ClampMin = "0", EditCondition = "Lifetime == EStatModLifetime::Periodic"))
	float Period = 1.f;

	//For periodic effects.
	//If true, the modifier will tick immediately upon application.
	//If false, the modifier will wait for the first period to elapse before applying.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats, Meta = (EditCondition = "Lifetime == EStatModLifetime::Periodic"))
	bool bPeriodicTickOnApplication = false;

	//Tags applied to the stats component when this modifier is active.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	FGameplayTagContainer GrantedTags;

	//Stats which should be captured from the source when an instance of this modifier is first created.
	//E.G. A unit throws a grenade. Their explosive attack power stat is captured at the moment it is thrown.
	//When the grenade later explodes, that previously stored attack power is used rather than the current value.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	FGameplayTagContainer SourceCaptureStats;

	//Same as SourceCaptureStats, but captures the target stats at the moment the modifier is actually applied to it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	FGameplayTagContainer TargetCaptureStats;

	//The calculation phases of the stats component in which this modifier should be applied.
	//Typically only one phase is necessary.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	FGameplayTagContainer CalculationPhases;

	//The tags that this modifier itself has.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	FGameplayTagContainer ModifierTags;

	//Applies the stat changes of this modifier according to the context.
	UFUNCTION(BlueprintNativeEvent, Category = Stats)
	void CalculateModifier(const FStatModCalcContext& Context) const;

	//Gets the calculation phase of this execution of ApplyModifier()
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE FGameplayTag GetCalculationPhase(const FStatModCalcContext& Context) { return Context.CalculationPhase; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE UWorld* GetWorldContext(const FStatModCalcContext& Context) { return Context.World; }

	//Get the stats component of the source or target unit
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE class UUnitStatsComponent* GetUnit(const FStatModCalcContext& Context, EStatModUnit Unit) 
	{
		return Unit == EStatModUnit::Source ? Context.Source.Component : Context.Target.Component;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE FGameplayTagContainer GetUnitTags(const FStatModCalcContext& Context, EStatModUnit Unit)
	{
		return Context.GetUnitTags(Unit);
	}

	//Uses the context to retrieve the value of a stat, such as the sources captured base health, or the targets current modified armor
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE float GetStatValue(const FStatModCalcContext& Context, FGameplayTag Tag, EStatModUnit Unit, EStatValueCapture Capture, EStatValueType Type)
	{
		return Context.GetValue(Tag, Unit, Capture, Type);
	}
	
	//Sets the value of the targets current base or modified stat value. This is the only type of stat that can be written
	//Additionally, Duration modifiers can only write
	UFUNCTION(BlueprintCallable, Category = Stats)
	static FORCEINLINE void SetTargetCurrentStatValue(const FStatModCalcContext& Context, FGameplayTag Tag, EStatValueType Type, float Value)
	{
		Context.SetTargetCurrentValue(Tag, Type, Value);
	}

	//Gets the duration remaining on the modifier
	//If duration is set to infinite, returns 0
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE float GetRemainingDuration(const FStatModCalcContext& Context)
	{
		return Context.RemainingDuration ? *Context.RemainingDuration : 0.f;
	}

	//Whether the modifier has an infinite duration
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE bool IsInfiniteDuration(const FStatModCalcContext& Context)
	{
		return !Context.RemainingDuration;
	}

	//Get the magnitude for the given tag.
	//If tag isn't present, returns 0
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE float GetMagnitude(const FStatModCalcContext& Context, FGameplayTag Tag)
	{
		return Context.Magnitudes->FindRef(Tag);
	}

	//Whether the magnitudes map has the given tag.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Stats)
	static FORCEINLINE bool HasMagnitude(const FStatModCalcContext& Context, FGameplayTag Tag)
	{
		return Context.Magnitudes->Contains(Tag);
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	FGameplayTag TestStatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	float TestAdd = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	float TestMul = 0.f;
};
