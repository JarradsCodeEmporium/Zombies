//Copyright Jarrad Alexander 2022


#include "UtilityTask.h"
#include "NativeTags.h"
#include "UtilityComponent.h"



UUtilityTask::UUtilityTask()
{
	ResourceTags.AddTag(Tag_UtilityTask);
}

UWorld* UUtilityTask::GetWorld() const
{
	if (auto Outer = GetOuter(); Outer && !HasAnyFlags(RF_ClassDefaultObject))
		return Outer->GetWorld();

	return nullptr;
}

void UUtilityTask::SetPawn(APawn* InPawn)
{
	Pawn = InPawn;
}

void UUtilityTask::SetUtilityComponent(UUtilityComponent* InComponent)
{
	if (InComponent == UtilityComponent)
		return;

	if (UpdateScoreHandle.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(UpdateScoreHandle);

	if (IsValid(UtilityComponent))
		UtilityComponent->NotifyTaskRemoved(this);

	UtilityComponent = InComponent;

	if (IsValid(UtilityComponent))
		UtilityComponent->NotifyTaskAdded(this);

	if (UpdateScoreFreq > 0.f)
		GetWorld()->GetTimerManager().SetTimer(UpdateScoreHandle, this, &UUtilityTask::UpdateScore, UpdateScoreFreq, true);

}


void UUtilityTask::UpdateScore_Implementation()
{
	Score = 0.f;
}

bool UUtilityTask::IsRunning() const
{
	if (!IsValid(UtilityComponent))
		return false;

	return UtilityComponent->IsRunningTask(this);
}

void UUtilityTask::BeginTask_Implementation()
{
}

void UUtilityTask::TickTask_Implementation(float DeltaTime)
{
}

void UUtilityTask::EndTask_Implementation()
{
}
