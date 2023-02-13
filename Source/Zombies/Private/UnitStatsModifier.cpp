//Copyright Jarrad Alexander 2022


#include "UnitStatsModifier.h"

void UUnitStatsModifier::CalculateModifier_Implementation(const FStatModCalcContext& Context) const
{
	auto Value = GetStatValue(Context, TestStatTag, EStatModUnit::Target, EStatValueCapture::Current, EStatValueType::Base);

	SetTargetCurrentStatValue(Context, TestStatTag, EStatValueType::Base, Value + TestAdd);

	//UE_LOG(LogTemp, Warning, TEXT("Modified stat from %.2f to %.2f"), Value, Value + TestAdd);

}
