//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FogOfWarCommon.h"
#include "FogOfWarVisionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFogOfWarVisibleActorsUpdated, class UFogOfWarVisionComponent*, VisionComponent, const TSet<AActor*>&, OldVisible, const TSet<AActor*>&, NewVisible);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UFogOfWarVisionComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFogOfWarVisionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Draw the vision of this component to the given canvases
	virtual void DrawVision(class UFogOfWarDisplayComponent* DisplayComponent, class FCanvas& DisplayCanvas);

	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	bool CanSeePoint(const FVector& Point) const;

	//The transform of a unit box that encloses the visible area
	FTransform GetVisionCanvasTransform(bool bForceSquareAspectRatio = true) const;

	FBox2D GetVisionCanvasBounds(bool bForceSquareAspectRatio = true) const;

	//Collision channel to test for vision relevant objects like shadow casting meshes, enemy units, items, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	TEnumAsByte<ECollisionChannel> VisionOverlapChannel = ECollisionChannel::ECC_GameTraceChannel1;

	//The radius around the component that is visible
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	double VisionRadius = 1000.0;

	//The field of view of the vision in the +X direction. 0 -> 360
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	double VisionFieldOfViewDeg = 90.0;

	//Radius of additional circle around the component itself
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	double SelfVisionRadius = 100.0;

	//World space distance between the edge of the vision cone and the canvas bounds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	double VisionCanvasPadding = 10.0;

	//Global push back applied to shadowing edges. Use small positive values to push shadows back into the object, or small negative values to bring them closer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	double GlobalShadowBias = 0.f;

	//How often the vision checks for new occluders and IFogOfWarActors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	float VisionActorsUpdateFrequency = 0.1;

	//Called when visible actors are updated
	UPROPERTY(BlueprintAssignable, Category = FogOfWar)
	FFogOfWarVisibleActorsUpdated OnVisibleActorsUpdated;

	FORCEINLINE const auto& GetVisibleActors() const { return VisibleActors; }

protected:
	
	//Draw the shadow of a static mesh component into a canvas
	virtual void DrawOccluderShadow(const FFogOfWarOccluderInstance& Occluder, class FCanvas& Canvas, FVector2D Centre, double Radius, double ShadowBias) const;

	//Helper to get half the FOV in radians while making sure it is clamped to 0 -> PI
	double GetHalfFOVInRadians() const;

	void UpdateVisionOverlaps();

	TArray<FFogOfWarOccluderInstance> OverlappingVisionOccluders;

	////The managers this component is registered with
	//UPROPERTY(Transient, BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	//class AFogOfWarManager* Manager;

	//void SetFogOfWarManager(class AFogOfWarManager* NewManager);

	//Fog of war actors that are currently visible to this component
	UPROPERTY(Transient, BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	TSet<AActor*> VisibleActors;

	void ClearVisibleActors();
};
