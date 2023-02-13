//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h" 
//#include "FogOfWarCommon.generated.h"


static constexpr ESPMode FogOfWarSPMode = ESPMode::NotThreadSafe;

template <typename T>
using TFogOfWarSharedPtr = TSharedPtr<T, FogOfWarSPMode>;

struct ZOMBIES_API FFogOfWarOccluderMesh : TSharedFromThis<FFogOfWarOccluderMesh, FogOfWarSPMode>
{

	//Mesh space edges that should cast shadows
	TArray<TPair<FVector2D, FVector2D>> ShadowEdges;

	//Mesh space bounding box
	FBox2D Bounds;

	//Possibly not needed...
	//The triangles that make up the occluder itself.
	//Only really necessary when the vision source is inside the geometry itself, since shadows would be projected outwards in that case.
	//TArray<FCanvasUVTri> OccluderTriangles;
};

struct ZOMBIES_API FFogOfWarOccluderInstance
{
	//World space transform of instance (Shadow edges will be projected on to XY plane)
	FTransform Transform;

	//Mesh data
	TFogOfWarSharedPtr<FFogOfWarOccluderMesh> OccluderMesh;
};