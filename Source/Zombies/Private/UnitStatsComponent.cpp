//Copyright Jarrad Alexander 2022


#include "UnitStatsComponent.h"
#include "UnitStatsModifier.h"
#include "GameplayTagAssetInterface.h"

//PRAGMA_DISABLE_OPTIMIZATION

// Sets default values for this component's properties
UUnitStatsComponent::UUnitStatsComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	bWantsInitializeComponent = true;
}


// Called when the game starts
void UUnitStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	//Stats may have modifiers applied before begin play.
	CalculateStats(CurrentStats);
}


void UUnitStatsComponent::InitializeComponent()
{
	//Match modified stats to base stats set in editor/constructor
	//Acts as if we updated stats with no modifiers and no broadcasts.
	CurrentStats.ResetModifiers();
}

// Called every frame
void UUnitStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);



	// ...
}

void UUnitStatsComponent::GetOwnedGameplayTags(FGameplayTagContainer& Container) const
{
	for (auto& Handle : PersistentModifiers)
		if (Handle && Handle->Modifier)
			Container.AppendTags(Handle->Modifier->GrantedTags);
}


void UUnitStatsComponent::InitializeStat(FGameplayTag StatTag, float DefaultValue)
{
	auto& Stat = CurrentStats.StatValues.FindOrAdd(StatTag);

	Stat.BaseValue = DefaultValue;
	Stat.ModifiedValue = DefaultValue;
}

FUnitStats UUnitStatsComponent::MakeCapturedStats(const FGameplayTagContainer& Tags)
{
	FUnitStats Result;

	if (Tags.IsEmpty())
		return Result;

	Result.StatValues.Reserve(Tags.Num());

	for (auto& [Tag, StatValue] : CurrentStats.StatValues)
		if (Tag.MatchesAny(Tags))
			Result.StatValues.Add(Tag, StatValue);

	return Result;
}

FStatModParams UUnitStatsComponent::MakeStatModParamsByClass(TSubclassOf<class UUnitStatsModifier> ModifierClass)
{
	return MakeStatModParams(ModifierClass.GetDefaultObject());
}

FStatModParams UUnitStatsComponent::MakeStatModParams(UUnitStatsModifier* Modifier)
{
	if (!IsValid(Modifier))
		return FStatModParams();

	FStatModParams Params;

	Params.Modifier = Modifier;

	Params.SourceStatsComponent = this;

	Params.SourceCapturedStats = MakeCapturedStats(Modifier->SourceCaptureStats);

	return Params;
}

bool UUnitStatsComponent::IsValidParams(const FStatModParams& Params)
{
	if (!IsValid(Params.Modifier))
		return false;

	if (!IsValid(Params.SourceStatsComponent))
		return false;

	return true;
}

FUnitStats UUnitStatsComponent::CalculateModifier(const FStatModParams& Params, const FGameplayTagContainer& SkipCalculationPhases)
{
	auto Handle = MakeStatModInstance(Params);

	FUnitStats Result = CurrentStats;

	CalculateStats(Result, Handle.Get(), SkipCalculationPhases);

	return Result;
}

FStatModHandle UUnitStatsComponent::AddModifier(const FStatModParams& Params)
{
	//SCOPE_LOG_TIME_FUNC();

	check(!bIsCalculatingStats);

	if (!IsValidParams(Params))
		return {};

	switch (Params.Modifier->Lifetime)
	{
	case EStatModLifetime::Duration:
	case EStatModLifetime::Periodic:
	{
		auto Handle = MakeStatModInstance(Params);

		check(IsValidMod(Handle));

		PersistentModifiers.Add(Handle);

		if (!Handle->Modifier->bInfiniteDuration)
			Handle->GameTimeAtExpiry = Handle->GameTimeAtLastTick + Handle->Modifier->Duration;

		DispatchTickPersistentModifier(Handle);

		uint32 Flags = AM_PersistentAdd;

		if (Handle->Modifier->Lifetime == EStatModLifetime::Periodic && Handle->Modifier->bPeriodicTickOnApplication)
			//Also want to do periodic tick in addition to persistent add event
			Flags |= AM_Instantaneous;

		ApplyModifiers(Handle, (EApplyModifierFlags)Flags);
		 
		return Handle;
	}
	case EStatModLifetime::Instant:
	{
		auto Handle = MakeStatModInstance(Params);

		check(IsValidMod(Handle));

		ApplyModifiers(Handle, AM_Instantaneous);

		//Instant modifiers do not return valid handles because they are "expired" by the time the call returns anyway.
		return {};
	}
	default:
		checkNoEntry();
		return {};
	}
}

bool UUnitStatsComponent::RemoveModifier(const FStatModHandle& Handle)
{
	if (!Handle || !IsValid(Handle->TargetStatsComponent))
		return false;

	auto Component = Handle->TargetStatsComponent;

	check(!Component->bIsCalculatingStats);

	if (Component->PersistentModifiers.Remove(Handle) != 1)
		//Modifier is already expired
		return false;

	if (auto World = Component->GetWorld())
		World->GetTimerManager().ClearTimer(Handle->TimerHandle);

	Component->ApplyModifiers(Handle, AM_PersistentRemove);

	return true;
}

bool UUnitStatsComponent::IsValidMod(const FStatModHandle& Handle)
{
	if (!Handle.IsValid())
		return false;

	return true;
}

bool UUnitStatsComponent::IsActiveMod(const FStatModHandle& Handle)
{
	if (!IsValidMod(Handle))
		return false;

	if (!IsValid(Handle->TargetStatsComponent))
		return false;

	//Only persistent modifiers can be observed to be active
	return Handle->TargetStatsComponent->PersistentModifiers.Contains(Handle);
}

void UUnitStatsComponent::RefreshRemainingDuration(const FStatModHandle& Handle)
{
	if (!IsActiveMod(Handle))
		return;

	if (Handle->Modifier->bInfiniteDuration)
		Handle->GameTimeAtExpiry.Reset();
	else
		Handle->GameTimeAtExpiry = Handle->TargetStatsComponent->GetWorld()->GetTimeSeconds() + Handle->Modifier->Duration;

	Handle->TargetStatsComponent->DispatchTickPersistentModifier(Handle);
}

void UUnitStatsComponent::SetRemainingDuration(const FStatModHandle& Handle, float RemainingDuration)
{
	if (!IsActiveMod(Handle))
		return;

	Handle->GameTimeAtExpiry = Handle->TargetStatsComponent->GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, RemainingDuration);

	Handle->TargetStatsComponent->DispatchTickPersistentModifier(Handle);
}

void UUnitStatsComponent::SetInfiniteDuration(const FStatModHandle& Handle)
{
	if (!IsActiveMod(Handle))
		return;

	Handle->GameTimeAtExpiry.Reset();

	Handle->TargetStatsComponent->DispatchTickPersistentModifier(Handle);
}

float UUnitStatsComponent::GetRemainingDuration(const FStatModHandle& Handle)
{
	if (!IsActiveMod(Handle))
		return 0.0f;

	if (!Handle->GameTimeAtExpiry)
		return 0.f;

	return *Handle->GameTimeAtExpiry - Handle->TargetStatsComponent->GetWorld()->GetTimeSeconds();
}

bool UUnitStatsComponent::IsInfiniteDuration(const FStatModHandle& Handle)
{
	if (!IsActiveMod(Handle))
		return 0.0f;

	return !Handle->GameTimeAtExpiry;
}

FStatChangeObserverHandle UUnitStatsComponent::AddStatChangeObserver(FGameplayTag Tag, const FStatChangeDelegate& Delegate)
{
	auto& DelegateList = StatChangeObservers.FindOrAdd(Tag);

	auto Handle = NewStatChangeObserverHandle(Tag);

	DelegateList.Add(Handle.GetID(), Delegate);

	return Handle;
}

bool UUnitStatsComponent::RemoveStatChangeObserver(const FStatChangeObserverHandle& Handle)
{
	if (!IsValidChangeHandle(Handle))
		return false;

	//If handle is valid, this is guaranteed to work.
	auto& Delegates = Handle.GetComponent()->StatChangeObservers[Handle.GetTag()];

	Delegates.Remove(Handle.GetID());

	if (Delegates.Num() == 0)
		Handle.GetComponent()->StatChangeObservers.Remove(Handle.GetTag());

	return true;
}

bool UUnitStatsComponent::IsValidChangeHandle(const FStatChangeObserverHandle& Handle)
{
	if (!IsValid(Handle.GetComponent()) || Handle.GetID() < 0 || !Handle.GetTag().IsValid())
		return false;

	auto Delegates = Handle.GetComponent()->StatChangeObservers.Find(Handle.GetTag());
	
	if (!Delegates)
		return false;

	return Delegates->Contains(Handle.GetID());
}


FStatModHandle UUnitStatsComponent::MakeStatModInstance(const FStatModParams& Params)
{
	check(IsValidParams(Params));

	FStatModHandle Result = MakeShared<FStatModInstance, UnitStatsSPMode>();
	
	Result->Modifier = Params.Modifier;

	Result->SourceActor = Params.SourceActor ? Params.SourceActor : Params.SourceStatsComponent->GetOwner();

	Result->SourceStatsComponent = Params.SourceStatsComponent;

	Result->SourceCapturedStats = Params.SourceCapturedStats;

	Result->TargetStatsComponent = this;

	Result->Magnitudes = Params.Magnitudes;

	//Instant modifiers do not need to capture stats from target, because they are always exactly the same as our current stats.
	if (Result->Modifier->Lifetime != EStatModLifetime::Instant)
		Result->TargetCapturedStats = Result->TargetStatsComponent->MakeCapturedStats(Result->Modifier->TargetCaptureStats);

	Result->GameTimeAtLastTick = GetWorld()->GetTimeSeconds();

	return Result;
}

void UUnitStatsComponent::ApplyModifiers(const FStatModHandle& Handle, EApplyModifierFlags Flags)
{
	check(Handle);

	//Capture stats before applying so we can tell what changed
	FUnitStats PreviousStats = CurrentStats;

	//Instant or periodic tick
	if (Flags & AM_Instantaneous)
		CalculateStats(CurrentStats, Handle.Get());

	//Refresh current stats
	CalculateStats(CurrentStats);

	if (Flags & AM_PersistentAdd)
	{
		{
			auto Temp = OnTargetPersistentModifierAdded;

			Temp.Broadcast(this, Handle, PreviousStats, CurrentStats);
		}

		if (Handle->SourceStatsComponent)
		{
			auto Temp = Handle->SourceStatsComponent->OnSourcePersistentModifierAdded;

			Temp.Broadcast(this, Handle, PreviousStats, CurrentStats);
		}
	}

	if (Flags & AM_PersistentRemove)
	{
		{
			auto Temp = OnTargetPersistentModifierRemoved;

			Temp.Broadcast(this, Handle, PreviousStats, CurrentStats);
		}

		if (Handle->SourceStatsComponent)
		{
			auto Temp = Handle->SourceStatsComponent->OnSourcePersistentModifierRemoved;

			Temp.Broadcast(this, Handle, PreviousStats, CurrentStats);
		}
	}

	if (Flags & AM_Instantaneous)
	{
		{
			auto Temp = OnTargetInstantaneousModifierApplied;

			Temp.Broadcast(this, Handle, PreviousStats, CurrentStats);
		}

		if (Handle->SourceStatsComponent)
		{
			auto Temp = Handle->SourceStatsComponent->OnSourceInstantaneousModifierApplied;

			Temp.Broadcast(this, Handle, PreviousStats, CurrentStats);
		}
	}

	//Always compare and dispatch stat change events
	BroadcastChangedStats(PreviousStats, CurrentStats);
}

void UUnitStatsComponent::CalculateStats(FUnitStats& Stats, const FStatModInstance* InstantModifier, const FGameplayTagContainer& SkipCalculationPhases)
{
	if (bIsCalculatingStats)
		{
			UE_LOG(LogTemp, Error, TEXT("Recursive stat calculation on %s"), *GetPathName());
	
			return;
		}
	
	TGuardValue Guard{ bIsCalculatingStats, true };
	
	Stats.ResetModifiers();
		
	FStatModCalcContext Context;
	
	Context.Target.Component = this;
	
	Context.Target.CurrentStats = &Stats;
		
	auto TargetUnit = GetOwner();
	
	FGameplayTagContainer TargetTags;
	
	if (auto Interface = Cast<IGameplayTagAssetInterface>(TargetUnit))
		Interface->GetOwnedGameplayTags(TargetTags);
	
	if (InstantModifier)
		TargetTags.AppendTags(InstantModifier->Modifier->GrantedTags);
	
	Context.UnitTags.Add(TargetUnit, TargetTags);
	
	//@todo: read the relevant phases from each modifier and only iterate by that instead, 
	//because this is exhaustively checking every modifier in every phase, which is not necessary.
	for (auto CalculationPhase : CalculationPhases)
	{
		if (CalculationPhase.MatchesAny(SkipCalculationPhases))
			continue;
	
		Context.CalculationPhase = CalculationPhase;
	
		for (const auto& Handle : PersistentModifiers)
			if (Handle->Modifier->Lifetime == EStatModLifetime::Duration)
				//Continuous modifiers contribute at all times
				DispatchCalculateModifier(Context, *Handle);
	
		if (InstantModifier)
			DispatchCalculateModifier(Context, *InstantModifier);
	}
}

void UUnitStatsComponent::DispatchCalculateModifier(FStatModCalcContext& Context, const FStatModInstance& Instance) const
{
	if (!IsValid(Instance.Modifier))
		return;

	Context.Target.CapturedStats = &Instance.TargetCapturedStats;

	Context.Source.Component = Instance.SourceStatsComponent;

	Context.Source.CapturedStats = &Instance.SourceCapturedStats;

	if (!IsValid(Context.Source.Component) || !IsValid(Context.Target.Component))
		return;

	//@todo: when the calculation phase code is updated to only visit relevant modifiers, this check could be removed
	if (!Context.CalculationPhase.MatchesAny(Instance.Modifier->CalculationPhases))
		return;

	//If self applied, we want to use the stats and tags that were supplied in the sources context,
	//which may be different to our current state if we are only temporarily checking the results of a stat mod without applying, or instantly applying a modifier.
	//Sources other than ourselves can just always use CurrentStats because those are the continuous stats calcluated outside of this modifier application.

	if (Context.Source.Component == Context.Target.Component)
		Context.Source.CurrentStats = Context.Target.CurrentStats;
	else
		Context.Source.CurrentStats = &Context.Source.Component->CurrentStats;

	Context.bAllowBaseValueModification = Instance.Modifier->Lifetime != EStatModLifetime::Duration;

	if (Instance.GameTimeAtExpiry)
		Context.RemainingDuration = *Instance.GameTimeAtExpiry - Instance.GameTimeAtLastTick;

	Context.Magnitudes = &Instance.Magnitudes;

	Instance.Modifier->CalculateModifier(Context);
}

void UUnitStatsComponent::DispatchTickPersistentModifier(const FStatModHandle& Handle)
{
	check(Handle.IsValid() && Handle->TargetStatsComponent == this && PersistentModifiers.Contains(Handle));

	double GameTimeAtTick = -1.0;

	bool bExpireModifier = false;

	bool bApplyPeriodicModifier = false;

	//float TimeDelay = -1.f;

	switch (Handle->Modifier->Lifetime)
	{
	case EStatModLifetime::Duration:
	{
		if (Handle->GameTimeAtExpiry)
		{
			bExpireModifier = true;

			GameTimeAtTick = *Handle->GameTimeAtExpiry;
		}

		break;
	}
	case EStatModLifetime::Periodic:
	{
		double GameTimeAtNextPeriodicTick = Handle->GameTimeAtLastTick + Handle->Modifier->Period;

		if (Handle->GameTimeAtExpiry)
		{
			//Allow apply and expire to happen in the same tick if they happen close enough in time
			//This is because floating point inaccuracy could cause the last periodic tick to be unintentionally cut off.
			bExpireModifier = GameTimeAtNextPeriodicTick >= *Handle->GameTimeAtExpiry - 0.001;

			bApplyPeriodicModifier = GameTimeAtNextPeriodicTick <= *Handle->GameTimeAtExpiry + 0.001;

			GameTimeAtTick = FMath::Min(*Handle->GameTimeAtExpiry, GameTimeAtNextPeriodicTick);
		}
		else
		{
			bExpireModifier = false;

			bApplyPeriodicModifier = true;

			GameTimeAtTick = GameTimeAtNextPeriodicTick;
		}

		break;
	}
	case EStatModLifetime::Instant:
	default:
		break;
	}
	
	auto& TimerManager = GetWorld()->GetTimerManager();

	if (bApplyPeriodicModifier || bExpireModifier)
	{
		FTimerDelegate Delegate;

		Delegate.BindUObject(this, &UUnitStatsComponent::TickPersistentModifier, Handle, GameTimeAtTick, bApplyPeriodicModifier, bExpireModifier);

		float TimeDelay = GameTimeAtTick - GetWorld()->GetTimeSeconds();

		//Timer manager does nothing if the delay is <= 0.f, so in that case we will queue for the next frame instead.
		//We don't immediately execute the tick here because that would cause calling code to have to deal with callback events modifying our state
		if (TimeDelay > 0.f)
			TimerManager.SetTimer(Handle->TimerHandle, Delegate, TimeDelay, false);
		else
		{
			TimerManager.ClearTimer(Handle->TimerHandle);

			Handle->TimerHandle = TimerManager.SetTimerForNextTick(Delegate);
		}
	}
	else
		//Is infinite duration or instant modifier, so there is nothing to tick.
		TimerManager.ClearTimer(Handle->TimerHandle);

}

void UUnitStatsComponent::TickPersistentModifier(FStatModHandle Handle, double GameTimeAtTick, bool bApplyPeriodicModifier, bool bExpireModifier)
{
	check(Handle.IsValid() && Handle->TargetStatsComponent == this && PersistentModifiers.Contains(Handle));

	if (bApplyPeriodicModifier) 
	{
		Handle->GameTimeAtLastTick = GameTimeAtTick;

		if (!bExpireModifier)
			DispatchTickPersistentModifier(Handle);

		ApplyModifiers(Handle, AM_Instantaneous);
	}

	//@todo: callbacks in periodic tick may want to extend the lifetime of the modifier, so maybe check if we're still expiring here?
	if (bExpireModifier)
		RemoveModifier(Handle);
}

FUnitStats UUnitStatsComponent::CaptureObservedStats(const FUnitStats& Stats)
{
	FUnitStats Result;

	Result.StatValues.Reserve(StatChangeObservers.Num());

	for (auto& [Tag, Delegates] : StatChangeObservers)
	{
		auto CurrentStatValue = Stats.StatValues.Find(Tag);

		if (!CurrentStatValue)
			//Existing stats do not contain this value
			continue;

		Result.StatValues.Add(Tag, *CurrentStatValue);
	}

	return Result;
}

void UUnitStatsComponent::BroadcastChangedStats(const FUnitStats& OldStats, const FUnitStats& NewStats)
{
	//Gather all required delegate broadcasts before executing any of them, 
	//so that the delegate itself can't accidentally stomp on our data while we're iterating it.
	TArray<TTuple<FGameplayTag, FUnitStatValue, FUnitStatValue, TArray<FStatChangeDelegate>>> Broadcasts;

	//Non-existing stats implicitly have the default value.
	FUnitStatValue DefaultValue;

	for (auto& [Tag, Delegates] : StatChangeObservers)
	{
		auto OldValue = OldStats.StatValues.Find(Tag);
		auto NewValue = NewStats.StatValues.Find(Tag);

		if (!OldValue)
			OldValue = &DefaultValue;

		if (!NewValue)
			NewValue = &DefaultValue;
		
		if (*NewValue == *OldValue)
			continue;

		TArray<FStatChangeDelegate> DelegateCopies;

		Delegates.GenerateValueArray(DelegateCopies);

		Broadcasts.Add(MakeTuple(Tag, *OldValue, *NewValue, MoveTemp(DelegateCopies)));
	}

	for (auto& Broadcast : Broadcasts)
		for (auto& Delegate : Broadcast.Get<3>())
			Delegate.ExecuteIfBound(this, Broadcast.Get<0>(), Broadcast.Get<1>(), Broadcast.Get<2>());
}

FStatChangeObserverHandle UUnitStatsComponent::NewStatChangeObserverHandle(FGameplayTag Tag)
{
	FStatChangeObserverHandle Handle;

	Handle.ID = NextStatChangeObserverHandleID++;

	Handle.Component = this;

	Handle.Tag = Tag;

	return Handle;
}


//
//FUnitStats UUnitStatsComponent::CalculateModifier(const FStatModParams& Instance, const FGameplayTagContainer& SkipCalculationPhases)
//{
//	auto ActiveModifier = MakeActiveUnitStatMod(Instance);
//
//	FUnitStats Result = CurrentStats;
//
//	CalculateStats(Result, &ActiveModifier, SkipCalculationPhases);
//
//	return Result;
//}
//
//FUnitStatModHandle UUnitStatsComponent::AddModifier(const FStatModParams& Instance)
//{
//	if (!IsValidInstance(Instance))
//		return {};
//
//	switch (Instance.Modifier->Lifetime)
//	{
//	case EStatModLifetime::Duration:
//	case EStatModLifetime::Periodic:
//	{
//		auto Handle = NewUnitStatModHandle();
//
//		{
//			auto& ActiveModifier = ActiveModifiers.Add(Handle.GetID(), MakeActiveUnitStatMod(Instance));
//
//			ActiveModifier.GameTimeAtLastTick = GetWorld()->GetTimeSeconds();
//
//			if (!ActiveModifier.Modifier->bInfiniteDuration)
//				ActiveModifier.GameTimeAtExpiry = ActiveModifier.GameTimeAtLastTick + ActiveModifier.Modifier->Duration;
//
//			DispatchTickPersistentModifier(Handle, ActiveModifier);
//			
//			if (ActiveModifier.Modifier->Lifetime == EStatModLifetime::Periodic && ActiveModifier.Modifier->bPeriodicTickOnApplication)
//				ApplyModifiers(&ActiveModifier);
//			else
//				ApplyModifiers();
//
//			//ActiveModifier is potentially invalidated at this line because the callbacks could have altered or removed a modifier
//		}
//
//		return Handle;
//	}
//	case EStatModLifetime::Instant:
//	{
//		auto ActiveModifier = MakeActiveUnitStatMod(Instance);
//
//		ApplyModifiers(&ActiveModifier);
//
//		//Instant effects do not return valid handles because they are "expired" by the time the call returns anyway.
//		return {};
//	}
//	default:
//		checkNoEntry();
//		return {};
//	}
//
//}
//
//bool UUnitStatsComponent::RemoveModifier(const FUnitStatModHandle& Handle)
//{
//	auto Component = Handle.GetComponent();
//
//	if (!Component)
//		return false;
//
//	if (auto ActiveModifier = Component->ActiveModifiers.Find(Handle.GetID()))
//	{
//		if (auto World = Component->GetWorld())
//			World->GetTimerManager().ClearTimer(ActiveModifier->TimerHandle);
//	}
//	else
//		return false;
//
//	Component->ActiveModifiers.Remove(Handle.GetID());
//
//	Component->ApplyModifiers();
//
//	return true;
//}
//
//bool UUnitStatsComponent::IsValidModHandle(const FUnitStatModHandle& Handle)
//{
//	if (!IsValid(Handle.GetComponent()) || Handle.GetID() < 0)
//		return false;
//
//	return Handle.Component->ActiveModifiers.Contains(Handle.GetID());
//}
//
//void UUnitStatsComponent::RefreshRemainingDuration(const FUnitStatModHandle& Handle)
//{
//	if (auto ActiveModifier = FindActiveModifier(Handle); ActiveModifier && IsValid(ActiveModifier->Modifier))
//	{
//		if (ActiveModifier->Modifier->bInfiniteDuration)
//			ActiveModifier->GameTimeAtExpiry.Reset();
//		else
//			ActiveModifier->GameTimeAtExpiry = Handle.GetComponent()->GetWorld()->GetTimeSeconds() + ActiveModifier->Modifier->Duration;
//
//		Handle.GetComponent()->DispatchTickPersistentModifier(Handle, *ActiveModifier);
//	}
//}
//
//void UUnitStatsComponent::SetRemainingDuration(const FUnitStatModHandle& Handle, float RemainingDuration)
//{
//	if (auto ActiveModifier = FindActiveModifier(Handle))
//	{
//		ActiveModifier->GameTimeAtExpiry = Handle.GetComponent()->GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, RemainingDuration);
//
//		Handle.GetComponent()->DispatchTickPersistentModifier(Handle, *ActiveModifier);
//	}
//}
//
//void UUnitStatsComponent::SetInfiniteDuration(const FUnitStatModHandle& Handle)
//{
//	if (auto ActiveModifier = FindActiveModifier(Handle))
//	{
//		ActiveModifier->GameTimeAtExpiry.Reset();
//
//		Handle.GetComponent()->DispatchTickPersistentModifier(Handle, *ActiveModifier);
//	}
//}
//
//float UUnitStatsComponent::GetRemainingDuration(const FUnitStatModHandle& Handle)
//{
//	if (auto ActiveModifier = FindActiveModifier(Handle))
//		return ActiveModifier->GameTimeAtExpiry ? *ActiveModifier->GameTimeAtExpiry - Handle.GetComponent()->GetWorld()->GetTimeSeconds() : 0.f;
//
//	return 0.f;
//}
//
//bool UUnitStatsComponent::IsInfiniteDuration(const FUnitStatModHandle& Handle)
//{
//	if (auto ActiveModifier = FindActiveModifier(Handle))
//		return !ActiveModifier->GameTimeAtExpiry;
//
//	return false;
//}
//
//FStatChangeObserverHandle UUnitStatsComponent::AddStatChangeDelegate(FGameplayTag Tag, const FUnitStatChangeDelegate& Delegate)
//{
//	auto& DelegateList = StatChangeDelegates.FindOrAdd(Tag);
//
//	auto Handle = NewUnitStatChangeHandle(Tag);
//
//	DelegateList.Add(Handle.GetID(), Delegate);
//
//	return Handle;
//}
//
//bool UUnitStatsComponent::RemoveStatChangeDelegate(const FStatChangeObserverHandle& Handle)
//{
//	if (!IsValidChangeHandle(Handle))
//		return false;
//
//	//If handle is valid, this is guaranteed to work.
//	auto& Delegates = Handle.GetComponent()->StatChangeDelegates[Handle.GetTag()];
//
//	Delegates.Remove(Handle.GetID());
//
//	if (Delegates.Num() == 0)
//		Handle.GetComponent()->StatChangeDelegates.Remove(Handle.GetTag());
//
//	return true;
//}
//
//bool UUnitStatsComponent::IsValidChangeHandle(const FStatChangeObserverHandle& Handle)
//{
//	if (!IsValid(Handle.GetComponent()) || Handle.GetID() < 0 || !Handle.GetTag().IsValid())
//		return false;
//
//	auto Delegates = Handle.GetComponent()->StatChangeDelegates.Find(Handle.GetTag());
//	
//	if (!Delegates)
//		return false;
//
//	return Delegates->Contains(Handle.GetID());
//}
//
//FActiveUnitStatModInstance UUnitStatsComponent::MakeActiveUnitStatMod(const FStatModParams& Instance)
//{
//	FActiveUnitStatModInstance Result;
//
//	Result.Modifier = Instance.Modifier;
//
//	Result.SourceCapturedStats = Instance.SourceCapturedStats;
//
//	Result.SourceStatsComponent = Instance.SourceStatsComponent;
//
//	Result.TargetCapturedStats = MakeCapturedStats(Result.Modifier->TargetCaptureStats);
//
//	return Result;
//}
//
//FUnitStats UUnitStatsComponent::CaptureWatchedStats(const FUnitStats& Stats)
//{
//	FUnitStats Result;
//
//	Result.StatValues.Reserve(StatChangeDelegates.Num());
//
//	for (auto& [Tag, Delegates] : StatChangeDelegates)
//	{
//		auto CurrentStatValue = Stats.StatValues.Find(Tag);
//
//		if (!CurrentStatValue)
//			//Existing stats do not contain this value
//			continue;
//
//		Result.StatValues.Add(Tag, *CurrentStatValue);
//	}
//
//	return Result;
//}
//
//void UUnitStatsComponent::BroadcastChangedStats(const FUnitStats& OldStats, const FUnitStats& NewStats)
//{
//	//Gather all required delegate broadcasts before executing any of them, 
//	//so that the delegate itself can't accidentally stomp on our data while we're iterating it.
//	TArray<TTuple<FGameplayTag, FUnitStatValue, FUnitStatValue, TArray<FUnitStatChangeDelegate>>> Broadcasts;
//
//	//Non-existing stats implicitly have the default value.
//	FUnitStatValue DefaultValue;
//
//	for (auto& [Tag, Delegates] : StatChangeDelegates)
//	{
//		auto OldValue = OldStats.StatValues.Find(Tag);
//		auto NewValue = NewStats.StatValues.Find(Tag);
//
//		if (!OldValue)
//			OldValue = &DefaultValue;
//
//		if (!NewValue)
//			NewValue = &DefaultValue;
//		
//		if (*NewValue == *OldValue)
//			continue;
//
//		TArray<FUnitStatChangeDelegate> DelegateCopies;
//
//		Delegates.GenerateValueArray(DelegateCopies);
//
//		Broadcasts.Add(MakeTuple(Tag, *OldValue, *NewValue, MoveTemp(DelegateCopies)));
//	}
//
//	for (auto& Broadcast : Broadcasts)
//		for (auto& Delegate : Broadcast.Get<3>())
//			Delegate.ExecuteIfBound(this, Broadcast.Get<0>(), Broadcast.Get<1>(), Broadcast.Get<2>());
//}
//
//void UUnitStatsComponent::ApplyModifiers(const FActiveUnitStatModInstance* InstantModifier)
//{
//	//Capture watched stats so we can tell what changed
//	FUnitStats WatchedStats = CaptureWatchedStats(CurrentStats);
//
//	//Do optional instant effect
//	if (InstantModifier)
//		CalculateStats(CurrentStats, InstantModifier);
//
//	//Refresh current stats
//	CalculateStats(CurrentStats);
//
//	//Compare and dispatch
//	BroadcastChangedStats(WatchedStats, CurrentStats);
//}
//
//
//void UUnitStatsComponent::CalculateStats(FUnitStats& Stats, const FActiveUnitStatModInstance* InstantModifier, const FGameplayTagContainer& SkipCalculationPhases)
//{
//	if (bIsCalculatingStats)
//	{
//		UE_LOG(LogTemp, Error, TEXT("Recursive stat calculation on %s"), *GetPathName());
//
//		return;
//	}
//
//	TGuardValue Guard{ bIsCalculatingStats, true };
//
//	Stats.ResetModifiers();
//	
//	FStatModCalcContext Context;
//
//	Context.Target.Component = this;
//
//	Context.Target.CurrentStats = &Stats;
//	
//	auto TargetUnit = GetOwner();
//
//	FGameplayTagContainer TargetTags;
//
//	if (TargetUnit->Implements<UUnitInterface>())
//		IUnitInterface::Execute_GetUnitTags(TargetUnit);
//
//	if (InstantModifier)
//		TargetTags.AppendTags(InstantModifier->Modifier->GrantedTags);
//
//	Context.UnitTags.Add(TargetUnit, TargetTags);
//
//	//@todo: read the relevant phases from each modifier and only iterate by that instead, 
//	//because this is exhaustively checking every modifier in every phase, which is not necessary.
//	for (auto CalculationPhase : CalculationPhases)
//	{
//		if (CalculationPhase.MatchesAny(SkipCalculationPhases))
//			continue;
//
//		Context.CalculationPhase = CalculationPhase;
//
//		for (auto& [HandleID, Modifier] : ActiveModifiers)
//			if (Modifier.Modifier->Lifetime == EStatModLifetime::Duration)
//				//Continuous modifiers contribute at all times
//				DispatchApplyModifier(Context, Modifier);
//
//		if (InstantModifier)
//			DispatchApplyModifier(Context, *InstantModifier);
//	}
//
//}
//
//void UUnitStatsComponent::DispatchApplyModifier(FStatModCalcContext& Context, const FActiveUnitStatModInstance& Instance) const
//{
//	if (!IsValid(Instance.Modifier))
//		return;
//
//	Context.Target.CapturedStats = &Instance.TargetCapturedStats;
//
//	Context.Source.Component = Instance.SourceStatsComponent;
//
//	Context.Source.CapturedStats = &Instance.SourceCapturedStats;
//
//	if (!IsValid(Context.Source.Component) || !IsValid(Context.Target.Component))
//		return;
//
//	//@todo: when the calculation phase code is updated to only visit relevant modifiers, this check could be removed
//	if (!Context.CalculationPhase.MatchesAny(Instance.Modifier->CalculationPhases))
//		return;
//
//	//If self applied, we want to use the stats and tags that were supplied in the sources context,
//	//which may be different to our current state if we are only temporarily checking the results of a stat mod without applying, or instantly applying a modifier.
//	//Sources other than ourselves can just always use CurrentStats because those are the continuous stats calcluated outside of this modifier application.
//
//	if (Context.Source.Component == Context.Target.Component)
//		Context.Source.CurrentStats = Context.Target.CurrentStats;
//	else
//		Context.Source.CurrentStats = &Context.Source.Component->CurrentStats;
//
//	Context.bAllowBaseValueModification = Instance.Modifier->Lifetime != EStatModLifetime::Duration;
//
//	Instance.Modifier->ApplyModifier(Context);
//}
//
//void UUnitStatsComponent::DispatchTickPersistentModifier(FUnitStatModHandle Handle, FActiveUnitStatModInstance& Modifier)
//{
//	check(Handle.GetComponent() == this && ActiveModifiers.Contains(Handle.GetID()));
//
//	bool bExpireModifier = false;
//
//	bool bApplyPeriodicModifier = false;
//
//	float TimeDelay = -1.f;
//
//	switch (Modifier.Modifier->Lifetime)
//	{
//	case EStatModLifetime::Duration:
//	{
//		if (Modifier.GameTimeAtExpiry)
//		{
//			bExpireModifier = true;
//
//			TimeDelay = *Modifier.GameTimeAtExpiry - GetWorld()->GetTimeSeconds();
//		}
//
//		break;
//	}
//	case EStatModLifetime::Periodic:
//	{
//		double GameTimeAtNextPeriodicTick = Modifier.GameTimeAtLastTick + Modifier.Modifier->Period;
//
//		if (Modifier.GameTimeAtExpiry)
//		{
//			//Allow apply and expire to happen in the same tick if they happen close enough in time
//			//This is because floating point inaccuracy could cause the last periodic tick to be unintentionally cut off.
//
//			bExpireModifier = GameTimeAtNextPeriodicTick >= *Modifier.GameTimeAtExpiry - 0.001;
//
//			bApplyPeriodicModifier = GameTimeAtNextPeriodicTick <= *Modifier.GameTimeAtExpiry + 0.001;
//
//			TimeDelay = FMath::Min(*Modifier.GameTimeAtExpiry, GameTimeAtNextPeriodicTick) - GetWorld()->GetTimeSeconds();
//		}
//		else
//		{
//			bExpireModifier = false;
//
//			bApplyPeriodicModifier = true;
//
//			TimeDelay = GameTimeAtNextPeriodicTick - GetWorld()->GetTimeSeconds();
//		}
//
//		break;
//	}
//	case EStatModLifetime::Instant:
//	default:
//		break;
//	}
//	
//	auto& TimerManager = GetWorld()->GetTimerManager();
//
//	if (bApplyPeriodicModifier || bExpireModifier)
//	{
//		FTimerDelegate Delegate;
//
//		Delegate.BindUObject(this, &UUnitStatsComponent::TickPersistentModifier, Handle, bApplyPeriodicModifier, bExpireModifier);
//
//		//Timer manager does nothing if the delay is <= 0.f, so in that case we will queue for the next frame instead.
//		//We don't immediately execute the tick here because that would cause calling code to have to deal with callback events modifying our state
//		if (TimeDelay > 0.f)
//			TimerManager.SetTimer(Modifier.TimerHandle, Delegate, TimeDelay, false);
//		else
//		{
//			TimerManager.ClearTimer(Modifier.TimerHandle);
//
//			Modifier.TimerHandle = TimerManager.SetTimerForNextTick(Delegate);
//		}
//	}
//	else
//		//Is infinite duration or instant modifier, so there is nothing to tick.
//		TimerManager.ClearTimer(Modifier.TimerHandle);
//	
//}
//
//void UUnitStatsComponent::TickPersistentModifier(FUnitStatModHandle Handle, bool bApplyPeriodicModifier, bool bExpireModifier)
//{
//	check(Handle.GetComponent() == this);
//	checkf(ActiveModifiers.Contains(Handle.GetID()), TEXT("Timer event for modifier tick was not cleared properly on %s"), *GetPathName());
//
//	if (bApplyPeriodicModifier) 
//	{
//		auto ActiveModifier = ActiveModifiers.Find(Handle.GetID());
//
//		check(ActiveModifier);
//
//		ActiveModifier->GameTimeAtLastTick = GetWorld()->GetTimeSeconds();
//
//		if (!bExpireModifier)
//			DispatchTickPersistentModifier(Handle, *ActiveModifier);
//
//		ApplyModifiers(ActiveModifier);
//	}
//
//	if (bExpireModifier)
//		RemoveModifier(Handle);
//}
//
//FActiveUnitStatModInstance* UUnitStatsComponent::FindActiveModifier(FUnitStatModHandle Handle)
//{
//	if (!Handle.GetComponent())
//		return nullptr;
//
//	return Handle.GetComponent()->ActiveModifiers.Find(Handle.GetID());
//}
//
////
////void UUnitStatsComponent::DispatchTickPersistentModifier(FUnitStatModHandle Handle, FActiveUnitStatModInstance& Modifier, float TimeDelay)
////{
////	FTimerDelegate Delegate;
////
////	Delegate.BindUObject(this, &UUnitStatsComponent::TickPersistentModifier, Handle);
////
////	GetWorld()->GetTimerManager().SetTimer(Modifier.TimerHandle, Delegate, TimeDelay, false);
////}
////
////void UUnitStatsComponent::TickPersistentModifier(FUnitStatModHandle Handle)
////{
////	if (!IsValidModHandle(Handle))
////		return;
////
////	bool bIsExpired = true;
////
////	{
////		auto ActiveModifier = ActiveModifiers.Find(Handle.GetID());
////
////		check(ActiveModifier);
////
////		if (ActiveModifier->Modifier->Lifetime == EStatModLifetime::Periodic)
////		{
////			double Now = GetWorld()->GetTimeSeconds();
////
////			double ElapsedDuration = Now - ActiveModifier->GameTimeAtLastTick;
////
////			//If period is less than delta time, makes sure that we execute multiple times
////			//If duration is a multiple of period, makes sure we don't clip the last tick due to numeric inaccuracy
////			int32 NumTicks = FMath::Floor(ElapsedDuration / ActiveModifier->Modifier->Period + 0.001);
////
////			ActiveModifier->GameTimeAtLastTick = Now;
////
////			double RemainingDuration = ActiveModifier->GameTimeAtExpiry - Now;
////
////			double DurationUntilNextTick = FMath::Min(RemainingDuration, ActiveModifier->Modifier->Period);
////
////			bIsExpired = RemainingDuration < 0.001;
////
////			if (!bIsExpired)
////				DispatchTickPersistentModifier(Handle, *ActiveModifier, DurationUntilNextTick);
////
////			for (int32 i = 0; i < NumTicks; ++i)
////			{
////				//Stat change callbacks can invalidate the active modifier, so we must retrieve it from the handle every time.
////				if (auto CurrentActiveModifier = ActiveModifiers.Find(Handle.GetID()))
////					ApplyModifiers(CurrentActiveModifier);
////				else
////					break;
////			}
////		}
////	}
////
////	if (bIsExpired)
////		RemoveModifier(Handle);
////
////};
//FUnitStatModHandle UUnitStatsComponent::NewUnitStatModHandle()
//{
//	FUnitStatModHandle Handle;
//
//	Handle.ID = NextModHandleID++;
//
//	Handle.Component = this;
//
//	return Handle;
//}
//
//FStatChangeObserverHandle UUnitStatsComponent::NewUnitStatChangeHandle(FGameplayTag Tag)
//{
//	FStatChangeObserverHandle Handle;
//
//	Handle.ID = NextChangeHandleID++;
//
//	Handle.Component = this;
//
//	Handle.Tag = Tag;
//
//	return Handle;
//}

//PRAGMA_ENABLE_OPTIMIZATION