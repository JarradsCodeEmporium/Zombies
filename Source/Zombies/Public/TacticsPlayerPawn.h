//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags.h"
#include "WorldCursor.h"
#include "CommandCommon.h"
#include "ItemCommon.h"
#include "TacticsPlayerPawn.generated.h"

//DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FWorldCursorEventDelegate, const FWorldCursorEventParams&, Params);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGameplaySelectedDelegate, class ATacticsPlayerPawn*, Pawn, UObject*, Object, bool, bIsSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTacticsPlayerPawnGenericDelegate, class ATacticsPlayerPawn*, Pawn);



UENUM(BlueprintType)
enum class EContextActionType : uint8
{
	//There is no context action to perform.
	//This can happen if nothing is selected or clicking somewhere far off the navmesh.
	None,

	//The action wants to move to a location
	Move,

	//The action wants to interact with something
	Interact,
};

//When performing a context action trace, we could hit an interactable object, 
//or somewhere on the ground we want to move to
USTRUCT(BlueprintType)
struct ZOMBIES_API FContextActionTraceResult
{
	GENERATED_BODY()
public:

	//The type of action to perform
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	EContextActionType Type = EContextActionType::None;

	//If Type is Move, this specifies where the move should happen.
	//This location will be on the nav mesh.
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	FVector MoveLocation{};

	//If Type is Interact, this is the object to interact with
	UPROPERTY(BlueprintReadWrite, Category = WorldCursor)
	UObject* Interactable = nullptr;

};


UCLASS()
class ZOMBIES_API ATacticsPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATacticsPlayerPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	UFUNCTION(BlueprintCallable, Category = Tactics)
	void PanRight(float Value);

	UFUNCTION(BlueprintCallable, Category = Tactics)
	void PanUp(float Value);

	//Directly rotates camera a fixed amount without having to enable camera rotation mode
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void StepRotateCW();

	UFUNCTION(BlueprintCallable, Category = Tactics)
	void StepRotateCCW();

	//Continuously rotate while button is pressed
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void ContinuousRotate(float Value);

	//Input for Camera rotation, ignored if bCameraRotationMode == false
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void CameraRotate(float Value);

	//Enable rotation mode based on input
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void CameraRotationModePressed();

	//Enable rotation mode based on input
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void CameraRotationModeReleased();

	//Set whether the camera is being rotated
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void SetCameraRotationMode(bool bNewCameraRotationMode);

	//Current velocity of pan movement
	UPROPERTY(BlueprintReadWrite, Category = Tactics)
	FVector PanVelocity;

	//How fast the view starts moving when panning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	float PanAcceleration = 1000.f;

	//How fast the view pans across the landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	float PanMaxSpeed = 1000.f;

	//How fast the view moves up and down with the terrain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	float HeightInterpSpeed = 100.f;

	//Whether the camera is in rotation mode (enables mouse/control stick rotation from player controller)
	UPROPERTY(BlueprintReadWrite, Category = Tactics)
	bool bCameraRotationMode = false;

	//Whether to enable/disable on press/release or to toggle current state on press
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	bool bToggleCameraRotation = false;

	//How far each press of the step rotation function will rotate the camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	float StepRotationAngle = 22.5f;

	//How fast does the camera rotate when pressing a button for continuous rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	float ContinuousRotationSpeed = 22.5f;

	//Don't really need a movement component, movement logic is so simple we can just handle it here.
	virtual void UpdateMovement(float DeltaTime);

	//Distance to trace for world cursor based line traces
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	double WorldCursorTraceDistance = 100000.f;;

	//Selectable objects can define a generalized selection type, such as player unit, enemy unit, item, etc.
	//When a box selection is made, it contains many different types. First index is highest priority.
	//The first match in this list will filter the rest of the selected objects to match the same type.
	//E.G. if the list is PlayerUnit -> EnemyUnit -> Item and the box selection contains 3 enemies and 2 items,
	//only the enemy units will be part of the selection because they match the higher priority entry in the list.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	TArray<FGameplayTag> SelectionPriority;

	//Trace channel for selection tests
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	TEnumAsByte<ECollisionChannel> SelectionTraceChannel = ECC_Visibility;

	//Minimum size of the selection box to be considered a proper box rather than a ray cast click
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	double SelectionBoxMinSize = 10.f;

	//The material to use for post process highlights.
	//Note that this material should already be manually added to all cameras that require it.
	//This only specifies which materials on the camera should be updated to display highlights.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	class UMaterialInterface* HighlightPostProcess;

	////Determine how the selection state of the objects will change given the current state
	////@Param OldSelection: The current selection
	////@Param NewSelection: The changes to be made to the selection. Does not check for selectable, the set must be pre-filtered.
	////@Param Operation: The selection operation type to perform
	////@Param OutSelection: The current selection after the operation is applied
	////@Param OutSelected: Objects that have been newly selected
	////@Param OutDeselected: Objects that have been newly deselected
	//UFUNCTION(BlueprintCallable, Category = Tactics)
	//void CalculateGameplaySelection(const TSet<UObject*>& OldSelection, const TSet<UObject*>& NewSelection, EGameplaySelectionOp Operation, TSet<UObject*>& OutSelection, TSet<UObject*>& OutSelected, TSet<UObject*>& OutDeselected) const;

	////Change the gameplay selection according to the given op
	//UFUNCTION(BlueprintCallable, Category = Tactics)
	//void ApplyGameplaySelection(const TSet<UObject*>& NewSelection, EGameplaySelectionOp Operation);

	////Change the display of selected objects according to the given op
	//UFUNCTION(BlueprintCallable, Category = Tactics)
	//void ApplyGameplaySelectionDisplayType(const TSet<UObject*>& NewSelection, EGameplaySelectionOp Operation);

	//Determine how the selection state of the objects will change given the current state
	//@Param OldSelection: The current selection
	//@Param NewSelection: The changes to be made to the selection. Does not check for selectable, the set must be pre-filtered.
	//@Param Operation: The selection operation type to perform
	//@Param OutSelection: The current selection after the operation is applied
	//@Param OutSelected: Objects that have been newly selected
	//@Param OutDeselected: Objects that have been newly deselected
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void CalculateGameplaySelection(
		const TSet<UGameplaySelectableComponent*>& OldSelection,
		const TSet<UGameplaySelectableComponent*>& NewSelection,
		EGameplaySelectionOp Operation,
		TSet<UGameplaySelectableComponent*>& OutSelection,
		TSet<UGameplaySelectableComponent*>& OutSelected,
		TSet<UGameplaySelectableComponent*>& OutDeselected
	) const;

	//Change the gameplay selection according to the given op
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void ApplyGameplaySelection(const TSet<UGameplaySelectableComponent*>& NewSelection, EGameplaySelectionOp Operation);

	//Change the display of selected objects according to the given op
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void ApplyGameplaySelectionDisplayType(const TSet<UGameplaySelectableComponent*>& NewSelection, EGameplaySelectionOp Operation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	FGameplayTag SelectionPreviewTag;

	FORCEINLINE auto GetSelectorComponent() const { return SelectorComponent; }

	//Hovers/Unhovers gameplay selectable objects based on current selection query.
	void UpdateSelectionDisplay();



	//Called once after all selection changes have been made
	UPROPERTY(BlueprintAssignable, Category = Tactics)
	FTacticsPlayerPawnGenericDelegate OnSelectionChanged;

	UFUNCTION(BlueprintCallable, Category = Tactics)
	void SelectionPressed();

	UFUNCTION(BlueprintCallable, Category = Tactics)
	void SelectionReleased();

	//Returns all selectable objects in order of distance.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Tactics)
	TArray<UGameplaySelectableComponent*> RaySelection(const FWorldCursorLocation& Cursor) const;

	//Returns set of selectable objects in screen box
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Tactics)
	TSet<UGameplaySelectableComponent*> BoxSelection(const FBox2D& Box) const;

	//When multiple objects are under the cursor, instead of being limited to the closest object,
	//we want to be able to cycle through them all by clicking again.
	//@return: The next object that should be selected.
	UFUNCTION(BlueprintCallable, Category = Tactics)
	UGameplaySelectableComponent* GetNextRaySelection(const TSet<UGameplaySelectableComponent*>& CurrentSelection, const TArray<UGameplaySelectableComponent*>& RaySelectionResult) const;

	//Returns the set of objects inside selection box, or the single selection from a ray
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Tactics)
	TSet<UGameplaySelectableComponent*> GetSelectionQuery() const;


	//Whether any selection operations can be performed at the moment
	UFUNCTION(BlueprintCallable, Category = Tactics)
	bool CanSelect() const;

	UFUNCTION(BlueprintCallable, Category = Tactics)
	bool HasSelectionBox() const;

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE FBox2D GetSelectionBox() const;

	//Gets the current selection op depending on what modifier inputs are currently pressed
	UFUNCTION(BlueprintCallable, Category = Tactics)
	EGameplaySelectionOp GetSelectionOp() const;

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE void SelectionAddPressed() { bWantsSelectionAdd = true; };

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE void SelectionAddReleased() { bWantsSelectionAdd = false; };

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE void SelectionSubtractPressed() { bWantsSelectionSubtract = true; };

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE void SelectionSubtractReleased() { bWantsSelectionSubtract = false; };

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE bool WantsSelectionAdd() const { return bWantsSelectionAdd; };

	UFUNCTION(BlueprintCallable, Category = Tactics)
	FORCEINLINE bool WantsSelectionSubtract() const { return bWantsSelectionSubtract; };

	//Move/Attack/Interact context dependent input pressed
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void ContextActionPressed();

	//Move/Attack/Interact context dependent input released
	UFUNCTION(BlueprintCallable, Category = Tactics)
	void ContextActionReleased();

	//Trace channel for move location during ContextActionTrace().
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	TEnumAsByte<ECollisionChannel> MoveTraceChannel = ECC_Pawn;

	//Horizontal distance that a move trace can snap to the nav mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	double MoveTraceHorizontalDistance = 200.0;
	
	//Vertical distance that a move trace can snap to the nav mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	double MoveTraceVerticalDistance = 200.0;

	//Class of move command actor to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	TSubclassOf<class ACommand> MoveCommandClass;

	//Trace channel for move locations during ContextActionTrace().
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tactics)
	TEnumAsByte<ECollisionChannel> InteractableTraceChannel = ECC_Pawn;

	//Context dependent trace for IInteractable, a move location on the navmesh, or nothing if nothing is selected.
	UFUNCTION(BlueprintCallable, Category = Tactics)
	FContextActionTraceResult ContextActionTrace(const FWorldCursorLocation& Location) const;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoomComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	class UFogOfWarDisplayComponent* FogOfWarDisplayComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	class UWorldCursorComponent* WorldCursorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	class UGameplaySelectorComponent* SelectorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	class UCommanderComponent* CommanderComponent;

	//Discover or undiscover the entire fog of war area.
	UFUNCTION(Exec, Category = Tactics)
	void SetFogOfWarDiscoveredAreas(bool bIsDiscovered);



	uint32 bWantsSelectionAdd : 1;

	uint32 bWantsSelectionSubtract : 1;

	//If selection is pressed but not released, stores the current box drag in screen space
	TOptional<FVector2D> SelectionBoxStart;

	//Updates cached values and broadcasts location event. Call after movement update but before anything else would want to use it
	void UpdateWorldCursorLocation();



	UPROPERTY(BlueprintReadOnly, Category = Tactics, Meta = (AllowPrivateAccess = "true"))
	FWorldCursorLocation WorldCursorLocation;

	FWorldCursorEventDelegate WorldCursorHandler;

	bool bHandlerCapturedSelectionPressed;

	bool bHandlerCapturedContextActionPressed;

	////Handler for when nothing consumes an event. Starts a SelectHandler() or ContextActionHandler()
	//bool DefaultWorldCursorEventHandler(const FWorldCursorEventParams& Params);

	////Handler for default select
	UFUNCTION()
	EWorldCursorEventResult SelectHandler(const FWorldCursorEventParams& Params);

	//FWorldCursorDelegateHandle SelectHandle;

	UFUNCTION()
	EWorldCursorEventResult ContextActionHandler(const FWorldCursorEventParams& Params);

	//FWorldCursorDelegateHandle ContextActionHandle;

	UFUNCTION()
	EWorldCursorEventResult DefaultCursorHandler(const FWorldCursorEventParams& Params);

	FWorldCursorDelegateHandle DefaultCursorHandle;

	UFUNCTION()
	FBeginCommandParams GetBeginCommandParams(ACommand* Command);

	////Handler for default context action
	//UFUNCTION()
	//bool ContextActionHandler(const FWorldCursorEventParams& Params);

	
};
