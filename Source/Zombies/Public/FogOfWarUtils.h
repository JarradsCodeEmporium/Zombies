//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Engine/Canvas.h"

class UTextureRenderTarget2D;

class UBoxComponent;

class UCapsuleComponent;

class USphereComponent;

namespace FogOfWarUtils
{
	//Get the non-snapped canvas transform that covers a box
	FTransform GetCanvasTransform(const UBoxComponent* Box);

	//Get the non-snapped canvas transform that covers a capsule
	FTransform GetCanvasTransform(const UCapsuleComponent* Capsule);

	//Get the non-snapped canvas transform that covers a sphere
	FTransform GetCanvasTransform(const USphereComponent* Sphere);

	//Get the texel snapped canvas transform that covers a box
	FTransform GetSnappedCanvasTransform(const UBoxComponent* Box, int32 Width, int32 Height);

	//Get the texel snapped canvas transform that covers a capsule
	FTransform GetSnappedCanvasTransform(const UCapsuleComponent* Capsule, int32 Width, int32 Height);

	//Get the texel snapped canvas transform that covers a sphere
	FTransform GetSnappedCanvasTransform(const USphereComponent* Sphere, int32 Width, int32 Height);

	//Snaps a transform to the nearest world space texel, to prevent subpixel artifacts when moving the rendered region of a canvas around the world 
	//(e.g. when displaying vision texture)
	FTransform GetSnappedCanvasTransform(const FTransform& DesiredTransform, FVector2D TexelSize);

	FCanvas BeginDrawingCanvas(UWorld* World, UTextureRenderTarget2D* RenderTarget);

	FCanvas BeginDrawingCanvas(UWorld* World, const FTransform& Transform, UTextureRenderTarget2D* RenderTarget);

	void EndDrawingCanvas(FCanvas& Canvas, UTextureRenderTarget2D* RenderTarget);

	void DrawColoredQuad(FCanvas& Canvas, const FTransform& Transform, FLinearColor Color = FLinearColor::White, ESimpleElementBlendMode BlendMode = ESimpleElementBlendMode::SE_BLEND_Opaque);

	void DrawTexturedQuad(FCanvas& Canvas, const FTransform& Transform, class UTexture* Texture, FLinearColor Color = FLinearColor::White, ESimpleElementBlendMode BlendMode = ESimpleElementBlendMode::SE_BLEND_Opaque);

	void DrawTexturedQuad(FCanvas& Canvas, const FTransform& Transform, class FTexture* Texture, FLinearColor Color = FLinearColor::White, ESimpleElementBlendMode BlendMode = ESimpleElementBlendMode::SE_BLEND_Opaque);

	void DrawCone(class FCanvas& Canvas, FVector2D Centre, FVector2D Direction, double Radius, double HalfFOVInRadians);

	void DrawMaterial(FCanvas& Canvas, UMaterialInterface* Material);

	UTextureRenderTarget2D* CreateRenderTarget(UObject* Outer, uint32 Resolution);

	//Gets the bounds of a canvas transform
	FBox2D GetCanvasBounds(const FTransform& Transform);

	//Transforms a 2D box so that the result box contains at least the input box
	//2D equivalent of FBox::TransformBy()
	FBox2D TransformBox2D(const FTransform& Transform, const FBox2D& Box);
}