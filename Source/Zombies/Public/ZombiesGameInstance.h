//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "HighlightingCommon.h"
#include "ZombiesGameInstance.generated.h"


/**
 * 
 */
UCLASS()
class ZOMBIES_API UZombiesGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:

	virtual void Init() override;

	//Since subsystems cannot be tweaked in blueprint, we just stick the data here.
	//since we're in control of the full project it doesn't matter if its not as portable

	//Texture format of highlight stencil -> color map
	static constexpr EPixelFormat HighlightPixelFormat = PF_B8G8R8A8;

	static constexpr int32 HighlightPixelSizeInBytes = 4;

	//16x16 maps entire 256 values of stencil
	//Highest 4 bits is y, lowest 4 is x
	static constexpr int32 HighlightTextureResolution = 16;

	//Thickness of outline for highlights that specify outline (not tweakable per-highlight type)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Highlighting)
	float OutlineThickness = 2.f;

	//Brightness multiplier for XRay color. If the XRay color is too hard to see, increase this value.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Highlighting)
	float XRayBrightness = 2.f;

	//Ensures the given post process material has the correct parameters to display the highlighting
	UFUNCTION(BlueprintCallable, Category = Highlighting)
	void UpdatePostProcessMaterial(class UMaterialInstanceDynamic* DynamicMaterial);

	//Updates all matching post process materials in all camera components on the actor
	UFUNCTION(BlueprintCallable, Category = Highlighting)
	void UpdateAllPostProcessMaterials(class AActor* Actor, class UMaterialInterface* Material);

	//Calculate the stencil mapping based on highlight colors array
	UFUNCTION(BlueprintCallable, Category = Highlighting)
	void UpdateStencil();

protected:


	//Upload the pixel data to the gpu. PixelData argument will be emptied.
	void UpdateStencilMapTexture(TArray<FColor>& PixelData);

	//The texture map that specifies colors
	UPROPERTY(Transient)
	class UTexture2D* StencilMap;

	//Any stencil values less than this will not have an outline
	uint8 StencilOutlineCutoff = 1;
};
