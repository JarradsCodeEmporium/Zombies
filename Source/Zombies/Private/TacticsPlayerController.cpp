//Copyright Jarrad Alexander 2022


#include "TacticsPlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"

ATacticsPlayerController::ATacticsPlayerController()
{
	bShowMouseCursor = true;
}

void ATacticsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetCameraControlMode(false);

}

void ATacticsPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATacticsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	//Route hotkey input actions through a single function that then does the indirect dispatch to blueprint.
	for (auto Action : PlayerHotkeyInputActions)
	{
		FInputActionBinding Pressed{ Action, IE_Pressed };
		Pressed.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &ATacticsPlayerController::PlayerHotkeyInputAction, Action, EInputEvent::IE_Pressed);
		InputComponent->AddActionBinding(Pressed);
		
		FInputActionBinding Released{ Action, IE_Released };
		Released.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &ATacticsPlayerController::PlayerHotkeyInputAction, Action, EInputEvent::IE_Released);
		InputComponent->AddActionBinding(Released);
	}

}

void ATacticsPlayerController::SetCameraControlMode(bool bEnabled)
{
	if (bEnabled)
	{
		float X, Y;

		if (GetMousePosition(X, Y))
			CapturedMousePosition = FVector2D{ X,Y };



		bShowMouseCursor = false;

		FInputModeGameOnly InputMode;

		SetInputMode(InputMode);
	}
	else
	{
		if (CapturedMousePosition)
			SetMouseLocation(CapturedMousePosition->X, CapturedMousePosition->Y);

		CapturedMousePosition.Reset();

		bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;

		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);

		SetInputMode(InputMode);
	}
}

bool ATacticsPlayerController::GetMousePositionOnViewport(FVector2D& OutMousePosition) const
{
	if (!FSlateApplication::IsInitialized())
		return false;
	
	FVector2D AbsoluteMousePosition = FSlateApplication::Get().GetCursorPos();

	FGeometry Geometry = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(const_cast<ATacticsPlayerController*>(this));
	//FGeometry Geometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(const_cast<ATacticsPlayerController*>(this));

	auto LocalMousePosition = Geometry.AbsoluteToLocal(AbsoluteMousePosition);

	LocalMousePosition /= Geometry.GetLocalSize();

	FIntPoint ViewportSize;

	GetViewportSize(ViewportSize.X, ViewportSize.Y);

	LocalMousePosition *= FVector2D{ ViewportSize };

	FBox2D Bound{FVector2D::Zero(), Geometry.GetLocalSize()};

	if (!Bound.IsInside(LocalMousePosition))
		return false;
	
	OutMousePosition = LocalMousePosition;
	
	return true;
}

void ATacticsPlayerController::BindPlayerHotkey(FPlayerHotkeyDelegate Delegate, FName InputAction)
{
	auto& Binding = PlayerHotkeyBindings.FindOrAdd(InputAction);

	Binding.AddUnique(Delegate);
	
}

void ATacticsPlayerController::UnbindPlayerHotkey(FPlayerHotkeyDelegate Delegate, FName InputAction)
{
	auto Binding = PlayerHotkeyBindings.Find(InputAction);

	if (!Binding)
		return;

	Binding->Remove(Delegate);

	if (Binding->IsBound())
		return;

	PlayerHotkeyBindings.Remove(InputAction);
}

void ATacticsPlayerController::PlayerHotkeyInputAction(FName InputAction, EInputEvent Event)
{
	auto Binding = PlayerHotkeyBindings.Find(InputAction);

	if (!Binding)
		return;
	
	Binding->Broadcast(Event);
}