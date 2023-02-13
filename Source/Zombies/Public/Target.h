//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "Target.generated.h"

class USceneComponent;
class AActor;

UENUM(BlueprintType)
enum class ETargetType : uint8
{
	//Default state, no target data
	None,

	//Targeting a world space location
	WorldLocation,

	//Targeting a location relative to a specific component
	RelativeLocation,
};

//Variant type specifying a point in space for focusing
USTRUCT(BlueprintType)
struct ZOMBIES_API FTarget
{
	GENERATED_BODY()
public:

	FTarget() = default;

	FORCEINLINE FTarget(const FVector& InWorldLocation)
	{
		SetWorldLocation(InWorldLocation);
	}

	FORCEINLINE FTarget(class USceneComponent* InComponent, const FVector& InRelativeLocation = FVector{ 0.0 })
	{
		SetRelativeLocation(InComponent, InRelativeLocation);
	}

	FORCEINLINE FTarget(const class AActor* InActor, const FVector& InRelativeLocation = FVector{ 0.0 })
	{
		SetRelativeLocation(InActor, InRelativeLocation);
	}

	FORCEINLINE bool operator==(const FTarget& Other) const { return Type == Other.Type && Component == Other.Component && Location == Other.Location; }

	FORCEINLINE bool operator!=(const FTarget& Other) const { return !(*this == Other); }

	FORCEINLINE operator FVector() const { return GetLocation(); }

	FORCEINLINE operator class USceneComponent* () const { return GetComponent(); }

	FORCEINLINE operator AActor* () const { return GetActor(); }

	FORCEINLINE operator bool() const { return IsValid(); }

	//Component that this target refers to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	class USceneComponent* Component = nullptr;

	//Relative location on Component if it is valid, world location if not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	FVector Location{ 0.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Focus)
	ETargetType Type = ETargetType::None;

	bool IsValid() const;

	FORCEINLINE void Clear() { *this = FTarget{}; }

	FVector GetLocation() const;

	FORCEINLINE USceneComponent* GetComponent() const { return Component; }

	AActor* GetActor() const;

	void SetWorldLocation(const FVector& InWorldLocation);

	bool SetRelativeLocation(USceneComponent* InComponent, const FVector& InRelativeLocation);

	bool SetRelativeLocation(const AActor* InActor, const FVector& InRelativeLocation);

};
