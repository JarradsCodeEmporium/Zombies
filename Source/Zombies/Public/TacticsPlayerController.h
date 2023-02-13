//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TacticsPlayerController.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FPlayerHotkeyDelegate, EInputEvent, Event);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerHotkeyMulticastDelegate, EInputEvent, Event);

/**
 * 
 */
UCLASS()
class ZOMBIES_API ATacticsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATacticsPlayerController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void SetupInputComponent() override;


	UFUNCTION(BlueprintCallable, Category = Game)
	void SetCameraControlMode(bool bEnabled);

	//The built in get mouse pos using viewport client is actually broken and doesn't update properly during mouse capture events.
	UFUNCTION(BlueprintCallable, Category = Game)
	bool GetMousePositionOnViewport(FVector2D& OutMousePosition) const;


	//Bind a delegate to hotkey. Exposes easy to use hook for widget blueprints to detect when 
	//player presses relevant keys without needing to have the widget focused.
	//Useful for action bar hotkeys especially.
	UFUNCTION(BlueprintCallable, Category = Game)
	void BindPlayerHotkey(FPlayerHotkeyDelegate Delegate, FName InputAction);

	//Unbind a delegate from hotkey.
	UFUNCTION(BlueprintCallable, Category = Game)
	void UnbindPlayerHotkey(FPlayerHotkeyDelegate Delegate, FName InputAction);

	//Input actions to bind to the easily accessible hotkey
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Game)
	TArray<FName> PlayerHotkeyInputActions;

protected:

	void PlayerHotkeyInputAction(FName InputAction, EInputEvent Event);

	//Delegates listening to UI hotkey actions on this character (i.e. the ability slot widgets on player UI)
	//Essentially routes a relevant subset of input events back through the UI even if it doesn't have focus.
	TMap<FName, FPlayerHotkeyMulticastDelegate> PlayerHotkeyBindings;

	TOptional<FVector2D> CapturedMousePosition;

};
