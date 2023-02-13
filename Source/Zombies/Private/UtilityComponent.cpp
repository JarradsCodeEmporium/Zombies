//Copyright Jarrad Alexander 2022


#include "UtilityComponent.h"
#include "UtilityTask.h"

UUtilityComponent::UUtilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UUtilityComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);



}

void UUtilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTasks();

	TickTasks(DeltaTime);
}

void UUtilityComponent::NotifyTaskAdded(UUtilityTask* Task)
{
	check(IsValid(Task));

	Tasks.AddUnique(Task);

	//Ensure a score calculation just before add so we know immediately if this task should run
	//Otherwise we'd always have to wait for the first update event/tick to happen, which would often be too slow
	Task->UpdateScore();

	UpdateTasks();
}

void UUtilityComponent::NotifyTaskRemoved(UUtilityTask* Task)
{
	check(IsValid(Task));

	SetTaskActive(Task, false);

	if (Tasks.RemoveSingle(Task) != 1)
		return;
	
	UpdateTasks();
}

void UUtilityComponent::SetTaskActive(class UUtilityTask* Task, bool bNewIsActive)
{
	if (!IsValid(Task))
		return;

	if (bNewIsActive && !ActiveTasks.Contains(Task))
	{
		ActiveTasks.Add(Task);

		Task->BeginTask();
	}
	else if (!bNewIsActive)
		if (ActiveTasks.RemoveSingleSwap(Task) == 1)
			Task->EndTask();
}

void UUtilityComponent::ClearActiveTasks()
{
	auto EndTasks = MoveTemp(ActiveTasks);

	for (auto Task : EndTasks)
		if (IsValid(Task))
			Task->EndTask();
}

void UUtilityComponent::UpdateTasks()
{
	Tasks.Remove(nullptr);

	if (Tasks.Num() == 0)
	{
		ClearActiveTasks();
		return;
	}

	Tasks.Sort(
		[](UUtilityTask& Lhs, UUtilityTask& Rhs) -> bool
		{
			return Lhs.GetScore() < Rhs.GetScore();
		}
	);

	auto TaskList = Tasks;

	FGameplayTagContainer UsedResources;

	for (auto Task : TaskList)
	{
		if (Task->GetScore() > 0.f)
			if (!Task->GetResourceTags().HasAny(UsedResources) && !UsedResources.HasAny(Task->GetResourceTags()))
			{
				UsedResources.AppendTags(Task->GetResourceTags());

				SetTaskActive(Task, true);

				continue;
			}

		SetTaskActive(Task, false);
	}
}

void UUtilityComponent::TickTasks(float DeltaTime)
{
	auto ActiveTickTasks = ActiveTasks;

	for (auto Task : ActiveTickTasks)
		if (IsValid(Task))
			Task->TickTask(DeltaTime);
}

const bool UUtilityComponent::IsRunningTask(const UUtilityTask* Task) const
{
	if (IsValid(Task))
		return false;

	return ActiveTasks.Contains(Task);
}

void UUtilityComponent::StartLogic()
{
	bEnableLogic = true;

	UpdateTasks();
}

void UUtilityComponent::RestartLogic()
{
	bEnableLogic = true;

	ClearActiveTasks();

	UpdateTasks();
}

void UUtilityComponent::StopLogic(const FString& Reason)
{
	bEnableLogic = false;

	ClearActiveTasks();
}

void UUtilityComponent::Cleanup()
{
	auto EndTasks = Tasks;

	for (auto Task : EndTasks)
		if (IsValid(Task))
			Task->SetUtilityComponent(nullptr);

	bEnableLogic = false;
}

void UUtilityComponent::PauseLogic(const FString& Reason)
{
	bEnableLogic = false;
}

EAILogicResuming::Type UUtilityComponent::ResumeLogic(const FString& Reason)
{
	bEnableLogic = true;

	return Super::ResumeLogic(Reason);
}

bool UUtilityComponent::IsRunning() const
{
	return bEnableLogic;
}

bool UUtilityComponent::IsPaused() const
{
	return !bEnableLogic;
}
