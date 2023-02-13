//Copyright Jarrad Alexander 2022


#include "FogOfWarDisplayComponent.h"
#include "FogOfWarManager.h"
#include "FogOfWarVisionComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "FogOfWarUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "FogOfWarActor.h"

static TAutoConsoleVariable<float> FogOfWarOpacity
(
	TEXT("FogOfWar.Opacity"),
	1.f,
	TEXT("Opacity of the fog of war effect. 0.0 == disabled, 1.0 == fully darkened.")
);

UFogOfWarDisplayComponent::UFogOfWarDisplayComponent()
{
	SetAbsolute(false, true, false);

	SetCollisionProfileName("FogOfWarOccluder");
	
	InitBoxExtent(FVector{ 1000.0 });

	PrimaryComponentTick.bCanEverTick = true;

	UpdateDiscoveredAreaCellSize();
}

void UFogOfWarDisplayComponent::BeginPlay()
{
	UpdateDiscoveredAreaCellSize();

	Super::BeginPlay();

	InitializeFogOfWarResources();
}

void UFogOfWarDisplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFogOfWar(DeltaTime);
}

void UFogOfWarDisplayComponent::UpdateFogOfWar(float DeltaTime)
{
	if (!GetOwner()->HasActiveCameraComponent())
		//Don't do any rendering if there is no active camera component to see it
		return;

	//Src now contains previous frames update, which can now be used for temporal fading.
	Swap(DisplaySrc, DisplayDest);

	auto DisplayCanvasTransform = GetDisplayCanvasTransform();

	auto DisplayCanvasBounds = FogOfWarUtils::GetCanvasBounds(DisplayCanvasTransform);

	auto DisplayCanvas = FogOfWarUtils::BeginDrawingCanvas(GetWorld(), DisplayCanvasTransform, DisplayDest);

	DisplayCanvas.Clear(FLinearColor::Black);

	for (auto VisionComponent : VisionComponents)
		VisionComponent->DrawVision(this, DisplayCanvas);

	//If any vision components rendered to discovered areas, end their canvases so we can render the result to the display
	EndDrawDiscoveredAreas();

	if (bForceAllDiscovered)
		FogOfWarUtils::DrawColoredQuad(DisplayCanvas, DisplayCanvasTransform, FLinearColor::White * DiscoveredAreasOpacity, SE_BLEND_Additive);
	else
	{
		auto DrawDiscoveredAreas = [&](FIntPoint Cell)
		{
			auto DiscoveredArea = DiscoveredAreas.Find(Cell);

			if (!DiscoveredArea)
				//Nothing there, defaults to black
				return;

			FogOfWarUtils::DrawTexturedQuad(DisplayCanvas, GetDiscoveredAreasCanvasTransform(Cell), DiscoveredArea->RenderTarget, FLinearColor::White * DiscoveredAreasOpacity, SE_BLEND_Additive);
		};

		ForEachDiscoveredArea(WorldToDiscoveredAreas(DisplayCanvasBounds), DrawDiscoveredAreas);
	}

	float DisplayPersistenceAlpha = FMath::Clamp(1.f - DeltaTime / FMath::Max(0.00001f, DisplayPersistence), 0.f, 1.f);

	if (DisplayPersistenceAlpha > 0.1f)
	{
		FogOfWarUtils::DrawTexturedQuad(DisplayCanvas, PreviousCanvasTransform, DisplaySrc, FLinearColor{1., 1., 1., DisplayPersistenceAlpha}, SE_BLEND_AlphaBlend);
	}

	FogOfWarUtils::EndDrawingCanvas(DisplayCanvas, DisplayDest);

	DrawBlurPass();

	PreviousCanvasTransform = DisplayCanvasTransform;

	ApplyPostProcessMaterialParameters();
}


UTextureRenderTarget2D* UFogOfWarDisplayComponent::GetCompositorRenderTarget()
{
	return CompositorRenderTarget;
}

//void UFogOfWarDisplayComponent::ClearDiscoveredAreas(FLinearColor Color)
//{
//	if (!DiscoveredAreaRenderTarget)
//		return;
//
//	UKismetRenderingLibrary::ClearRenderTarget2D(this, DiscoveredAreaRenderTarget, Color);
//}

void UFogOfWarDisplayComponent::AddVisionComponent(UFogOfWarVisionComponent* Component)
{
	if (!IsValid(Component))
		return;

	VisionComponents.Add(Component);
}

void UFogOfWarDisplayComponent::RemoveVisionComponent(UFogOfWarVisionComponent* Component)
{
	if (!IsValid(Component))
		return;

	VisionComponents.Remove(Component);
}

FTransform UFogOfWarDisplayComponent::GetDisplayCanvasTransform() const
{
	return FogOfWarUtils::GetSnappedCanvasTransform(this, DisplayResolution, DisplayResolution);
}

FTransform UFogOfWarDisplayComponent::GetDiscoveredAreasCanvasTransform(FIntPoint Cell) const
{
	double Offset = DiscoveredAreaCellSize * 0.5;
	return FTransform{ FQuat::Identity, FVector{ DiscoveredAreasToWorld(Cell) + Offset, 0.0 }, FVector{ Offset, Offset, 1.0 } };
}

FIntPoint UFogOfWarDisplayComponent::WorldToDiscoveredAreas(const FVector2D& Position) const
{
	return FIntPoint{ FMath::FloorToInt32(Position.X * InvDiscoveredAreaCellSize), FMath::FloorToInt32(Position.Y * InvDiscoveredAreaCellSize) };
}

FIntRect UFogOfWarDisplayComponent::WorldToDiscoveredAreas(const FBox2D& Box) const
{
	return FIntRect{ WorldToDiscoveredAreas(Box.Min), WorldToDiscoveredAreas(Box.Max) + 1 };
}

FVector2D UFogOfWarDisplayComponent::DiscoveredAreasToWorld(FIntPoint Cell) const
{
	return FVector2D{ Cell.X * DiscoveredAreaCellSize, Cell.Y * DiscoveredAreaCellSize };
}

FBox2D UFogOfWarDisplayComponent::DiscoveredAreasToWorld(FIntRect Box) const
{
	return FBox2D{ DiscoveredAreasToWorld(Box.Min), DiscoveredAreasToWorld(Box.Max) };
}

FCanvas& UFogOfWarDisplayComponent::BeginDrawDiscoveredArea(FIntPoint Cell)
{
	auto& DiscoveredArea = DiscoveredAreas.FindOrAdd(Cell);

	if (DiscoveredArea.Canvas)
		return *DiscoveredArea.Canvas;

	if (!DiscoveredArea.RenderTarget)
		DiscoveredArea.RenderTarget = FogOfWarUtils::CreateRenderTarget(this, DiscoveredAreaResolution);

	DiscoveredArea.Canvas = MakeShared<FCanvas>(FogOfWarUtils::BeginDrawingCanvas(GetWorld(), GetDiscoveredAreasCanvasTransform(Cell), DiscoveredArea.RenderTarget));

	return *DiscoveredArea.Canvas;
}

void UFogOfWarDisplayComponent::EndDrawDiscoveredAreas()
{
	for (auto& [Cell, DiscoveredArea] : DiscoveredAreas)
	{
		if (!DiscoveredArea.Canvas)
			continue;

		FogOfWarUtils::EndDrawingCanvas(*DiscoveredArea.Canvas, DiscoveredArea.RenderTarget);
	}
}



void UFogOfWarDisplayComponent::UpdateDiscoveredAreaCellSize()
{
	DiscoveredAreaCellSize = DiscoveredAreaResolution * DiscoveredAreaTexelSize;
	InvDiscoveredAreaCellSize = 1.0 / DiscoveredAreaCellSize;
}

void UFogOfWarDisplayComponent::InitializeFogOfWarResources()
{
	DisplaySrc = FogOfWarUtils::CreateRenderTarget(this, DisplayResolution);

	DisplayDest = FogOfWarUtils::CreateRenderTarget(this, DisplayResolution);

	CompositorRenderTarget = FogOfWarUtils::CreateRenderTarget(this, CompositorResolution);

	//InitializeDiscoveredAreaResources();

	InitializeBlurPassResources();
}

void UFogOfWarDisplayComponent::InitializeBlurPassResources()
{
	BlurPassXRenderTarget = FogOfWarUtils::CreateRenderTarget(this, DisplayResolution);

	BlurPassYRenderTarget = FogOfWarUtils::CreateRenderTarget(this, DisplayResolution);

	//Half of a symmetric 7 sample gaussian kernel
	FVector4 KernelWeights
	{
		0.28125f,
		0.21875f,
		0.109375f,
		0.03125f
	};

	BlurPassXDynamicMaterial = UMaterialInstanceDynamic::Create(BlurPassXMaterial, this);
	
	BlurPassXDynamicMaterial->SetVectorParameterValue("KernelWeights", KernelWeights);

	BlurPassYDynamicMaterial = UMaterialInstanceDynamic::Create(BlurPassYMaterial, this);

	BlurPassYDynamicMaterial->SetVectorParameterValue("KernelWeights", KernelWeights);

}

void UFogOfWarDisplayComponent::UpdateBlurPassResources()
{
	
	BlurPassXDynamicMaterial->SetTextureParameterValue("Texture", DisplayDest);

	BlurPassYDynamicMaterial->SetTextureParameterValue("Texture", BlurPassXRenderTarget);

	float StepSize = BlurRadius / 6.f;

	//Offsets are dependent on blur radius and display size
	FVector4 KernelOffsets
	{
		StepSize * 0.f,
		StepSize * 1.f,
		StepSize * 2.f,
		StepSize * 3.f,
	};

	BlurPassXDynamicMaterial->SetVectorParameterValue("KernelOffsets", KernelOffsets / GetScaledBoxExtent().X);

	BlurPassYDynamicMaterial->SetVectorParameterValue("KernelOffsets", KernelOffsets / GetScaledBoxExtent().Y);

}

void UFogOfWarDisplayComponent::DrawBlurPass()
{
	//For some reason, the inner drawing of material tiles doesn't behave the same way as some other tile overloads, so we must manually set a Y flipped identity base transform.
	static const FMatrix MaterialTransform{ {1.0, 0.0, 0.0, 0.0}, {0.0, -1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} };

	UpdateBlurPassResources();

	{
		auto Canvas = FogOfWarUtils::BeginDrawingCanvas(GetWorld(), BlurPassXRenderTarget);

		Canvas.SetBaseTransform(MaterialTransform);

		FogOfWarUtils::DrawMaterial(Canvas, BlurPassXDynamicMaterial);

		FogOfWarUtils::EndDrawingCanvas(Canvas, BlurPassXRenderTarget);
	}

	{
		auto Canvas = FogOfWarUtils::BeginDrawingCanvas(GetWorld(), BlurPassYRenderTarget);

		Canvas.SetBaseTransform(MaterialTransform);

		FogOfWarUtils::DrawMaterial(Canvas, BlurPassYDynamicMaterial);

		FogOfWarUtils::EndDrawingCanvas(Canvas, BlurPassYRenderTarget);
	}
}

void UFogOfWarDisplayComponent::ApplyPostProcessMaterialParameters()
{
	//Remap canvas transform from -1 -> 1 to 0 -> 1
	//Saves a tiny bit of math for every pixel on screen so might as well do it here.
	auto CanvasTransform = GetDisplayCanvasTransform();

	CanvasTransform.AddToTranslation(CanvasTransform.GetScale3D() * FVector { -1.f, 1.f, 0.f });

	//Flip Y again to match world space
	CanvasTransform.MultiplyScale3D(FVector{ 2.f, -2.f, 1.f });

	FVector4 VisionBasisInvScale = CanvasTransform.GetScale3D().Reciprocal();

	FVector4 VisionBasisTranslation = CanvasTransform.GetLocation();

	for (const auto& Component : GetOwner()->GetComponents())
	{
		auto Camera = Cast<UCameraComponent>(Component);

		if (!Camera)
			continue;

		if (!Camera->IsActive())
			continue;

		for (auto& Blendable : Camera->PostProcessSettings.WeightedBlendables.Array)
		{
			auto MaterialInterface = Cast<UMaterialInterface>(Blendable.Object);

			if (!MaterialInterface)
				continue;

			//Ensure that any fog of war post processes are replaced with a dynamic version that can have its parameters updated
			if (MaterialInterface == PostProcessMaterial)
			{
				MaterialInterface = UMaterialInstanceDynamic::Create(PostProcessMaterial, Camera);
				Blendable.Object = MaterialInterface;
			}

			auto DynamicInstance = Cast<UMaterialInstanceDynamic>(MaterialInterface);

			if (!DynamicInstance || !DynamicInstance->IsChildOf(PostProcessMaterial))
				continue;

			//@note: Basis vectors are currently unused since the display does not rotate. 
			//If this changes, we will also need to update the material to apply the inverse rotation math as well.
			//DynamicInstance->SetVectorParameterValue("VisionBasisX", VisionBasisX);
			//DynamicInstance->SetVectorParameterValue("VisionBasisY", VisionBasisY);
			DynamicInstance->SetVectorParameterValue("VisionBasisInvScale", VisionBasisInvScale);
			DynamicInstance->SetVectorParameterValue("VisionBasisTranslation", VisionBasisTranslation);
			DynamicInstance->SetTextureParameterValue("VisionTexture", BlurPassYRenderTarget);
			DynamicInstance->SetScalarParameterValue("FogOfWarOpacity", FogOfWarOpacity.GetValueOnGameThread());

		}

	}

}

//void UFogOfWarDisplayComponent::InitializeDiscoveredAreaResources()
//{
//	auto Manager = AFogOfWarManager::GetFogOfWarManager(this);
//
//	if (Manager)
//	{
//		//Each display component has its own discovered areas
//		DiscoveredAreaRenderTarget = Manager->CreateDiscoveredAreaRenderTarget(this);
//
//		DiscoveredAreaCanvasTransform = Manager->GetDiscoveredAreaCanvasTransform();
//	}
//	else
//	{
//		//Use default settings that covers 100m by 100m at the world origin
//
//		DiscoveredAreaRenderTarget = FogOfWarUtils::CreateRenderTarget(this, 256);
//
//		DiscoveredAreaCanvasTransform = FTransform{ FQuat::Identity, FVector::Zero(), FVector{5000., 5000., 1.} };
//	}
//}
