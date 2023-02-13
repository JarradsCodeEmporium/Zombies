//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "BrainComponent.h"
#include "UtilityComponent.generated.h"

class UUtilityTask;

/**
 * A list of tasks ordered by score. Selects a task based on its score and executes it until another task supersedes it.
 */
UCLASS()
class ZOMBIES_API UUtilityComponent : public UBrainComponent
{
	GENERATED_BODY()
public:

	UUtilityComponent();

protected:

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const auto& GetTasks() const { return Tasks; }

	FORCEINLINE const auto& GetActiveTasks() const { return ActiveTasks; }

	const bool IsRunningTask(const class UUtilityTask* Task) const;

	//Begin UBrainComponent

	virtual void StartLogic() override;

	virtual void RestartLogic() override;

	virtual void StopLogic(const FString& Reason) override;

	virtual void Cleanup() override;

	virtual void PauseLogic(const FString& Reason) override;

	virtual EAILogicResuming::Type ResumeLogic(const FString& Reason) override;

	virtual bool IsRunning() const override;

	virtual bool IsPaused() const override;

	//End UBrainComponent

protected:

	//List of tasks that can be chosen from, sorted by their score.
	UPROPERTY(BlueprintReadOnly, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	TArray<class UUtilityTask*> Tasks;

	//The tasks that are currently being run
	UPROPERTY(BlueprintReadOnly, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	TArray<class UUtilityTask*> ActiveTasks;

	friend UUtilityTask;

	//Only called by utility task.
	void NotifyTaskAdded(class UUtilityTask* Task);

	//Only called by utility task.
	void NotifyTaskRemoved(class UUtilityTask* Task);

	void SetTaskActive(class UUtilityTask* Task, bool bNewIsActive);

	void ClearActiveTasks();

	void UpdateTasks();

	void TickTasks(float DeltaTime);

	bool bEnableLogic = false;

};
