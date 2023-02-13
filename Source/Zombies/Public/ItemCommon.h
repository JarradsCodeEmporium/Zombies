//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Gameplaytags.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemCommon.generated.h"

class UInventoryComponent;

class AItem;


//An item rect that has two orientations; horizontal or vertical.
USTRUCT(BlueprintType)
struct FItemRect
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = ItemRect)
	FIntPoint Min{ ForceInit };

	UPROPERTY(BlueprintReadWrite, Category = ItemRect)
	FIntPoint Max{ ForceInit };

	UPROPERTY(BlueprintReadWrite, Category = ItemRect)
	bool bIsRotated = false;

	FORCEINLINE bool operator==(const FItemRect& Other) const { return Min == Other.Min && Max == Other.Max && bIsRotated == Other.bIsRotated; }
	FORCEINLINE bool operator!=(const FItemRect& Other) const { return !(*this == Other); }

	FORCEINLINE bool IsValid() const
	{
		return Min.X <= Max.X && Min.Y <= Max.Y;
	}

	FORCEINLINE void Clip(const FItemRect& Other)
	{
		Min = Min.ComponentMax(Other.Min);
		Max = Max.ComponentMin(Other.Max);

		Max = Max.ComponentMax(Min);
	}

	FORCEINLINE bool Intersects(const FItemRect& Other) const
	{
		return Other.Min.X < Max.X && Other.Max.X > Min.X && Other.Min.Y < Max.Y && Other.Max.Y > Min.Y;
	}

	FORCEINLINE bool Contains(const FIntPoint& Point) const
	{
		return Point.X > Min.X && Point.Y > Min.Y && Point.X < Min.X && Point.Y < Min.Y;
	}

	FORCEINLINE void SetRotated(bool bNewIsRotated)
	{
		if (bIsRotated != bNewIsRotated)
			Rotate();
	}

	FORCEINLINE void Rotate()
	{
		bIsRotated = !bIsRotated;

		auto Size = Max - Min;

		Max.X = Min.X + Size.Y;
		Max.Y = Min.Y + Size.X;
	}

	FORCEINLINE void SetRotatedSize(FIntPoint Size)
	{
		Max = Min + Size;
	}

	FORCEINLINE void SetUnrotatedSize(FIntPoint Size)
	{
		Max = Min + (bIsRotated ? FIntPoint{ Size.Y, Size.X } : Size);
	}

	FORCEINLINE FIntPoint GetRotatedSize() const
	{
		return Max - Min;
	}

	FORCEINLINE FIntPoint GetUnrotatedSize() const
	{
		auto Size = GetRotatedSize();

		return bIsRotated ? FIntPoint{Size.Y, Size.X} : Size;
	}

	FORCEINLINE bool IsSameSize(const FItemRect& Other) const
	{
		return GetUnrotatedSize() == Other.GetUnrotatedSize();
	}

};

FORCEINLINE uint32 GetTypeHash(const FItemRect& Rect)
{
	return HashCombineFast(HashCombineFast(GetTypeHash(Rect.Min), GetTypeHash(Rect.Max)), GetTypeHash(Rect.bIsRotated));
}

template<>
struct TStructOpsTypeTraits<FItemRect> : public TStructOpsTypeTraitsBase2<FItemRect>
{
	enum
	{
		WithIdenticalViaEquality = true,
	};
};


//Refers to a container inside an inventory component
USTRUCT(BlueprintType)
struct ZOMBIES_API FInventoryContainerHandle
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = Item)
	UInventoryComponent* Inventory = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Item)
	FGameplayTag Container;

	bool operator==(const FInventoryContainerHandle& Other) const { return Inventory == Other.Inventory && Container == Other.Container; }
	bool operator!=(const FInventoryContainerHandle& Other) const { return !(*this == Other); }

	FORCEINLINE void Reset() { *this = FInventoryContainerHandle{}; }

	FORCEINLINE bool IsValid() const { return Inventory && Container.IsValid(); }
};

FORCEINLINE uint32 GetTypeHash(const FInventoryContainerHandle& Slot)
{
	return HashCombineFast(GetTypeHash(Slot.Inventory), GetTypeHash(Slot.Container));
}

template<>
struct TStructOpsTypeTraits<FInventoryContainerHandle> : public TStructOpsTypeTraitsBase2<FInventoryContainerHandle>
{
	enum
	{
		WithIdenticalViaEquality = true,
	};
};

//Refers to a rectangle within an inventory container
USTRUCT(BlueprintType)
struct ZOMBIES_API FInventoryLocationHandle : public FInventoryContainerHandle
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = Item)
	FItemRect Rect;

	bool operator==(const FInventoryLocationHandle& Other) const { return FInventoryContainerHandle::operator==(Other) && Rect == Other.Rect; }
	bool operator!=(const FInventoryLocationHandle& Other) const { return !(*this == Other); }

	FORCEINLINE void Reset() { *this = FInventoryLocationHandle{}; }

	FORCEINLINE bool IsValid() const { return FInventoryContainerHandle::IsValid() && Rect.IsValid(); }
};

FORCEINLINE uint32 GetTypeHash(const FInventoryLocationHandle& Slot)
{
	return HashCombineFast(GetTypeHash(Slot.Inventory), HashCombineFast(GetTypeHash(Slot.Container), GetTypeHash(Slot.Rect)));
}

template<>
struct TStructOpsTypeTraits<FInventoryLocationHandle> : public TStructOpsTypeTraitsBase2<FInventoryLocationHandle>
{
	enum
	{
		WithIdenticalViaEquality = true,
	};
};

UENUM(BlueprintType)
enum class EInventoryContainerChangeType : uint8
{
	Added,
	Moved,
	Removed,
};

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FInventoryContainerChangedDelegate, AItem*, Item, const FInventoryContainerHandle&, Container, EInventoryContainerChangeType, ChangeType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventoryLocationChangedDelegate, AItem*, Item, const FInventoryLocationHandle&, OldLocation);

//Data for a slot type in an inventory component
USTRUCT(BlueprintType)
struct ZOMBIES_API FInventoryContainer
{
	GENERATED_BODY()
public:

	//Display name of slot type
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	FText Name;

	//Filters the allowed items in this slot type. Only items whose tags match this query will be allowed.
	//Empty filter allows all items.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	FGameplayTagQuery ItemFilter;

	//How big is this slot type
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	FIntPoint Size{ 1, 1 };

	//How many items are allowed in this slot type. Negative implies no limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	int32 MaxItems = -1;

	//The current items stored in this slot type
	UPROPERTY(Transient, BlueprintReadOnly, Category = Item)
	TArray<AItem*> Items;

	TArray<FInventoryContainerChangedDelegate> OnChanged;
};

UCLASS()
class UItemRectBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE bool IsValid(FItemRect Rect) { return Rect.IsValid(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FItemRect Clip(FItemRect A, FItemRect B) { auto Result = A; Result.Clip(B); return Result; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE bool Contains(FItemRect Rect, FIntPoint Point) { return Rect.Contains(Point); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE bool Intersects(FItemRect A, FItemRect B) { return A.Intersects(B); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FItemRect SetRotatedSize(FItemRect Rect, FIntPoint Size) { auto Result = Rect; Result.SetRotatedSize(Size); return Result; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FItemRect SetUnrotatedSize(FItemRect Rect, FIntPoint Size) { auto Result = Rect; Result.SetUnrotatedSize(Size); return Result; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FIntPoint GetRotatedSize(FItemRect Rect) { return Rect.GetRotatedSize(); }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FIntPoint GetUnrotatedSize(FItemRect Rect) { return Rect.GetUnrotatedSize(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FItemRect Rotate(FItemRect Rect) { auto Result = Rect; Result.Rotate(); return Result; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE FItemRect SetRotated(FItemRect Rect, bool bNewIsRotated) { auto Result = Rect; Result.SetRotated(bNewIsRotated); return Result; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = ItemRect)
	static FORCEINLINE bool IsSameSize(FItemRect A, FItemRect B) { return A.IsSameSize(B); }
};


UENUM(BlueprintType)
enum class EItemIconType : uint8
{
	//Item icon is a simple static texture
	StaticImage,
	//Item icon is a rendered 3d model that is the same across all instances of the item
	ClassRendered,
	//Item icon is a rendered 3d model that is unique to the specific item (expensive)
	InstanceRendered,
};
