//Copyright Jarrad Alexander 2022


#include "UnitStatsCommon.h"
#include "UnitStatsComponent.h"
#include "GameplayTagAssetInterface.h"

float FUnitStatValue::GetValue(EStatValueType Type) const
{
	switch (Type)
	{
	case EStatValueType::Base:
		return BaseValue;
	case EStatValueType::Modified:
		return ModifiedValue;
	default:
		checkNoEntry();
		return 0.f;
	}
}

void FUnitStatValue::SetValue(EStatValueType Type, float Value)
{
	switch (Type)
	{
	case EStatValueType::Base:
		BaseValue = Value;
		return;
	case EStatValueType::Modified:
		ModifiedValue = Value;
		return;
	default:
		checkNoEntry();
		return;
	}
}

float FUnitStats::GetValue(FGameplayTag Tag, EStatValueType Type) const
{
	auto StatValue = StatValues.Find(Tag);

	if (!StatValue)
		return 0.0f;

	return StatValue->GetValue(Type);
}

void FUnitStats::SetValue(FGameplayTag Tag, EStatValueType Type, float Value)
{
	StatValues.FindOrAdd(Tag).SetValue(Type, Value);

	//auto StatValue = StatValues.Find(Tag);

	//if (!StatValue)
	//	return;

	//StatValue->SetValue(Type, Value);
}

void FUnitStats::ResetModifiers()
{
	for (auto& [Key, Value] : StatValues)
		Value.ResetModifiers();
}

void FStatModInstance::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Modifier);
	Collector.AddReferencedObject(SourceStatsComponent);
	Collector.AddReferencedObject(SourceActor);
	Collector.AddReferencedObject(TargetStatsComponent);
}

FString FStatModInstance::GetReferencerName() const
{
	static const FString Name = "FStatModInstance";
	return Name;
}

float FStatModCalcContext::FUnit::GetValue(FGameplayTag Tag, EStatValueCapture Capture, EStatValueType Type) const
{
	switch (Capture)
	{

	case EStatValueCapture::Current:
		return CurrentStats->GetValue(Tag, Type);

	case EStatValueCapture::Captured:

		if (auto Result = CapturedStats->StatValues.Find(Tag))
			return Result->GetValue(Type);
		else
			return CurrentStats->GetValue(Tag, Type);

	default:
		checkNoEntry();
		return 0.0f;
	}
}

const FGameplayTagContainer& FStatModCalcContext::GetUnitTags(EStatModUnit Unit) const
{
	//Lazy evaluate and then cache tags during stat evaluation because:
	//most modifiers probably don't access any tags.
	//modifiers always share target, and will often share sources, which would duplicate the tag gathering work.
	//Tags should not change during calculation

	auto Component = Unit == EStatModUnit::Source ? Source.Component : Target.Component;

	if (!Component)
		return FGameplayTagContainer::EmptyContainer;

	auto SourceUnit = Component->GetOwner();

	if (!SourceUnit)
		return FGameplayTagContainer::EmptyContainer;

	if (auto Tags = UnitTags.Find(SourceUnit))
		return *Tags;

	auto Interface = Cast<IGameplayTagAssetInterface>(SourceUnit);

	if (!Interface)
		return FGameplayTagContainer::EmptyContainer;

	auto& Tags = UnitTags.Add(SourceUnit);

	Interface->GetOwnedGameplayTags(Tags);

	return Tags;
}

float FStatModCalcContext::GetValue(FGameplayTag Tag, EStatModUnit Unit, EStatValueCapture Capture, EStatValueType Type) const
{
	switch (Unit)
	{
	case EStatModUnit::Source:
		return Source.GetValue(Tag, Capture, Type);
	case EStatModUnit::Target:
		return Target.GetValue(Tag, Capture, Type);
	default:
		checkNoEntry();
		return 0.0f;
	}
	
}

void FStatModCalcContext::SetTargetCurrentValue(FGameplayTag Tag, EStatValueType Type, float Value) const
{
	Target.CurrentStats->SetValue(Tag, bAllowBaseValueModification ? Type : EStatValueType::Modified, Value);
}
