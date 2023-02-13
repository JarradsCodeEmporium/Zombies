//Copyright Jarrad Alexander 2022


#include "FogOfWarOccluderSubsystem.h"

TFogOfWarSharedPtr<FFogOfWarOccluderMesh> UFogOfWarOccluderSubsystem::GetOccluderMesh(UStaticMesh* StaticMesh)
{
	if (!StaticMesh)
		return nullptr;

	if (auto OccluderMesh = OccluderMeshes.Find(StaticMesh))
		return *OccluderMesh;

	if (!StaticMesh->bAllowCPUAccess)
	{
		FMessageLog("AssetCheck").Error(FText::FromString(FString::Printf(TEXT("Static Mesh asset %s is used as fog of war shadow occluder but does not have bAllowCPUAccess flag enabled"), *StaticMesh->GetPackage()->GetPathName())));

		//In editor, the mesh data is always available even if not allowing CPU access.
		//However, we still want to reject it so that we can see this mesh doesn't work before we package it and CPU access flag becomes relevant.
		return {};
	}

	auto OccluderMesh = MakeShared<FFogOfWarOccluderMesh, FogOfWarSPMode>();

	auto RenderData = StaticMesh->GetRenderData();

	if (!RenderData || RenderData->LODResources.Num() == 0)
		return OccluderMesh;

	//Cast shadows using LOD 0
	auto& LODResource = RenderData->LODResources[0];

	auto& PositionBuffer = LODResource.VertexBuffers.PositionVertexBuffer;
	auto& IndexBuffer = LODResource.IndexBuffer;

	TArray<FVector> Verts;
	TArray<int32> Indices;

	TSet<TPair<FVector2D, FVector2D>> Edges;

	for (auto& Section : LODResource.Sections)
	{
		for (uint32 TriangleIndex = 0; TriangleIndex < Section.NumTriangles; ++TriangleIndex)
		{
			uint32 CornerIndex = Section.FirstIndex + TriangleIndex * 3;

			//The actual indices that can be used to sample the vertex buffers like position, uv, normal, color, etc.
			uint32 VertexIndex[4] = 
			{
				IndexBuffer.GetIndex(CornerIndex + 0),
				IndexBuffer.GetIndex(CornerIndex + 1),
				IndexBuffer.GetIndex(CornerIndex + 2),
				IndexBuffer.GetIndex(CornerIndex + 0),
			};

			auto U = PositionBuffer.VertexPosition(VertexIndex[2]) - PositionBuffer.VertexPosition(VertexIndex[0]);
			auto V = PositionBuffer.VertexPosition(VertexIndex[1]) - PositionBuffer.VertexPosition(VertexIndex[0]);

			auto Normal = (U ^ V).GetSafeNormal();

			if (Normal.Z < 0.9f)
				//Reject triangles that don't face up
				continue;
			
			for (int32 i = 0; i < 3; ++i)
			{
				FVector2D From{ (FVector)PositionBuffer.VertexPosition(VertexIndex[i]) };
				FVector2D To{ (FVector)PositionBuffer.VertexPosition(VertexIndex[i + 1]) };

				Edges.Add({ From, To });

				//DrawDebugDirectionalArrow(GetWorld(), (FVector)PositionBuffer.VertexPosition(VertexIndex[i]), (FVector)PositionBuffer.VertexPosition(VertexIndex[i + 1]), 10.f, FColor::Green, true);

				//Verts.Add(FVector{ PositionBuffer.VertexPosition(VertexIndex[i]) });
				//Indices.Add(Indices.Num());
			}
		}
		//UE_LOG(LogTemp, Warning, TEXT("%d triangles"), Section.NumTriangles);
		//DrawDebugMesh(GetWorld(), Verts, Indices, FColor::Blue, true);

	}

	for (auto& [From, To] : Edges)
	{
		//Edge is a shadow casting boundary if there isn't another edge going in the opposite direction
		if (Edges.Contains({ To, From }))
			continue;

		OccluderMesh->ShadowEdges.Add({ From,To });

		OccluderMesh->Bounds += From;
		OccluderMesh->Bounds += To;

		//DrawDebugDirectionalArrow(GetWorld(), FVector{ From, 100.f }, FVector{ To, 100.f }, 10.f, Edges.Contains({ To, From }) ? FColor::Red : FColor::Green, true);

	}

	if (OccluderMesh->ShadowEdges.Num() > 0)
	{
		OccluderMeshes.Add(StaticMesh, OccluderMesh);

		return OccluderMesh;
	}
	else
		return nullptr;
}
