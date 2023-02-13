//Copyright Jarrad Alexander 2022


#include "FogOfWarUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"

FTransform FogOfWarUtils::GetCanvasTransform(const UBoxComponent* Box)
{
	check(Box);

	auto Transform = Box->GetComponentTransform();

	Transform.SetLocation({ Transform.GetLocation().X, Transform.GetLocation().Y, 0.0 });
	
	Transform.MultiplyScale3D({Box->GetUnscaledBoxExtent().X, Box->GetUnscaledBoxExtent().Y, 1});

	return Transform;
}

FTransform FogOfWarUtils::GetCanvasTransform(const UCapsuleComponent* Capsule)
{
	check(Capsule);

	auto Transform = Capsule->GetComponentTransform();

	Transform.SetLocation({ Transform.GetLocation().X, Transform.GetLocation().Y, 0.0 });

	Transform.MultiplyScale3D({ Capsule->GetUnscaledCapsuleRadius(), Capsule->GetUnscaledCapsuleRadius(), 1.0 });

	return Transform;
}

FTransform FogOfWarUtils::GetCanvasTransform(const USphereComponent* Sphere)
{
	check(Sphere);

	auto Transform = Sphere->GetComponentTransform();

	Transform.SetLocation({ Transform.GetLocation().X, Transform.GetLocation().Y, 0.0 });

	Transform.MultiplyScale3D({ Sphere->GetUnscaledSphereRadius(), Sphere->GetUnscaledSphereRadius(), 1.0 });

	return Transform;
}

FTransform FogOfWarUtils::GetSnappedCanvasTransform(const UBoxComponent* Box, int32 Width, int32 Height)
{
	check(Box);

	auto Extent = Box->GetScaledBoxExtent() * 2.0;

	return GetSnappedCanvasTransform(GetCanvasTransform(Box), { Extent.X / Width, Extent.Y / Height});

}

FTransform FogOfWarUtils::GetSnappedCanvasTransform(const UCapsuleComponent* Capsule, int32 Width, int32 Height)
{
	check(Capsule);

	float Diameter = Capsule->GetScaledCapsuleRadius() * 2.f;

	return GetSnappedCanvasTransform(GetCanvasTransform(Capsule), {Diameter / Width, Diameter / Height});
}

FTransform FogOfWarUtils::GetSnappedCanvasTransform(const USphereComponent* Sphere, int32 Width, int32 Height)
{
	check(Sphere);

	float Diameter = Sphere->GetScaledSphereRadius() * 2.f;

	return GetSnappedCanvasTransform(GetCanvasTransform(Sphere), { Diameter / Width, Diameter / Height });
}

FTransform FogOfWarUtils::GetSnappedCanvasTransform(const FTransform& DesiredTransform, FVector2D TexelSize)
{
	auto RotatedTexelPosition = DesiredTransform.InverseTransformVectorNoScale(DesiredTransform.GetLocation());

	RotatedTexelPosition.X = FMath::GridSnap(RotatedTexelPosition.X, TexelSize.X);

	RotatedTexelPosition.Y = FMath::GridSnap(RotatedTexelPosition.Y, TexelSize.Y);

	return FTransform{DesiredTransform.GetRotation(), DesiredTransform.TransformVectorNoScale(RotatedTexelPosition), DesiredTransform.GetScale3D()};
}

FCanvas FogOfWarUtils::BeginDrawingCanvas(UWorld* World, UTextureRenderTarget2D* RenderTarget)
{
	check(World);

	check(RenderTarget);

	World->FlushDeferredParameterCollectionInstanceUpdates();

	auto Resource = RenderTarget->GameThread_GetRenderTargetResource();

	FCanvas Canvas{ Resource, nullptr, World, World->FeatureLevel,FCanvas::CDM_ImmediateDrawing };

	ENQUEUE_RENDER_COMMAND(FlushDeferredUpdates)(
		[Resource](FRHICommandListImmediate& RHICmdList)
		{
			Resource->FlushDeferredResourceUpdate(RHICmdList);
		});

	RenderTarget->ReleaseFence.BeginFence();

	return Canvas;
}

FCanvas FogOfWarUtils::BeginDrawingCanvas(UWorld* World, const FTransform& Transform, UTextureRenderTarget2D* RenderTarget)
{
	auto Canvas = BeginDrawingCanvas(World, RenderTarget);

	//@todo: consider negative Z values, do they clip the geometry?
	Canvas.SetBaseTransform(Transform.ToInverseMatrixWithScale());
	
	return Canvas;
}

void FogOfWarUtils::EndDrawingCanvas(FCanvas& Canvas, UTextureRenderTarget2D* RenderTarget)
{
	Canvas.Flush_GameThread();

	//Apparently this isn't necessary at all any more.
	//ENQUEUE_RENDER_COMMAND(ResolveVision)(
	//	[Resource = RenderTarget->GameThread_GetRenderTargetResource()](FRHICommandList& RHICmdList)
	//	{
	//		RHICmdList.CopyToResolveTarget(Resource->GetRenderTargetTexture(), Resource->TextureRHI, FResolveParams());
	//		//RHICmdList.CopyTexture(Resource->GetRenderTargetTexture(), Resource->TextureRHI, FRHICopyTextureInfo());
	//	}
	//);
	
	//RenderTarget->ReleaseFence.BeginFence();
}

void FogOfWarUtils::DrawColoredQuad(FCanvas& Canvas, const FTransform& Transform, FLinearColor Color, ESimpleElementBlendMode BlendMode)
{
	DrawTexturedQuad(Canvas, Transform, GWhiteTexture, Color, BlendMode);
}

void FogOfWarUtils::DrawTexturedQuad(FCanvas& Canvas, const FTransform& Transform, UTexture* Texture, FLinearColor Color, ESimpleElementBlendMode BlendMode)
{
	DrawTexturedQuad(Canvas, Transform, Texture->GetResource(), Color, BlendMode);
}

void FogOfWarUtils::DrawTexturedQuad(FCanvas& Canvas, const FTransform& Transform, FTexture* Texture, FLinearColor Color, ESimpleElementBlendMode BlendMode)
{
	TArray<FCanvasUVTri> QuadTriangles;

	QuadTriangles.SetNum(2);

	FVector2D BottomLeft{ Transform.TransformPosition({ -1.0, -1.0, 0.0 }) };
	FVector2D BottomRight{ Transform.TransformPosition({ 1.0, -1.0, 0.0 }) };
	FVector2D TopLeft{ Transform.TransformPosition({ -1.0, 1.0, 0.0 }) };
	FVector2D TopRight{ Transform.TransformPosition({ 1.0, 1.0, 0.0 }) };

	QuadTriangles[0].V0_Pos = TopLeft;
	QuadTriangles[0].V0_UV = { 0.0, 0.0 };

	QuadTriangles[0].V1_Pos = TopRight;
	QuadTriangles[0].V1_UV = { 1.0, 0.0 };

	QuadTriangles[0].V2_Pos = BottomLeft;
	QuadTriangles[0].V2_UV = { 0.0, 1.0 };

	QuadTriangles[1].V0_Pos = TopRight;
	QuadTriangles[1].V0_UV = { 1.0, 0.0 };

	QuadTriangles[1].V1_Pos = BottomRight;
	QuadTriangles[1].V1_UV = { 1.0, 1.0 };

	QuadTriangles[1].V2_Pos = BottomLeft;
	QuadTriangles[1].V2_UV = { 0.0, 1.0 };

	FCanvasTriangleItem Item{ QuadTriangles, Texture };
	
	Item.BlendMode = BlendMode;
	
	Item.SetColor(Color);
	
	Canvas.DrawItem(Item);
}

void FogOfWarUtils::DrawCone(FCanvas& Canvas, FVector2D Centre, FVector2D Direction, double Radius, double HalfFOVInRadians)
{
	HalfFOVInRadians = FMath::Clamp(HalfFOVInRadians, 0.0, 180.0);

	Radius = FMath::Max(Radius, 0.0);

	auto BasisX = Direction.GetSafeNormal() * Radius;

	FVector2D BasisY{ -BasisX.Y, BasisX.X };

	double AngleStep = PI / 32.0;

	double Angle = 0.0;
	double NextAngle = FMath::Min(AngleStep, HalfFOVInRadians);

	FCanvasUVTri PositiveTri;

	PositiveTri.V0_Pos = Centre;

	PositiveTri.V1_Pos = Centre + Direction * Radius;

	FCanvasUVTri NegativeTri;

	NegativeTri.V0_Pos = PositiveTri.V0_Pos;

	NegativeTri.V2_Pos = PositiveTri.V1_Pos;

	TArray<FCanvasUVTri> Triangles;

	Triangles.Reserve(FMath::CeilToInt(HalfFOVInRadians / AngleStep) * 2);

	//Fan out from front direction
	while (Angle < HalfFOVInRadians)
	{
		auto AxialPosition = Centre + FMath::Cos(NextAngle) * BasisX;
		auto PerpendicularOffset = FMath::Sin(NextAngle) * BasisY;

		PositiveTri.V2_Pos = AxialPosition + PerpendicularOffset;
		NegativeTri.V1_Pos = AxialPosition - PerpendicularOffset;

		Triangles.Add(PositiveTri);
		Triangles.Add(NegativeTri);

		PositiveTri.V1_Pos = PositiveTri.V2_Pos;
		NegativeTri.V2_Pos = NegativeTri.V1_Pos;
		Angle = NextAngle;
		NextAngle = FMath::Min(Angle + AngleStep, HalfFOVInRadians);
	}

	if (Triangles.Num() == 0)
		return;

	FCanvasTriangleItem Item{ Triangles, GWhiteTexture };

	Item.SetColor(FLinearColor::White);

	Canvas.DrawItem(Item);
}

void FogOfWarUtils::DrawMaterial(FCanvas& Canvas, UMaterialInterface* Material)
{
	FCanvasTileItem Item{ FVector2D{-1.0,-1.0}, Material->GetRenderProxy(), FVector2D{2.0,2.0} };

	Canvas.DrawItem(Item);
}

UTextureRenderTarget2D* FogOfWarUtils::CreateRenderTarget(UObject* Outer, uint32 Resolution)
{
	check(Resolution >= 32);

	UTextureRenderTarget2D* Texture = NewObject<UTextureRenderTarget2D>(Outer ? Outer : GetTransientPackage());

	check(Texture);

	Texture->RenderTargetFormat = ETextureRenderTargetFormat::RTF_R8;

	Texture->ClearColor = FLinearColor::Black;

	Texture->bAutoGenerateMips = false;

	Texture->InitAutoFormat(Resolution, Resolution);

	Texture->UpdateResourceImmediate(true);

	return Texture;
}

FBox2D FogOfWarUtils::GetCanvasBounds(const FTransform& Transform)
{
	FBox2D Result{ ForceInit };

	Result += FVector2D{ Transform.TransformPosition(FVector{-1.0, -1.0, 0.0}) };
	Result += FVector2D{ Transform.TransformPosition(FVector{ 1.0, -1.0, 0.0}) };
	Result += FVector2D{ Transform.TransformPosition(FVector{-1.0, 1.0, 0.0}) };
	Result += FVector2D{ Transform.TransformPosition(FVector{ 1.0, 1.0, 0.0}) };

	return Result;
}

FBox2D FogOfWarUtils::TransformBox2D(const FTransform& Transform, const FBox2D& Box)
{
	FBox2D Result{ForceInit};

	Result += FVector2D{ Transform.TransformPosition(FVector{ Box.Min.X, Box.Min.Y, 0.0 }) };
	Result += FVector2D{ Transform.TransformPosition(FVector{ Box.Max.X, Box.Min.Y, 0.0 }) };
	Result += FVector2D{ Transform.TransformPosition(FVector{ Box.Min.X, Box.Max.Y, 0.0 }) };
	Result += FVector2D{ Transform.TransformPosition(FVector{ Box.Max.X, Box.Max.Y, 0.0 }) };

	return Result;
}
