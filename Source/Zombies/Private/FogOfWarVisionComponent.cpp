//Copyright Jarrad Alexander 2022


#include "FogOfWarVisionComponent.h"
#include "FogOfWarSubSystem.h"
#include "FogOfWarDisplayComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "DrawDebugHelpers.h"
#include "FogOfWarOccluderSubsystem.h"
#include "FogOfWarUtils.h"
#include "FogOfWarActor.h"

static TAutoConsoleVariable<bool> FogOfWarVisionDebug
(
	TEXT("FogOfWar.Vision.Debug"),
	false,
	TEXT("Show debug info for fog of war vision components.")
);

static TAutoConsoleVariable<bool> FogOfWarVisionShadows
(
	TEXT("FogOfWar.Vision.Shadows"),
	true,
	TEXT("Enable or disable shadows in fog of war vision components.")
);

UFogOfWarVisionComponent::UFogOfWarVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UFogOfWarVisionComponent::BeginPlay()
{
	Super::BeginPlay();

	//if (auto FoundManager = AFogOfWarManager::GetFogOfWarManager(this))
	//	SetFogOfWarManager(FoundManager);

	FTimerHandle Handle;

	//Overlap detection can be done at a slower rate than whatever tick we want to do, so decouple it from tick
	//Also has a random initial delay so that all the actors on spawn don't all tick their components on the same frame.
	//GetWorld()->GetTimerManager().SetTimer(Handle, this, &UFogOfWarVisionComponent::UpdateVisionOverlaps, VisionActorsUpdateFrequency, true, FMath::FRand() * VisionActorsUpdateFrequency);

	UpdateVisionOverlaps();
}

void UFogOfWarVisionComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	ClearVisibleActors();

	//SetFogOfWarManager(nullptr);
}

void UFogOfWarVisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateVisionOverlaps();

}

void UFogOfWarVisionComponent::DrawVision(class UFogOfWarDisplayComponent* DisplayComponent, FCanvas& DisplayCanvas)
{
	//if (FogOfWarVisionDebug.GetValueOnGameThread())
	//{
	//	auto VisionBounds = GetVisionCanvasTransform();

	//	DrawDebugBox(GetWorld(), VisionBounds.GetLocation(), VisionBounds.GetScale3D(), VisionBounds.GetRotation(), FColor::Blue, false, GetWorld()->GetDeltaSeconds() * 1.05f);
	//}

	auto CompositorTransform = GetVisionCanvasTransform();

	auto CompositorBounds = GetVisionCanvasBounds();

	bool bIsInDisplayRegion = FogOfWarUtils::GetCanvasBounds(DisplayComponent->GetDisplayCanvasTransform()).Intersect(CompositorBounds);
	
	auto CompositorRenderTarget = DisplayComponent->GetCompositorRenderTarget();

	auto CompositorCanvas = FogOfWarUtils::BeginDrawingCanvas(GetWorld(), CompositorTransform, CompositorRenderTarget);

	FVector2D VisionCentre{ GetComponentLocation() };

	CompositorCanvas.Clear(FLinearColor::Black);

	//Self vision circle
	CompositorCanvas.DrawNGon(VisionCentre, FColor::White, 16, SelfVisionRadius);

	FogOfWarUtils::DrawCone(CompositorCanvas, VisionCentre, FVector2D{ GetForwardVector() }.GetSafeNormal(), VisionRadius, GetHalfFOVInRadians());
	
	//for (auto& Occluder : OverlappingVisionOccluders)
	//	DrawOccluderShadow(Occluder, CompositorCanvas, VisionCentre, VisionRadius, GlobalShadowBias);

	if (auto Subsystem = GetWorld()->GetSubsystem<UFogOfWarSubsystem>())
		for (auto It = Subsystem->GetStaticOccluders().WorldBoxQuery(CompositorBounds); It; ++It)
			DrawOccluderShadow(*It, CompositorCanvas, VisionCentre, VisionRadius, GlobalShadowBias);

	FogOfWarUtils::EndDrawingCanvas(CompositorCanvas, CompositorRenderTarget);

	//Only bother drawing in the display if we're actually going to be in the visible region
	if (bIsInDisplayRegion)
		FogOfWarUtils::DrawTexturedQuad(DisplayCanvas, CompositorTransform, CompositorRenderTarget, FLinearColor::White, SE_BLEND_Additive);

	//@todo: depending on game and level design, might want to discover areas in range regardless of occluders blocking direct vision
	//In that case, we could skip shadows and compositing entirely if we're not in the display region and instead simply render vision directly to the discovered areas canvas
	
	auto DrawDiscoveredArea = [&](FIntPoint Cell)
	{
		auto& DiscoveredAreaCanvas = DisplayComponent->BeginDrawDiscoveredArea(Cell);

		FogOfWarUtils::DrawTexturedQuad(DiscoveredAreaCanvas, CompositorTransform, CompositorRenderTarget, FLinearColor::White, SE_BLEND_Additive);

	};

	DisplayComponent->ForEachDiscoveredArea(DisplayComponent->WorldToDiscoveredAreas(CompositorBounds), DrawDiscoveredArea);

	
}

bool UFogOfWarVisionComponent::CanSeePoint(const FVector& Point) const
{
	FVector2D VisionLocation{ GetComponentLocation() };

	auto VisionToPoint = FVector2D{ Point } - VisionLocation;

	auto PointDistSqr = VisionToPoint.SizeSquared();

	if (PointDistSqr > FMath::Square(VisionRadius))
		//Fully outside the max range that can be seen
		return false;

	if (PointDistSqr < KINDA_SMALL_NUMBER)
		//Point is approximately on top of the vision source, so we will consider it trivially visible.
		return true;

	if (PointDistSqr > FMath::Square(SelfVisionRadius))
	{
		//If point is further than the 360 degree self vision, check against the fov cone
		auto PointAngleCos = (VisionToPoint * FMath::InvSqrtEst(PointDistSqr)) | (FVector2D{ GetForwardVector() }.GetSafeNormal());

		if (PointAngleCos < FMath::Cos(GetHalfFOVInRadians()))
			return false;
	}

	if (auto Subsystem = GetWorld()->GetSubsystem<UFogOfWarSubsystem>())
	{
		//@todo: this should probably step the ray through the grid rather than querying a box that contains the ray
		FBox2D QueryBox{ ForceInit };

		QueryBox += VisionLocation;
		QueryBox += FVector2D{ Point };

		for (auto It = Subsystem->GetStaticOccluders().WorldBoxQuery(QueryBox); It; ++It)
		{
			auto& Occluder = *It;

			if (!Occluder.OccluderMesh)
				continue;

			for (auto& Segment : Occluder.OccluderMesh->ShadowEdges)
			{
				//Unused, but needed for SegmentIntersection2D overload
				FVector IntersectionPoint;

				FVector From{ Occluder.Transform.TransformPosition(FVector{ Segment.Key, 0.0 }) };

				FVector To{ Occluder.Transform.TransformPosition(FVector{ Segment.Value, 0.0 }) };

				if (FMath::SegmentIntersection2D(GetComponentLocation(), Point, From, To, IntersectionPoint))
				{
					if (FogOfWarVisionDebug.GetValueOnGameThread())
					{
						DrawDebugLine(GetWorld(), GetComponentLocation(), IntersectionPoint, FColor::Green, false, VisionActorsUpdateFrequency);
						DrawDebugPoint(GetWorld(), IntersectionPoint, 10.f, FColor::Red, false, VisionActorsUpdateFrequency);
						DrawDebugLine(GetWorld(), IntersectionPoint, Point, FColor::Red, false, VisionActorsUpdateFrequency);
					}

					return false;
				}
			}
		}
	}

	if (FogOfWarVisionDebug.GetValueOnGameThread())
		DrawDebugLine(GetWorld(), GetComponentLocation(), Point, FColor::Magenta, false, VisionActorsUpdateFrequency);

	return true;
}

FTransform UFogOfWarVisionComponent::GetVisionCanvasTransform(bool bForceSquareAspectRatio) const
{
	//Form a bounding box that encloses the "pie slice" that is visible

	double HalfFOVRad = GetHalfFOVInRadians();

	//Clamp to only expand in the back quadrant
	double MinAxialDistance = FMath::Cos(FMath::Clamp(HalfFOVRad, PI * 0.5, PI)) * VisionRadius;

	//Clamp to only expand in the front quadrant
	double Width = FMath::Sin(FMath::Clamp(HalfFOVRad, 0.0, PI * 0.5)) * VisionRadius;

	FRotator Rotation{ 0.0, GetComponentRotation().Yaw, 0.0 };

	FVector Center{ GetComponentLocation() + Rotation.RotateVector(FVector{ (MinAxialDistance + VisionRadius) * 0.5, 0.0, 0.0 }) };

	FVector Extent{ (VisionRadius - MinAxialDistance) * 0.5 + VisionCanvasPadding, Width + VisionCanvasPadding, 1. };

	if (bForceSquareAspectRatio)
	{
		//Force square aspect ratio (prevents really stretched pixels at the cost of lower overall resolution)
		//This assumes the render target is also square. If not, this will need to account for the different aspect ratio.
		Extent.X = FMath::Max(Extent.X, Extent.Y);
		Extent.Y = Extent.X;
	}

	return FTransform{ Rotation, Center, Extent };
}

FBox2D UFogOfWarVisionComponent::GetVisionCanvasBounds(bool bForceSquareAspectRatio) const
{
	return FogOfWarUtils::GetCanvasBounds(GetVisionCanvasTransform(bForceSquareAspectRatio));
}

void UFogOfWarVisionComponent::DrawOccluderShadow(const FFogOfWarOccluderInstance& Occluder, FCanvas& Canvas, FVector2D Centre, double Radius, double ShadowBias) const
{
	if (!Occluder.OccluderMesh)
		return;

	auto DebugDeltaSeconds = GetWorld()->GetDeltaSeconds() * 0.05f;

	//if (FogOfWarVisionDebug.GetValueOnGameThread())
	//{
	//	auto ComponentBounds = Occluder.Component->Bounds;

	//	DrawDebugBox(GetWorld(), ComponentBounds.Origin, ComponentBounds.BoxExtent, Occluder.OccluderMesh ? FColor::Green : FColor::Red, false, DebugDeltaSeconds);
	//}

	//auto OccluderTransform = Occluder.Component->GetComponentTransform();

	TArray<FCanvasUVTri> ShadowTriangles;

	for (auto& [From, To] : Occluder.OccluderMesh->ShadowEdges)
	{
		FVector2D FromWS{ Occluder.Transform.TransformPosition(FVector{ From, 0.f }) };
		FVector2D ToWS{ Occluder.Transform.TransformPosition(FVector{ To, 0.f }) };

		//A shadow only needs to be cast far enough to get just outside the vision radius
		//The shadow edge is just the occluding edge scaled up and away from the vision center, 
		//so that its closest point lies on the vision circle

		FVector2D ClosestPoint = FMath::ClosestPointOnSegment2D(Centre, FromWS, ToWS);

		double DistanceSqr = (ClosestPoint - Centre).SizeSquared();

		if (DistanceSqr > Radius * Radius)
			continue;

		FVector2D FromOffset = FromWS - Centre;
		FVector2D ToOffset = ToWS - Centre;

		//Bias shadow casting edges
		double ScaledShadowBias = 1.f + ShadowBias;

		FromWS = Centre + FromOffset * ScaledShadowBias;
		ToWS = Centre + ToOffset * ScaledShadowBias;

		//Project back edge tangent to vision circle
		double ProjectionScale = Radius * FMath::InvSqrtEst(DistanceSqr);

		FVector2D FromShadowWS = Centre + FromOffset * ProjectionScale;
		FVector2D ToShadowWS = Centre + ToOffset * ProjectionScale;

		if (FogOfWarVisionDebug.GetValueOnGameThread())
		{	
			DrawDebugLine(GetWorld(), FVector{ FromWS, 1.f }, FVector{ ToWS, 1.f }, FColor::Green, false, DebugDeltaSeconds);
			DrawDebugLine(GetWorld(), FVector{ FromWS, 1.f }, FVector{ FromShadowWS, 1.f }, FColor::Turquoise, false, DebugDeltaSeconds);
			DrawDebugLine(GetWorld(), FVector{ ToWS, 1.f }, FVector{ ToShadowWS, 1.f }, FColor::Turquoise, false, DebugDeltaSeconds);
			DrawDebugLine(GetWorld(), FVector{ FromShadowWS, 1.f }, FVector{ ToShadowWS, 1.f }, FColor::Turquoise, false, DebugDeltaSeconds);
		}

		
		FCanvasUVTri TriangleA;
		FCanvasUVTri TriangleB;

		TriangleA.V0_Pos = FromWS;
		TriangleA.V0_UV = { 0.f, 0.f };
		TriangleA.V0_Color = FLinearColor::Black;

		TriangleA.V1_Pos = ToWS;
		TriangleA.V1_UV = { 1.f, 0.f };
		TriangleA.V1_Color = FLinearColor::Black;

		TriangleA.V2_Pos = ToShadowWS;
		TriangleA.V2_UV = { 0.f, 1.f };
		TriangleA.V2_Color = FLinearColor::Black;


		TriangleB.V0_Pos = FromShadowWS;
		TriangleB.V0_UV = { 1.f, 1.f };
		TriangleB.V0_Color = FLinearColor::Black;

		TriangleB.V1_Pos = TriangleA.V0_Pos;
		TriangleB.V1_UV = TriangleA.V0_UV;
		TriangleB.V1_Color = TriangleA.V0_Color;

		TriangleB.V2_Pos = TriangleA.V2_Pos;
		TriangleB.V2_UV = TriangleA.V2_UV;
		TriangleB.V2_Color = TriangleA.V2_Color;

		ShadowTriangles.Add(TriangleA);
		ShadowTriangles.Add(TriangleB);

	}

	if (ShadowTriangles.Num() == 0)
		return;

	FCanvasTriangleItem ShadowTrianglesItem{ ShadowTriangles, GWhiteTexture };

	if (FogOfWarVisionShadows.GetValueOnGameThread())
		Canvas.DrawItem(ShadowTrianglesItem);
}

double UFogOfWarVisionComponent::GetHalfFOVInRadians() const
{
	return FMath::Abs(FMath::DegreesToRadians(FMath::Clamp(VisionFieldOfViewDeg * 0.5, 0.0, 180.0)));
}


void UFogOfWarVisionComponent::UpdateVisionOverlaps()
{
	QUICK_SCOPE_CYCLE_COUNTER(UpdateVisionOverlaps);

	auto Subsystem = GetWorld()->GetSubsystem<UFogOfWarSubsystem>();

	auto OldVisibleActors = MoveTemp(VisibleActors);

	TArray<AActor*> NewlyVisibleActors;

	auto VisionCanvasBounds = GetVisionCanvasBounds();

	if (FogOfWarVisionDebug.GetValueOnGameThread())
	{
		FVector2D Center, Extent;

		VisionCanvasBounds.GetCenterAndExtents(Center, Extent);

		DrawDebugBox(GetWorld(), FVector{ Center, 150.0 }, FVector{ Extent, 1.0}, FColor::Magenta, false, GetWorld()->GetDeltaSeconds() * 1.05f);
	}

	for (auto It = Subsystem->GetFogOfWarActors().WorldBoxQuery(VisionCanvasBounds); It; ++It)
	{
		if (!It->IsValid())
			continue;

		auto Actor = It->Get();

		bool bActorVisible = IFogOfWarActor::Execute_IsFogOfWarVisible(Actor, this);

		if (bActorVisible)
		{
			VisibleActors.Add(Actor);

			if (!OldVisibleActors.Contains(Actor))
				NewlyVisibleActors.Add(Actor);
			
		}
	}

	bool bVisibleActorsChanged = NewlyVisibleActors.Num() > 0;

	for (auto Actor : OldVisibleActors)
		if (!VisibleActors.Contains(Actor))
		{
			bVisibleActorsChanged = true;

			if (IsValid(Actor))
				IFogOfWarActor::Execute_SetFogOfWarVisionVisibility(Actor, this, false);
		}

	for (auto Actor : NewlyVisibleActors)
		IFogOfWarActor::Execute_SetFogOfWarVisionVisibility(Actor, this, true);

	if (bVisibleActorsChanged)
		OnVisibleActorsUpdated.Broadcast(this, OldVisibleActors, VisibleActors);

}

//void UFogOfWarVisionComponent::SetFogOfWarManager(AFogOfWarManager* NewManager)
//{
//	if (IsValid(Manager))
//		Manager->UnregisterVisionComponent(this);
//
//	Manager = NewManager;
//
//	if (IsValid(Manager))
//	{
//		Manager->RegisterVisionComponent(this);
//	}
//}

void UFogOfWarVisionComponent::ClearVisibleActors()
{
	auto OldVisibleActors = MoveTemp(VisibleActors);

	for (auto Actor : OldVisibleActors)
	{
		if (!Actor)
			continue;

		IFogOfWarActor::Execute_SetFogOfWarVisionVisibility(Actor, this, false);
	}

	OnVisibleActorsUpdated.Broadcast(this, OldVisibleActors, VisibleActors);
}


