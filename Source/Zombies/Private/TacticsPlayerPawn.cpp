//Copyright Jarrad Alexander 2022


#include "TacticsPlayerPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplaySelectable.h"
#include "TacticsPlayerController.h"
#include "FogOfWarDisplayComponent.h"
#include "EngineUtils.h"
#include "HighlightingCommon.h"
#include "UnitInterface.h"
#include "Interactable.h"
#include "NavigationSystem.h"
#include "PooledActorSubsystem.h"
#include "Command.h"
#include "ZombiesGameInstance.h"
#include "GameplaySelectorComponent.h"
#include "GameplaySelectableComponent.h"
#include "CommanderComponent.h"
#include "CommandFollowerComponent.h"
#include "WorldCursorComponent.h"


ATacticsPlayerPawn::ATacticsPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	CameraBoomComponent = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoomComponent->SetupAttachment(GetRootComponent());
	CameraBoomComponent->SetRelativeRotation(FRotator{-60.0, 0.0, 0.0});
	CameraBoomComponent->TargetArmLength = 1000.f;
	CameraBoomComponent->bUsePawnControlRotation = true;


	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera");
	CameraComponent->SetupAttachment(CameraBoomComponent, CameraBoomComponent->SocketName);
	CameraComponent->bUsePawnControlRotation = false;

	FogOfWarDisplayComponent = CreateDefaultSubobject<UFogOfWarDisplayComponent>("FogOfWarDisplay");
	FogOfWarDisplayComponent->SetupAttachment(CameraComponent);
	FogOfWarDisplayComponent->SetRelativeLocation(FVector{ CameraBoomComponent->TargetArmLength, 0.0, 0.0 });

	WorldCursorComponent = CreateDefaultSubobject<UWorldCursorComponent>("WorldCursorComponent");

	SelectorComponent = CreateDefaultSubobject<UGameplaySelectorComponent>("SelectorComponent");

	CommanderComponent = CreateDefaultSubobject<UCommanderComponent>("CommanderComponent");


}

void ATacticsPlayerPawn::BeginPlay()
{
	if (auto GI = GetGameInstance<UZombiesGameInstance>())
		GI->UpdateAllPostProcessMaterials(this, HighlightPostProcess);

	{
		FWorldCursorEventDelegate Delegate;

		Delegate.BindDynamic(this, &ATacticsPlayerPawn::DefaultCursorHandler);

		DefaultCursorHandle = WorldCursorComponent->AddDelegate(Delegate);
	}

	{
		FGetBeginCommandParamsDelegate Delegate;

		Delegate.BindDynamic(this, &ATacticsPlayerPawn::GetBeginCommandParams);

		CommanderComponent->SetBeginCommandParamsDelegate(Delegate);
	}

	Super::BeginPlay();
}

void ATacticsPlayerPawn::Tick(float DeltaTime)
{
	UpdateMovement(DeltaTime);

	UpdateWorldCursorLocation();


	//BP ticks inside here, so make sure we're updated before passing it on to BP
	Super::Tick(DeltaTime);
}

void ATacticsPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("PanRight", this, &ATacticsPlayerPawn::PanRight);
	PlayerInputComponent->BindAxis("PanUp", this, &ATacticsPlayerPawn::PanUp);

	PlayerInputComponent->BindAction("StepRotateCW", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::StepRotateCW);
	PlayerInputComponent->BindAction("StepRotateCCW", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::StepRotateCCW);

	PlayerInputComponent->BindAxis("ContinuousRotate", this, &ATacticsPlayerPawn::ContinuousRotate);

	PlayerInputComponent->BindAction("CameraRotationMode", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::CameraRotationModePressed);
	PlayerInputComponent->BindAction("CameraRotationMode", EInputEvent::IE_Released, this, &ATacticsPlayerPawn::CameraRotationModeReleased);

	PlayerInputComponent->BindAxis("CameraRotate", this, &ATacticsPlayerPawn::CameraRotate);
	

	PlayerInputComponent->BindAction("Select", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::SelectionPressed);
	PlayerInputComponent->BindAction("Select", EInputEvent::IE_Released, this, &ATacticsPlayerPawn::SelectionReleased);

	PlayerInputComponent->BindAction("SelectionAdd", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::SelectionAddPressed);
	PlayerInputComponent->BindAction("SelectionAdd", EInputEvent::IE_Released, this, &ATacticsPlayerPawn::SelectionAddReleased);

	PlayerInputComponent->BindAction("SelectionSubtract", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::SelectionSubtractPressed);
	PlayerInputComponent->BindAction("SelectionSubtract", EInputEvent::IE_Released, this, &ATacticsPlayerPawn::SelectionSubtractReleased);

	PlayerInputComponent->BindAction("ContextAction", EInputEvent::IE_Pressed, this, &ATacticsPlayerPawn::ContextActionPressed);
	PlayerInputComponent->BindAction("ContextAction", EInputEvent::IE_Released, this, &ATacticsPlayerPawn::ContextActionReleased);
}


void ATacticsPlayerPawn::PanRight(float Value)
{
	AddMovementInput(CameraComponent->GetRightVector().GetSafeNormal2D() * Value);
}

void ATacticsPlayerPawn::PanUp(float Value)
{
	AddMovementInput(CameraComponent->GetForwardVector().GetSafeNormal2D() * Value);
}

void ATacticsPlayerPawn::StepRotateCW()
{
	auto PC = Cast<APlayerController>(GetController());

	if (!PC)
		return;

	PC->SetControlRotation(PC->GetControlRotation() + FRotator{ 0.f, StepRotationAngle, 0.f });

}

void ATacticsPlayerPawn::StepRotateCCW()
{
	auto PC = Cast<APlayerController>(GetController());

	if (!PC)
		return;

	PC->SetControlRotation(PC->GetControlRotation() + FRotator{ 0.f, -StepRotationAngle, 0.f });
}

void ATacticsPlayerPawn::ContinuousRotate(float Value)
{
	AddControllerYawInput(GetWorld()->GetDeltaSeconds() * Value * ContinuousRotationSpeed);
}

void ATacticsPlayerPawn::CameraRotate(float Value)
{
	if (!bCameraRotationMode)
		return;

	AddControllerYawInput(Value);
}

void ATacticsPlayerPawn::CameraRotationModePressed()
{
	if (bToggleCameraRotation)
		SetCameraRotationMode(!bCameraRotationMode);
	else
		SetCameraRotationMode(true);
	
}

void ATacticsPlayerPawn::CameraRotationModeReleased()
{
	if (!bToggleCameraRotation)
		SetCameraRotationMode(false);
}

void ATacticsPlayerPawn::SetCameraRotationMode(bool bNewCameraRotationMode)
{
	if (bNewCameraRotationMode == bCameraRotationMode)
		return;

	bCameraRotationMode = bNewCameraRotationMode;

	if (auto PC = Cast<ATacticsPlayerController>(GetController()))
		PC->SetCameraControlMode(bCameraRotationMode);
	
}


void ATacticsPlayerPawn::UpdateMovement(float DeltaTime)
{
	
	//Prevent numerical drift of root roll and pitch as player rotates around yaw (since internally represented as quat, there could be drift while converting)
	RootComponent->SetWorldRotation(FRotator{ 0.f, RootComponent->GetComponentRotation().Yaw, 0.f });

	auto MoveInput = ConsumeMovementInputVector();

	MoveInput.Z = 0.f;

	MoveInput = MoveInput.GetClampedToMaxSize(1.f);

	//RootComponent->AddWorldOffset(MoveInput * (PanSpeed * DeltaTime));

	if ((MoveInput | PanVelocity) < 0.f)
		//boost move input if in opposite direction to allow snappy direction change
		MoveInput *= 2.f;

	if (MoveInput.IsNearlyZero())
	{
		//apply stopping/drag force equivalent to reverse of pan acceleration
		auto DragForce = -PanVelocity.GetSafeNormal() * PanAcceleration * DeltaTime;

		if (DragForce.SizeSquared() < PanVelocity.SizeSquared())
			PanVelocity += DragForce;
		else
			//don't overshoot and oscillate around zero
			PanVelocity = FVector::ZeroVector;
	}
	else
		//Accelerate pan velocity according to input
		PanVelocity += MoveInput * PanAcceleration * DeltaTime;

	PanVelocity = PanVelocity.GetClampedToMaxSize(PanMaxSpeed);

	RootComponent->AddWorldOffset(PanVelocity * DeltaTime);

	float AverageZ = 0.f;

	int32 SampleCount = 0;

	auto GroundTrace = [&](const FVector& Location)
	{

		FHitResult GroundHit;

		FCollisionQueryParams CQP;

		CQP.AddIgnoredActor(this);

		//FCollisionResponseParams CRP;

		//Find ground by tracing down from way above us
		//could probably change this to a rather large sphere to make it smoother
		if (!GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			Location + FVector{ 0.f,0.f, 20000.f },
			Location + FVector{ 0.f,0.f, -20000.f },
			ECollisionChannel::ECC_Camera,
			CQP
		))
			//no hit, don't snap to ground
			return;

		//DrawDebugDirectionalArrow(GetWorld(), GroundHit.TraceStart, GroundHit.TraceEnd, 10.f, FColor::Red, false, DeltaTime * 1.1f);

		AverageZ += GroundHit.Location.Z;
		++SampleCount;

	};

	auto Location = RootComponent->GetComponentLocation();

	//@todo: currently just use constant height at spawn. if we add multiple height levels to the game world, then we could adjust this
	//auto Radius = CameraBoomComponent->TargetArmLength * 0.4f;
	////*.99 so that numeric inaccuracy won't cut off the last iteration
	//auto Step = Radius * 0.99f;

	//for (float Y = -Radius; Y <= Radius; Y += Step)
	//	for (float X = -Radius; X <= Radius; X += Step)
	//		GroundTrace(Location + FVector{ X, Y, 0.f });


	//Location.Z = AverageZ / SampleCount;

	
	RootComponent->SetWorldLocation(FMath::VInterpTo(RootComponent->GetComponentLocation(), Location, DeltaTime, HeightInterpSpeed));

}



void ATacticsPlayerPawn::CalculateGameplaySelection(
	const TSet<UGameplaySelectableComponent*>& OldSelection,
	const TSet<UGameplaySelectableComponent*>& NewSelection,
	EGameplaySelectionOp Operation,
	TSet<UGameplaySelectableComponent*>& OutSelection,
	TSet<UGameplaySelectableComponent*>& OutSelected,
	TSet<UGameplaySelectableComponent*>& OutDeselected
) const
{
	switch (Operation)
	{
	case EGameplaySelectionOp::Replace:
	{
		OutSelected = NewSelection.Difference(OldSelection);
		OutDeselected = OldSelection.Difference(NewSelection);
		OutSelection = NewSelection;
		break;
	}
	case EGameplaySelectionOp::Add:
	{
		OutSelected = NewSelection.Difference(OldSelection);
		OutDeselected.Reset();
		OutSelection = OldSelection.Union(NewSelection);
		break;
	}
	case EGameplaySelectionOp::Subtract:
	{
		OutDeselected = OldSelection.Intersect(NewSelection);
		OutSelection = OldSelection.Difference(NewSelection);
		OutSelected.Reset();
		break;
	}
	}
}

void ATacticsPlayerPawn::ApplyGameplaySelection(const TSet<UGameplaySelectableComponent*>& NewSelection, EGameplaySelectionOp Operation)
{
	
	TSet<UGameplaySelectableComponent*> Selection;

	TSet<UGameplaySelectableComponent*> Selected;
	TSet<UGameplaySelectableComponent*> Deselected;

	CalculateGameplaySelection(SelectorComponent->GetSelectedComponents(), NewSelection, Operation, Selection, Selected, Deselected);
	
	for (auto Component : Selected)
		SelectorComponent->SetIsSelected(Component, true);
	

	for (auto Component : Deselected)
		SelectorComponent->SetIsSelected(Component, false);
	
	//@todo: could possibly move batch select operations to the selector itself.

	OnSelectionChanged.Broadcast(this);
}

void ATacticsPlayerPawn::ApplyGameplaySelectionDisplayType(const TSet<UGameplaySelectableComponent*>& NewSelection, EGameplaySelectionOp Operation)
{
	SelectorComponent->ClearDisplayTypeOverridesForTag(SelectionPreviewTag);

	TSet<UGameplaySelectableComponent*> Selection;

	TSet<UGameplaySelectableComponent*> Selected;
	TSet<UGameplaySelectableComponent*> Deselected;

	CalculateGameplaySelection(SelectorComponent->GetSelectedComponents(), NewSelection, Operation, Selection, Selected, Deselected);

	for (auto Component : Selected)
		SelectorComponent->SetDisplayTypeOverride(Component, SelectionPreviewTag, EGameplaySelectionDisplayType::Highlighted);

	for (auto Component : Selection)
	{
		if (!NewSelection.Contains(Component))
			continue;
		
		//Also highlight anything that was already selected, such as already selected units inside a box selection
		SelectorComponent->SetDisplayTypeOverride(Component, SelectionPreviewTag, EGameplaySelectionDisplayType::Highlighted);

	}

	for (auto Component : Deselected)
		SelectorComponent->SetDisplayTypeOverride(Component, SelectionPreviewTag, EGameplaySelectionDisplayType::None);
	

}

void ATacticsPlayerPawn::UpdateSelectionDisplay()
{
	//Box select is a preview of what would happen if mouse is let go, so execute the apply display with the actual op
	//otherwise execute ray select with add op, if nothing hit then just displays selection

	ApplyGameplaySelectionDisplayType(GetSelectionQuery(), HasSelectionBox() ? GetSelectionOp() : EGameplaySelectionOp::Add);

}



void ATacticsPlayerPawn::SelectionPressed()
{
	WorldCursorComponent->SelectionPressed();
	//ExecuteWorldCursorEvent(MakeWorldCursorEventParams(EWorldCursorEventType::Selection, EInputEvent::IE_Pressed));
}

void ATacticsPlayerPawn::SelectionReleased()
{
	WorldCursorComponent->SelectionReleased();
	//ExecuteWorldCursorEvent(MakeWorldCursorEventParams(EWorldCursorEventType::Selection, EInputEvent::IE_Released));
}


TArray<UGameplaySelectableComponent*> ATacticsPlayerPawn::RaySelection(const FWorldCursorLocation& Cursor) const
{
	TArray<UGameplaySelectableComponent*> Result;

	check(GetWorld());

	TArray<FHitResult> Hits;

	FCollisionQueryParams CQP;

	CQP.AddIgnoredActor(this);

	GetWorld()->LineTraceMultiByChannel(Hits, Cursor.WorldLocation, Cursor.WorldLocation + Cursor.WorldDirection * WorldCursorTraceDistance, SelectionTraceChannel, CQP);

	for (auto& Hit : Hits)
	{
		auto Actor = Hit.GetActor();

		if (!Actor)
			continue;

		if (!Actor->Implements<UGameplaySelectable>())
			continue;

		auto Component = IGameplaySelectable::Execute_GetSelectableComponent(Actor);

		if (Result.Contains(Component))
			//Hit same actor twice, don't count it again
			continue;

		if (!Component)
			continue;

		if (!Component->CanBeSelectedBy(SelectorComponent))
			continue;

		Result.Add(Component);
	}

	return Result;
}

TSet<UGameplaySelectableComponent*> ATacticsPlayerPawn::BoxSelection(const FBox2D& Box) const
{
	TSet<UGameplaySelectableComponent*> Result;

	auto PC = GetController<ATacticsPlayerController>();

	if (!PC)
		return Result;

	//Modified version of AHUD::GetActorsInSelectionRectangle()

	//The Actor Bounds Point Mapping
	const FVector BoundsPointMapping[8] =
	{
		FVector(1.f, 1.f, 1.f),
		FVector(1.f, 1.f, -1.f),
		FVector(1.f, -1.f, 1.f),
		FVector(1.f, -1.f, -1.f),
		FVector(-1.f, 1.f, 1.f),
		FVector(-1.f, 1.f, -1.f),
		FVector(-1.f, -1.f, 1.f),
		FVector(-1.f, -1.f, -1.f)
	};

	int32 MaxPriorityIndex = SelectionPriority.Num() - 1;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		auto Actor = *It;

		if (!Actor->Implements<UGameplaySelectable>())
			continue;

		auto Component = IGameplaySelectable::Execute_GetSelectableComponent(Actor);

		if (!Component)
			continue;

		if (!Component->CanBeSelectedBy(SelectorComponent))
			continue;

		auto Rule = Component->BoxRule;

		auto Bounds = Actor->GetComponentsBoundingBox(false);

		auto Center = Bounds.GetCenter();

		auto Extent = Bounds.GetExtent();

		//Attempt to reject this actor based on its box selection rule

		if (Rule == EGameplaySelectionBoxRule::CenterPoint)
		{
			//Center point selection
			FVector2D PointScreen{};

			if (!PC->ProjectWorldLocationToScreen(Center, PointScreen, false))
				continue;

			//DrawDebugPoint(GetWorld(), Center, 10.f, FColor::Blue, false, 0.05f, 255);

			if (!Box.IsInside(PointScreen))
				continue;
		}
		else if (Rule == EGameplaySelectionBoxRule::FullBounds || Rule == EGameplaySelectionBoxRule::AnyBounds)
		{
			//Full box test, doesn't seem to feel as nice as center point selection though
			FBox2D BoundsScreen(ForceInit);

			//DrawDebugBox(GetWorld(), Center, Extent, FColor::Blue, false, 0.01f);

			for (int32 BoundsPointIndex = 0; BoundsPointIndex < 8; BoundsPointIndex++)
			{
				FVector PointWorld = Center + (BoundsPointMapping[BoundsPointIndex] * Extent);

				FVector2D PointScreen{};

				if (!PC->ProjectWorldLocationToScreen(PointWorld, PointScreen, false))
					continue;

				BoundsScreen += PointScreen;
			}

			if (!BoundsScreen.bIsValid)
				continue;

			if (Rule == EGameplaySelectionBoxRule::FullBounds && !Box.IsInside(BoundsScreen))
				continue;

			if (Rule == EGameplaySelectionBoxRule::AnyBounds && !Box.Intersect(BoundsScreen))
				continue;
		}
		else
			continue;

		if (SelectionPriority.Num() > 0)
		{
			//We have a priority list, so we need to check how this actor modifies the selection
			//The way this works is that we know we want to only ever allow objects that match the single highest priority level that appears in the selection box
			//So if the object is lower priority than one we've already seen, we can easily reject it
			//and if this object is higher priority than any we've seen, we know everything we've already added will be rejected so just empty the result

			bool bRejectActor = true;

			for (int32 i = 0; i <= MaxPriorityIndex; ++i)
			{
				if (!Component->SelectionTag.MatchesTag(SelectionPriority[i]))
					continue;

				bRejectActor = false;

				if (i < MaxPriorityIndex)
					Result.Reset();

				MaxPriorityIndex = i;

				break;
			}

			if (bRejectActor)
				continue;
		}

		Result.Add(Component);
	}

	return Result;
}

UGameplaySelectableComponent* ATacticsPlayerPawn::GetNextRaySelection(const TSet<UGameplaySelectableComponent*>& CurrentSelection, const TArray<UGameplaySelectableComponent*>& RaySelectionResult) const
{
	UGameplaySelectableComponent* Selection = nullptr;

	//Default to selecting the closest
	if (RaySelectionResult.Num() > 0)
		Selection = RaySelectionResult[0];

	//If theres already something selected in this trace, cycle to the next furthest one
	for (int32 i = 0; i < RaySelectionResult.Num(); ++i)
		if (CurrentSelection.Contains(RaySelectionResult[i]))
			Selection = RaySelectionResult[(i + 1) % RaySelectionResult.Num()];

	return Selection;
	
}

TSet<UGameplaySelectableComponent*> ATacticsPlayerPawn::GetSelectionQuery() const
{
	if (HasSelectionBox())
		return BoxSelection(GetSelectionBox());
	else if (auto Object = GetNextRaySelection(SelectorComponent->GetSelectedComponents(), RaySelection(WorldCursorLocation)))
		return { Object };
	
	return {};
}

bool ATacticsPlayerPawn::CanSelect() const
{
	//@todo: this should probably be based on whether the player controller says the cursor is visible
	return !bCameraRotationMode;
}

bool ATacticsPlayerPawn::HasSelectionBox() const
{
	if (!CanSelect())
		return false;

	if (!SelectionBoxStart.IsSet())
		return false;

	if (FVector2D::DistSquared(*SelectionBoxStart, WorldCursorLocation.ScreenLocation) < SelectionBoxMinSize * SelectionBoxMinSize)
		//There is technically a box, but it is too small to be considered different from a ray selection.
		return false;

	return true;
}

FBox2D ATacticsPlayerPawn::GetSelectionBox() const
{
	FBox2D Result{ForceInit};

	if (!HasSelectionBox())
		return Result;

	Result += *SelectionBoxStart;

	Result += WorldCursorLocation.ScreenLocation;

	return Result;
}


EGameplaySelectionOp ATacticsPlayerPawn::GetSelectionOp() const
{
	//Defines a priority when multiple keys pressed at the same time, and avoids problems with interleaved press/release.

	if (bWantsSelectionAdd)
		return EGameplaySelectionOp::Add;
	
	if (bWantsSelectionSubtract)
		return EGameplaySelectionOp::Subtract;

	return EGameplaySelectionOp::Replace;
}

void ATacticsPlayerPawn::ContextActionPressed()
{
	WorldCursorComponent->ContextActionPressed();
	//ExecuteWorldCursorEvent(MakeWorldCursorEventParams(EWorldCursorEventType::ContextAction, EInputEvent::IE_Pressed));
}

void ATacticsPlayerPawn::ContextActionReleased()
{
	WorldCursorComponent->ContextActionReleased();

	//ExecuteWorldCursorEvent(MakeWorldCursorEventParams(EWorldCursorEventType::ContextAction, EInputEvent::IE_Released));
}

FContextActionTraceResult ATacticsPlayerPawn::ContextActionTrace(const FWorldCursorLocation& Location) const
{
	FContextActionTraceResult Result;

	check(GetWorld());

	FCollisionQueryParams CQP;

	CQP.AddIgnoredActor(this);

	{
		TArray<FHitResult> Interactables;

		GetWorld()->LineTraceMultiByChannel(Interactables, Location.WorldLocation, Location.WorldLocation + Location.WorldDirection * WorldCursorTraceDistance, InteractableTraceChannel, CQP);

		for (auto& Hit : Interactables)
		{
			auto Actor = Hit.GetActor();

			if (!Actor || !Actor->Implements<UInteractable>())
				continue;

			if (!IInteractable::Execute_CanInteract(Actor, this))
				continue;

			Result.Type = EContextActionType::Interact;

			Result.Interactable = Actor;

			break;
		}
	}

	if (Result.Type != EContextActionType::None)
		//Found an interactable, choose it as the result.
		return Result;

	//No interactables, try find a location to move to

	FHitResult Hit;

	if (!GetWorld()->LineTraceSingleByChannel(Hit, Location.WorldLocation, Location.WorldLocation + Location.WorldDirection * WorldCursorTraceDistance, MoveTraceChannel, CQP))
		return Result;

	auto NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());

	if (!NavSystem)
		return Result;

	FNavLocation NavLocation;

	if (!NavSystem->ProjectPointToNavigation(Hit.Location, NavLocation, FVector{ MoveTraceHorizontalDistance, MoveTraceHorizontalDistance, MoveTraceVerticalDistance }))
		return Result;

	Result.Type = EContextActionType::Move;

	Result.MoveLocation = NavLocation.Location;

	return Result;
}
//
//void ATacticsPlayerPawn::SetWorldCursorEventHandler(const FWorldCursorEventDelegate& Delegate)
//{
//	if (!Delegate.IsBound())
//		return;
//	
//	bHandlerCapturedSelectionPressed = false;
//
//	bHandlerCapturedContextActionPressed = false;
//
//	WorldCursorHandler = Delegate;
//
//}
//
//void ATacticsPlayerPawn::ClearWorldCursorEventHandler()
//{
//	bHandlerCapturedSelectionPressed = false;
//
//	bHandlerCapturedContextActionPressed = false;
//
//	WorldCursorHandler.Clear();
//}
//
//void ATacticsPlayerPawn::ExecuteWorldCursorEvent(const FWorldCursorEventParams& Params)
//{
//	if (CommanderComponent->HandleWorldCursor(Params))
//		return;
//
//	if (SelectorComponent->HandleWorldCursor(Params))
//		return;
//
//	//Commander and selector did not handle the event, so do default behaviour
//
//	if (Params.Type != EWorldCursorEventType::ContextAction)
//		return;
//
//	////Dummy value
//	//bool bIgnore = false;
//
//	////Points to the appropriate handler pressed capture bool
//	//bool* bCaptured = &bIgnore;
//
//	//if (Params.Type == EWorldCursorEventType::Selection)
//	//	bCaptured = &bHandlerCapturedSelectionPressed;
//
//	//if (Params.Type == EWorldCursorEventType::ContextAction)
//	//	bCaptured = &bHandlerCapturedContextActionPressed;
//
//	//if (Params.Input == EInputEvent::IE_Released)
//	//{
//	//	if (!(*bCaptured))
//	//		//The handler did not capture the pressed event so we cannot route the released event to it.
//	//		return;
//
//	//	//Release events always clear the capture state whether the handler responds to it or not.
//	//	*bCaptured = false;
//	//}
//
//	////Take a copy to allow the handler to unbind itself and/or bind a new one
//	//auto Handler = WorldCursorHandler;
//
//	//bool bHandled = false;
//
//	//if (Handler.IsBound())
//	//{
//	//	bHandled = Handler.Execute(Params);
//
//	//	if (Params.Input == EInputEvent::IE_Pressed)
//	//		//The handler captured the press event, so we can route the next release to it
//	//		*bCaptured = bHandled;
//	//}
//
//	//if (!bHandled)
//	//	DefaultWorldCursorEventHandler(Params);
//}
//
//FWorldCursorEventParams ATacticsPlayerPawn::MakeWorldCursorEventParams(EWorldCursorEventType Type, EInputEvent Input)
//{
//	FWorldCursorEventParams Params;
//
//	Params.Pawn = this;
//
//	Params.Type = Type;
//
//	Params.Input = Input;
//
//	Params.Cursor = WorldCursorLocation;
//
//	return Params;
//}

void ATacticsPlayerPawn::SetFogOfWarDiscoveredAreas(bool bIsDiscovered)
{
	if (!FogOfWarDisplayComponent)
		return;

	FogOfWarDisplayComponent->bForceAllDiscovered = bIsDiscovered;
}

void ATacticsPlayerPawn::UpdateWorldCursorLocation()
{
	auto PC = GetController<ATacticsPlayerController>();

	if (!PC)
		return;

	PC->GetMousePositionOnViewport(WorldCursorLocation.ScreenLocation);

	PC->DeprojectScreenPositionToWorld(WorldCursorLocation.ScreenLocation.X, WorldCursorLocation.ScreenLocation.Y, WorldCursorLocation.WorldLocation, WorldCursorLocation.WorldDirection);

	WorldCursorComponent->CursorLocationUpdated(WorldCursorLocation);
	//ExecuteWorldCursorEvent(MakeWorldCursorEventParams(EWorldCursorEventType::Location, EInputEvent::IE_Axis));
}

EWorldCursorEventResult ATacticsPlayerPawn::SelectHandler(const FWorldCursorEventParams& Params)
{
	//UE_LOG(LogTemp, Warning, TEXT("Select handler: %s %s"), *StaticEnum<EWorldCursorEventType>()->GetNameStringByValue((int64)Params.Type), *StaticEnum<EInputEvent>()->GetNameStringByValue(Params.Input));

	switch (Params.Type)
	{
	case EWorldCursorEventType::Location:
	{
		if (Params.Input == EInputEvent::IE_Axis)
			UpdateSelectionDisplay();
		else if (Params.Input == EInputEvent::IE_Released)
			SelectorComponent->ClearDisplayTypeOverridesForTag(SelectionPreviewTag);

		return EWorldCursorEventResult::Captured;
	}
	case EWorldCursorEventType::Selection:
	{
		if (Params.Input == EInputEvent::IE_Pressed)
		{
			SelectionBoxStart.Emplace(Params.Location.ScreenLocation);
		}
		else if (Params.Input == EInputEvent::IE_Released)
		{
			ApplyGameplaySelection(GetSelectionQuery(), GetSelectionOp());

			SelectionBoxStart.Reset();

			Params.Handle.GetCursor()->RemoveDelegate(Params.Handle);
		}

		return EWorldCursorEventResult::Captured;
	}
	case EWorldCursorEventType::ContextAction:

		SelectionBoxStart.Reset();

		Params.Handle.GetCursor()->RemoveDelegate(Params.Handle);

		return EWorldCursorEventResult::Captured;
	default:
		return EWorldCursorEventResult::Ignore;
	}

	return EWorldCursorEventResult::Ignore;
}

EWorldCursorEventResult ATacticsPlayerPawn::ContextActionHandler(const FWorldCursorEventParams& Params)
{
	if (Params.Type != EWorldCursorEventType::Location)
	{
		UE_LOG(LogTemp, Warning, TEXT("ContextAction handler: %s %s"), *StaticEnum<EWorldCursorEventType>()->GetNameStringByValue((int64)Params.Type), *StaticEnum<EInputEvent>()->GetNameStringByValue(Params.Input));
	}

	switch (Params.Type)
	{
	case EWorldCursorEventType::Location:
	{
		return EWorldCursorEventResult::Ignore;
	}
	case EWorldCursorEventType::Selection:
	{
		if (Params.Input == IE_Pressed)
			Params.Cursor->RemoveDelegate(Params.Handle);

		return EWorldCursorEventResult::Captured;
	}
	case EWorldCursorEventType::ContextAction:
	{
		auto Trace = ContextActionTrace(WorldCursorLocation);

		if (Params.Input == EInputEvent::IE_Pressed && Trace.Type == EContextActionType::Move)
		{
			Params.Cursor->RemoveDelegate(Params.Handle);

			if (MoveCommandClass.Get() && SelectorComponent->IsAnySelected())
			{
				FTransform Transform(GetActorQuat(), Trace.MoveLocation);

				CommanderComponent->SpawnCommand(Transform, MoveCommandClass);
			}

			return EWorldCursorEventResult::Handled;

		}
		else if (Params.Input == EInputEvent::IE_Released)
		{
			Params.Cursor->RemoveDelegate(Params.Handle);

			if (Trace.Type == EContextActionType::Interact)
			{
				IInteractable::Execute_Interact(Trace.Interactable, this);

				return EWorldCursorEventResult::Handled;
			}
		}

		return EWorldCursorEventResult::Captured;
	}
	default:
		return EWorldCursorEventResult::Ignore;
	}
}

EWorldCursorEventResult ATacticsPlayerPawn::DefaultCursorHandler(const FWorldCursorEventParams& Params)
{
	//UE_LOG(LogTemp, Warning, TEXT("Default handler: %s %s"), *StaticEnum<EWorldCursorEventType>()->GetNameStringByValue((int64)Params.Type), *StaticEnum<EInputEvent>()->GetNameStringByValue(Params.Input));

	switch (Params.Type)
	{
	case EWorldCursorEventType::Location:
	{
		//Default to capturing location for selection highlights when nothing is happening
		if (Params.Input == EInputEvent::IE_Axis)
			UpdateSelectionDisplay();
		else if (Params.Input == EInputEvent::IE_Released)
			SelectorComponent->ClearDisplayTypeOverridesForTag(SelectionPreviewTag);

		return EWorldCursorEventResult::Captured;
	}
	case EWorldCursorEventType::Selection:
	{
		if (Params.Input == IE_Pressed)
		{
			FWorldCursorEventDelegate Delegate;

			Delegate.BindDynamic(this, &ATacticsPlayerPawn::SelectHandler);

			Params.Handle.GetCursor()->AddDelegate(Delegate);

			return EWorldCursorEventResult::Handled;
		}

		return EWorldCursorEventResult::Ignore;
	}
	case EWorldCursorEventType::ContextAction:
	{
		if (Params.Input == IE_Pressed)
		{
			FWorldCursorEventDelegate Delegate;

			Delegate.BindDynamic(this, &ATacticsPlayerPawn::ContextActionHandler);

			Params.Handle.GetCursor()->AddDelegate(Delegate);

			return EWorldCursorEventResult::Handled;
		}

		return EWorldCursorEventResult::Ignore;
	}
	default:
		return EWorldCursorEventResult::Ignore;
	}
}

FBeginCommandParams ATacticsPlayerPawn::GetBeginCommandParams(ACommand* Command)
{
	FBeginCommandParams Result;

	Result.Behavior = bWantsSelectionAdd ? ECommandQueueBehavior::Append : ECommandQueueBehavior::Replace;

	for (auto Selectable : SelectorComponent->GetSelectedComponents())
		if (IsValid(Selectable))
			if (auto Follower = Selectable->GetOwner()->FindComponentByClass<UCommandFollowerComponent>(); IsValid(Follower))
				Result.Followers.Add(Follower);

	return Result;
}


/*

float3 Result;
uint Type = (uint(StencilValue.x) - 1) % 3;
Result.x = Type == 0;
Result.y = Type == 1;
Result.z = Type == 2;
return Result;



float2 Result;

Result.x = (uint(StencilValue.x) & 0xF) / 16.0;
Result.y = (uint(StencilValue.x) >> 4) / 16.0;

return Result;



float Kernel[3][3] = 
{{-1, 0, 1},
{-2, 0, 2},
{-1, 0, 1}};

int TexIndex = PPI_CustomStencil;
int KernelSize = 3;
float2 Mag = {0.0, 0.0};

TexCoords *= 0.5;
InvSize *= OutlineThickness;

for (int i = 0; i < KernelSize; ++i) 
{
	int offsetI = -(KernelSize / 2) + i;
	float v = TexCoords.y + offsetI * InvSize.y;
	int temp = i * KernelSize;

	for (int j = 0; j < KernelSize; ++j) 
	{
		int offsetJ = -(KernelSize / 2) + j;
		float u = TexCoords.x + offsetJ * InvSize.x;
		float2 TexCoordsShifted = TexCoords + float2(u, v);
		int Sample = SceneTextureLookup(ClampSceneTextureUV(ViewportUVToSceneTextureUV(DERIV_BASE_VALUE(TexCoordsShifted), TexIndex), TexIndex), TexIndex, false);
		
		if (i == 1 && j == 1 && Sample < StencilOutlineCutoff)
			return 0.0;

		Mag.x += (Sample < StencilOutlineCutoff) ? 0 : Kernel[i][j];
		Mag.y += (Sample < StencilOutlineCutoff) ? 0 : Kernel[j][i];
		

	}
}

return saturate(Mag.x * Mag.x + Mag.y * Mag.y);


int ColorIndex = (int(StencilValue.x) - 1) % 63;
int Type = (int(StencilValue.x) - 1) / 63;

float4 Color =
ColorIndex == 0 ? Color0 :
ColorIndex == 1 ? Color1 :
ColorIndex == 2 ? Color2 :
ColorIndex == 3 ? Color3 :
ColorIndex == 63 ? float4(0.0,0.0,.1.0,1.0) :
float4(0.0,0.0,0.0,0.0);

float4 ModifiedColor =
Type == 0 ? Color * XRay :
Type == 1 ? Color * Selected :
Type == 2 ? Color * Highlighted :
float4(1.0, 0.0, 0.0, 1.0);

return StencilValue == 0 ? 0.0 : ModifiedColor;

*/


