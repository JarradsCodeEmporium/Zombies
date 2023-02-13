//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WorldCursor.h"
#include "WorldCursorComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UWorldCursorComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWorldCursorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	static bool IsValidDelegateHandle(const FWorldCursorDelegateHandle& Handle);

	//Adds event delegate at highest priority. Safe to call inside any delegate.
	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	FWorldCursorDelegateHandle AddDelegate(const FWorldCursorEventDelegate& Delegate);

	//Removes the delegate. Safe to call inside any delegate.
	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	static void RemoveDelegate(const FWorldCursorDelegateHandle& Handle);

	//UFUNCTION(BlueprintCallable, Category = WorldCursor)
	//static void BeginCapture(const FWorldCursorDelegateHandle& Handle, EWorldCursorEventType Type);

	//UFUNCTION(BlueprintCallable, Category = WorldCursor)
	//static void BeginCaptureAll(const FWorldCursorDelegateHandle& Handle);

	//UFUNCTION(BlueprintCallable, Category = WorldCursor)
	//static void EndCapture(const FWorldCursorDelegateHandle& Handle, EWorldCursorEventType Type);

	//UFUNCTION(BlueprintCallable, Category = WorldCursor)
	//static void EndCaptureAll(const FWorldCursorDelegateHandle& Handle);

	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	void SelectionPressed();

	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	void SelectionReleased();

	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	void ContextActionPressed();

	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	void ContextActionReleased();

	UFUNCTION(BlueprintCallable, Category = WorldCursor)
	void CursorLocationUpdated(const FWorldCursorLocation& NewCursorLocation);

	FORCEINLINE const FWorldCursorLocation& GetCursorLocation() const { return CursorLocation; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = WorldCursor, Meta = (AllowPrivateAccess = "True"))
	FWorldCursorLocation CursorLocation;

	TArray<TPair<int32, FWorldCursorEventDelegate>> Delegates;

	TOptional<int32> Captures[(int32)EWorldCursorEventType::Delegate];

	FORCEINLINE TOptional<int32>& GetCapture(EWorldCursorEventType Type) { return Captures[(int32)Type]; }
	FORCEINLINE const TOptional<int32>& GetCapture(EWorldCursorEventType Type) const { return Captures[(int32)Type]; }

	void BeginCaptureInternal(int32 ID, EWorldCursorEventType Type);
	void EndCaptureInternal(int32 ID, EWorldCursorEventType Type);


	bool HasDelegate(int32 ID) const;

	void BeginCaptureAllInternal(int32 ID);

	//Executes end events for a delegate if it currently has a capture
	//Note that the result of end events is always ignored
	void EndCaptureAllInternal(int32 ID);

	//Execute an event. Note that this must always be done by lookup every time,
	//because events themselves will be modifying the delegate list.
	EWorldCursorEventResult ExecuteEvent(int32 ID, EWorldCursorEventType Type, EInputEvent Input);

	void ExecuteEventStack(EWorldCursorEventType Type, EInputEvent Input);

	int32 NextID = 0;

	FWorldCursorDelegateHandle NewDelegateHandle();
};
