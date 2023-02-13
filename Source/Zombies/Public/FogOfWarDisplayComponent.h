//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Templates/SharedPointer.h" 
#include "FogOfWarDisplayComponent.generated.h"

USTRUCT()
struct ZOMBIES_API FDiscoveredAreasCell
{
	GENERATED_BODY()
public:

	//The render target for this cell
	UPROPERTY()
	class UTextureRenderTarget2D* RenderTarget = nullptr;

	//If set, is a canvas that is currently drawing on the render target.
	TSharedPtr<FCanvas> Canvas;
};

/**
 * An axis aligned box that renders fog of war within its boundaries, then sends the texture to any camera post process materials on the same actor.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIES_API UFogOfWarDisplayComponent : public UBoxComponent
{
	GENERATED_BODY()
public:

	UFogOfWarDisplayComponent();

protected:

	virtual void BeginPlay() override;

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void UpdateFogOfWar(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	void AddVisionComponent(class UFogOfWarVisionComponent* Component);

	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	void RemoveVisionComponent(class UFogOfWarVisionComponent* Component);

	FORCEINLINE const auto& GetVisionComponents() const { return VisionComponents; }

	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	class UTextureRenderTarget2D* GetCompositorRenderTarget();

	//Gets the world transform of the canvas that fog of war vision is drawn to
	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	FTransform GetDisplayCanvasTransform() const;

	//Gets the canvas transform for a particular cell
	FTransform GetDiscoveredAreasCanvasTransform(FIntPoint Cell) const;

	FIntPoint WorldToDiscoveredAreas(const FVector2D& Position) const;

	FIntRect WorldToDiscoveredAreas(const FBox2D& Box) const;

	FVector2D DiscoveredAreasToWorld(FIntPoint Cell) const;

	FBox2D DiscoveredAreasToWorld(FIntRect Box) const;

	//Find or add the discovered areas canvas for the given cell and ensure it has begun drawing.
	FCanvas& BeginDrawDiscoveredArea(FIntPoint Cell);

	//Ends drawing on all currently drawing discovered areas
	void EndDrawDiscoveredAreas();

	//Executes Func(Cell, Args...) for every cell that overlaps the input box
	//E.G. ForEachDiscoveredArea(Display->WorldToDiscoveredAreas(MyCanvasBounds), [&](FIntPoint Cell){ auto& Canvas = Display->BeginDrawDiscoveredArea(Cell); .... });
	template <typename FuncType, typename ... ArgTypes>
	void ForEachDiscoveredArea(FIntRect Box, FuncType&& Func, ArgTypes&& ... Args)
	{
		for (int32 Y = Box.Min.Y; Y < Box.Max.Y; ++Y)
			for (int32 X = Box.Min.X; X < Box.Max.X; ++X)
				Func(FIntPoint{ X,Y }, Forward<ArgTypes>(Args)...);
	}


	//Temporal fade of fog of war display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	float DisplayPersistence = 0.f;

	//World space radius of blur applied to display texture
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	float BlurRadius = 100.f;

	//How bright the discovered areas appear
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	float DiscoveredAreasOpacity = 0.2f;

	//Override all discovered areas to always be fully discovered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool bForceAllDiscovered = false;

protected:

	//The post process material on the camera that is used to display the fog of war.
	//This material must be present in the blendable post process stack of any camera components in order for fog of war to be visible.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	class UMaterialInterface* PostProcessMaterial;

	//Resolution of fog of war texture
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	int32 DisplayResolution = 512;

	//Resolution of intermediate compositor texture that vision components use to render shadows.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	int32 CompositorResolution = 128;

	//Material that is used to smooth out the display pixels using separable kernel gaussian blur technique
	//This material should implement the X pass of the blur
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	class UMaterialInterface* BlurPassXMaterial;

	//Material that is used to smooth out the display pixels using separable kernel gaussian blur technique
	//This material should implement the X pass of the blur
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	class UMaterialInterface* BlurPassYMaterial;

	//World space size of each pixel in the discovered area render targets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	double DiscoveredAreaTexelSize = 10.0;

	//Size of each texture cell in the discovered area
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FogOfWar)
	int32 DiscoveredAreaResolution = 512;

	double DiscoveredAreaCellSize;
	double InvDiscoveredAreaCellSize;

	void UpdateDiscoveredAreaCellSize();

	////The render target holding areas that have been discovered
	//FORCEINLINE class UTextureRenderTarget2D* GetDiscoveredAreaRenderTarget() const { return DiscoveredAreaRenderTarget; };

	////The canvas transform used to draw vision geometry into the discovered render target
	//FORCEINLINE const FTransform& GetDiscoveredAreaCanvasTransform() const { return DiscoveredAreaCanvasTransform; };

	////Clears the discovered areas to the given color. Black == undiscovered, White == Discovered.
	//UFUNCTION(BlueprintCallable, Category = FogOfWar)
	//void ClearDiscoveredAreas(FLinearColor Color);




	void InitializeFogOfWarResources();

	//The transform of the vision canvas at the previous update
	//Allows rendering previous frame into current frame, which is used in fade out.
	FTransform PreviousCanvasTransform;

	//The previous frames render target for vision.
	UPROPERTY(Transient)
	class UTextureRenderTarget2D* DisplaySrc;

	//The current frames render target for vision
	UPROPERTY(Transient)
	class UTextureRenderTarget2D* DisplayDest;

	//Intermediate render target for vision components to use to composit their vision and shadows before rendering into the vision render target
	UPROPERTY(Transient)
	class UTextureRenderTarget2D* CompositorRenderTarget;

	UPROPERTY(Transient)
	class UTextureRenderTarget2D* BlurPassXRenderTarget;

	UPROPERTY(Transient)
	class UTextureRenderTarget2D* BlurPassYRenderTarget;

	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* BlurPassXDynamicMaterial;

	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* BlurPassYDynamicMaterial;

	void InitializeBlurPassResources();

	void UpdateBlurPassResources();

	void DrawBlurPass();

	//Applies the final fog of war parameters to any active cameras on the owning actor.
	void ApplyPostProcessMaterialParameters();

	////Visit this worlds fog of war manager and create our discovered areas render target
	//void InitializeDiscoveredAreaResources();

	////The render target for areas that this player has discovered
	//UPROPERTY(Transient, BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	//class UTextureRenderTarget2D* DiscoveredAreaRenderTarget;

	//UPROPERTY(BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	//FTransform DiscoveredAreaCanvasTransform;

	UPROPERTY(BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	TSet<class UFogOfWarVisionComponent*> VisionComponents;

	UPROPERTY()
	TMap<FIntPoint, FDiscoveredAreasCell> DiscoveredAreas;
};
