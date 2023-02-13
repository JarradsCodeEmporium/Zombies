//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTags.h"
#include "UtilityTask.generated.h"

class UUtilityComponent;

/**
 * Scores and implements a task that should be performed by utility AI.
 * Typically an actor will keep a single instance of each type of task they support, and then run it multiple times when it reaches the top priority.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class ZOMBIES_API UUtilityTask : public UObject
{
	GENERATED_BODY()
public:

	UUtilityTask();

	virtual class UWorld* GetWorld() const override;

	//Sets the pawn that will receive the actions from this task
	UFUNCTION(BlueprintCallable, Category = Utility)
	virtual void SetPawn(class APawn* InPawn);

	FORCEINLINE auto GetPawn() const { return Pawn; }

	//Sets the utility component that this task is added to
	UFUNCTION(BlueprintCallable, Category = Utility)
	void SetUtilityComponent(class UUtilityComponent* InComponent);

	FORCEINLINE auto GetUtilityComponent() const { return UtilityComponent; }

	//Update the score of this task.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Utility)
	void UpdateScore();

	//Return the last calculated score
	FORCEINLINE float GetScore() const { return Score; }

	FORCEINLINE const FGameplayTagContainer& GetResourceTags() const { return ResourceTags; }

	FORCEINLINE float GetUpdateScoreFreq() const { return UpdateScoreFreq; }

	//Whether this task is currently selected by the owning utility component
	UFUNCTION(BlueprintCallable, Category = Utility)
	bool IsRunning() const;

	//The task has been selected to run. Start task logic.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Utility)
	void BeginTask();

	//Tick task logic
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Utility)
	void TickTask(float DeltaTime);

	//The task has fallen in priority ranking. End task logic.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Utility)
	void EndTask();

protected:

	//The pawn this task is currently associated with
	UPROPERTY(BlueprintReadOnly, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	class APawn* Pawn;

	//The utility component that this task is currently registered with
	UPROPERTY(BlueprintReadOnly, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	class UUtilityComponent* UtilityComponent;

	//The most recently calculated score
	UPROPERTY(BlueprintReadWrite, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	float Score = 0.f;

	//The resource tags that this task uses. Defaults to the base "UtilityTask" tag, which implies mutally exclusive task activations.
	//Specify only sub tags to occupy only a portion of the agents resources. 
	//E.G. "UtilityTask.Movement" will only occupy the movement, leaving "UtilityTask.UpperBody" free to shoot while moving.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	FGameplayTagContainer ResourceTags;

	//How often the score is automatically updated. <= 0.0 does not update automatically.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	float UpdateScoreFreq = 0.25f;

	FTimerHandle UpdateScoreHandle;
};
