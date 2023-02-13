//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h" 
#include "WorldCursor.generated.h"

class UWorldCursorComponent;

USTRUCT(BlueprintType)
struct ZOMBIES_API FWorldCursorLocation
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	FVector2D ScreenLocation{};

	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	FVector WorldLocation{};

	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	FVector WorldDirection{};

};

//The types of input actions that the world cursor can perform
UENUM(BlueprintType)
enum class EWorldCursorEventType : uint8
{
	//Location has changed.
	//Capture IE_Pressed to start receiving IE_Axis updates every frame
	//Uncapture IE_Axis to stop receiving updates.
	//IE_Released is called when stopped.
	Location,
	//Selection input event
	Selection,
	//Context action input event
	ContextAction,
	//Event for delegate add on IE_Pressed and removed on IE_Released
	Delegate,
};

UENUM(BlueprintType)
enum class EWorldCursorEventResult : uint8
{
	//Handler does not care about this event and did nothing in response to it
	Ignore,
	//Handler handled this event but did not capture it, leaving it available for other delegates added during the handling of this event.
	Handled,
	//Handler handled and captured the event.
	Captured,
};

USTRUCT(BlueprintType)
struct FWorldCursorDelegateHandle
{
	GENERATED_BODY()
public:
	
	FWorldCursorDelegateHandle() = default;

	FORCEINLINE FWorldCursorDelegateHandle(UWorldCursorComponent* InCursor, int32 InID) : Cursor(InCursor), ID(InID) {}

	FORCEINLINE UWorldCursorComponent* GetCursor() const { return Cursor; }

	FORCEINLINE int32 GetID() const { return ID; }

	FORCEINLINE void Reset() { *this = FWorldCursorDelegateHandle{}; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = WorldCursor, Meta = (AllowPrivateAccess = "True"))
	UWorldCursorComponent* Cursor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = WorldCursor, Meta = (AllowPrivateAccess = "True"))
	int32 ID = -1;
};

//Parameters for world cursor event handlers
USTRUCT(BlueprintType)
struct ZOMBIES_API FWorldCursorEventParams
{
	GENERATED_BODY()
public:

	//The cursor that created this event
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	class UWorldCursorComponent* Cursor = nullptr;

	//The type of event, similar to binding name in UE input system
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	EWorldCursorEventType Type = EWorldCursorEventType::Location;

	//The input type, same as in UE input system
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	TEnumAsByte<EInputEvent> Input = EInputEvent::IE_Axis;

	//The cursor coordinates
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	FWorldCursorLocation Location;

	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	FWorldCursorDelegateHandle Handle;
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(EWorldCursorEventResult, FWorldCursorEventDelegate, const FWorldCursorEventParams&, Params);

