//Copyright Jarrad Alexander 2022


#include "PrototypingMaze.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "BaseUnit.h"
#include "DrawDebugHelpers.h"

// Sets default values
APrototypingMaze::APrototypingMaze()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	auto Root = CreateDefaultSubobject<USceneComponent>("Root");

	Root->SetMobility(EComponentMobility::Static);

	SetRootComponent(Root);
	
	//WallMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>("WallMeshes");

	//WallMeshes->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void APrototypingMaze::BeginPlay()
{
	Super::BeginPlay();

	InitCells();
	MakeVoronoiCells();
	CalculateAdjacency();
	JoinCells();
	InsertDoors();
	BuildWalls();
	SpawnEnemies();

	if (bDrawDebug)
		DrawDebugCells();
}

// Called every frame
void APrototypingMaze::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FPrototypingMazeCell& APrototypingMaze::GetCell(FVector Location)
{
	auto Local = GetActorTransform().InverseTransformPosition(Location);

	return GetCell({ FMath::FloorToInt32(Local.X), FMath::FloorToInt32(Local.Y) });
}

FPrototypingMazeCell& APrototypingMaze::GetCell(FIntPoint Location)
{
	int32 X = FMath::Clamp(Location.X, 0, MazeSize.X - 1);
	int32 Y = FMath::Clamp(Location.Y, 0, MazeSize.Y - 1);
	return Cells[X + Y * MazeSize.X];
}

FVector APrototypingMaze::GetCellWorldLocation(FIntPoint Location) const
{
	return GetActorTransform().TransformPosition(FVector{ Location });
}

void APrototypingMaze::InitCells()
{
	Cells.SetNum(MazeSize.X * MazeSize.Y);
}

void APrototypingMaze::MakeVoronoiCells()
{

	VoronoiCells.SetNum(NumVoronoiCells);
	
	for (int32 i = 0; i < NumVoronoiCells; ++i)
	{
		VoronoiCells[i].Label = i;
		VoronoiCells[i].Location = FVector2D{ (double)RandomStream.RandRange(0, MazeSize.X), (double)RandomStream.RandRange(0, MazeSize.Y) };
	}

	for (int32 Y = 0; Y < MazeSize.Y; ++Y)
		for (int32 X = 0; X < MazeSize.X; ++X)
		{
			FIntPoint Location{ X, Y };
			auto& Cell = GetCell(Location);

			Cell.Label = -1;

			TOptional<double> MinDistSqr;
			int32 MinIndex = -1;

			for (int32 i = 0; i < VoronoiCells.Num(); ++i)
			{
				//Euclidean
				auto DistSqr = FVector2D::DistSquared(FVector2D{(double)X, (double)Y}, VoronoiCells[i].Location);

				//Manhattan
				//double DistSqr = FMath::Abs(X - VoronoiPoints[i].X) + FMath::Abs(Y - VoronoiPoints[i].Y);

				if (MinDistSqr && DistSqr >= *MinDistSqr)
					continue;

				MinDistSqr = DistSqr;
				MinIndex = i;
			}

			if (!MinDistSqr)
				continue;

			Cell.Label = VoronoiCells[MinIndex].Label;
			VoronoiCells[MinIndex].Cells.Add(Location);
		}
	
}

void APrototypingMaze::CalculateAdjacency()
{
	for (auto& VoronoiCell : VoronoiCells)
		VoronoiCell.Adjacency.Reset();


	for (auto& VoronoiCell : VoronoiCells)
	{
		for (auto Location : VoronoiCell.Cells)
		{
			auto& Cell = GetCell(Location);

			auto CheckNeighbourCell = [&](FIntPoint OtherCellLocation)
			{
				auto& OtherCell = GetCell(OtherCellLocation);

				if (OtherCell.Label == Cell.Label)
					return;

				VoronoiCell.Adjacency.FindOrAdd(OtherCell.Label).Add({ Location, OtherCellLocation });
			};

			CheckNeighbourCell(Location + FIntPoint{ 1, 0 });
			CheckNeighbourCell(Location + FIntPoint{ 0, 1 });
			CheckNeighbourCell(Location + FIntPoint{ -1, 0 });
			CheckNeighbourCell(Location + FIntPoint{ 0, -1 });


		}

	}

}

void APrototypingMaze::JoinCells()
{
	//Since voronoi labels are uniformly distributed, we can just scale down each label to join regions.
	//Note that this does not actually join adjacent cells, it just gives a chance that two cells were next to each other and now have the same label
	//for (auto& Cell : Cells)
	//	Cell.Label /= VoronoiJoin;

	auto JoinCell = [&](FPrototypingMazeVoronoiCell& This, FPrototypingMazeVoronoiCell& Other)
	{
		if (This.Cells.Num() == 0 || Other.Cells.Num() == 0)
			return;

		if (bDrawDebug)
		{
			DrawDebugDirectionalArrow(GetWorld(),
				GetActorTransform().TransformPosition(FVector{ Other.Location, 0.0 }) + FVector::ZAxisVector * 400.f,
				GetActorTransform().TransformPosition(FVector{ This.Location, 0.0 }) + FVector::ZAxisVector * 400.f,
				100.f, FColor::Magenta, true, -1.f, 0, 20.f);
		}

		//Label the cells with this label
		for (auto& CellLocation : Other.Cells)
			GetCell(CellLocation).Label = This.Label;

		//Remove edge between joining cells
		This.Adjacency.Remove(Other.Label);
		Other.Adjacency.Remove(This.Label);

		//Make adjacent cells of the other point to this now
		for (auto& [OtherAdjacentLabel, OtherAdjacentEdges] : Other.Adjacency)
		{
			auto& OtherAdjacentCell = VoronoiCells[OtherAdjacentLabel];

			auto AdjacentCells = MoveTemp(OtherAdjacentCell.Adjacency[Other.Label]);

			OtherAdjacentCell.Adjacency.Remove(Other.Label);

			OtherAdjacentCell.Adjacency.FindOrAdd(This.Label).Append(AdjacentCells);

			This.Adjacency.FindOrAdd(OtherAdjacentLabel).Append(OtherAdjacentEdges);
		}

		//Take adjacent cells
		This.Cells.Append(Other.Cells);

		//Empty the other
		Other.Cells.Empty();
		Other.Adjacency.Empty();
	};

	for (auto& VoronoiCell : VoronoiCells)
	{
		TArray<int32> Joins;

		for (auto& [AdjacentLabel, AdjacentEdges] : VoronoiCell.Adjacency)
			if (RandomStream.GetFraction() < VoronoiJoinChance)
				Joins.Add(AdjacentLabel);

		for (auto AdjacentLabel : Joins)
			JoinCell(VoronoiCell, VoronoiCells[AdjacentLabel]);
	}

}

void APrototypingMaze::InsertDoors()
{

	for (auto& VoronoiCell : VoronoiCells)
	{
		for (auto& [AdjacentLabel, AdjacentEdges] : VoronoiCell.Adjacency)
		{
			if (AdjacentLabel < VoronoiCell.Label)
				//doors are symmetric, so only one of the cells on the edge needs to add it
				continue;

			if (AdjacentEdges.Num() == 0)
				//It shouldn't be in here but check just in case.
				continue;

			int32 NumDoors = FMath::Max(1, AdjacentEdges.Num() * DoorChance);

			for (int32 i = 0; i < NumDoors; ++i)
			{
				auto DoorIndex = RandomStream.RandHelper(AdjacentEdges.Num() - 1);
				
				Doors.Add(AdjacentEdges[DoorIndex]);
			}
			//for (auto& Edge : AdjacentEdges)
			//{
			//	if (RandomStream.GetFraction() > DoorChance)
			//		continue;

			//	Doors.Add(Edge);
			//}
		}
	}

	//auto TryInsertDoor = [&](FPrototypingMazeCell& Center, FPrototypingMazeCell& Other, bool& CenterFlag, bool& OtherFlag)
	//{
	//	if (Center.Label == Other.Label)
	//		return;

	//	if (Center.bXNDoor || Center.bXPDoor || Center.bYNDoor || Center.bYPDoor)
	//		return;

	//	if (RandomStream.GetFraction() > DoorChance)
	//		return;

	//	CenterFlag = true;
	//	OtherFlag = true;
	//	
	//};

	//for (int32 Y = 0; Y < MazeSize.Y - 1; ++Y)
	//	for (int32 X = 0; X < MazeSize.X - 1; ++X)
	//	{
	//		FIntPoint Location{ X,Y };

	//		auto& Cell = GetCell(Location);
	//		auto& XPCell = GetCell(Location + FIntPoint{ 1, 0 });
	//		auto& YPCell = GetCell(Location + FIntPoint{ 0, 1 });

	//		TryInsertDoor(Cell, XPCell, Cell.bXPDoor, XPCell.bXNDoor);
	//		TryInsertDoor(Cell, YPCell, Cell.bYPDoor, YPCell.bYNDoor);

	//	}
}

void APrototypingMaze::BuildWalls()
{
	//WallMeshes->ClearInstances();

	auto BuildWall = [&](FIntPoint CenterLocation, FIntPoint OtherDirection)
	{

		FVector Location = FVector{ CenterLocation } + FVector{ OtherDirection } * 0.5;

		FRotator Rotation{ 0.0, FMath::RadiansToDegrees(FMath::Atan2((double)OtherDirection.Y, (double)OtherDirection.X)), 0.0 };

		//Scale unit box mesh to be a wall
		//FVector Scale{ 0.001, 0.01, 0.01 };

		FTransform Transform{ Rotation, Location, WallMeshScale };

		auto MeshComponent = NewObject<UStaticMeshComponent>(this);

		MeshComponent->SetStaticMesh(WallMesh);

		MeshComponent->SetMaterial(0, WallMaterial);

		MeshComponent->SetupAttachment(GetRootComponent());

		MeshComponent->SetRelativeTransform(Transform);

		MeshComponent->SetCollisionProfileName("BlockAll");

		MeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);

		MeshComponent->SetMobility(EComponentMobility::Static);

		MeshComponent->RegisterComponent();



		//WallMeshes->AddInstance(Transform);

	};

	for (auto& VoronoiCell : VoronoiCells)
	{
		for (auto& [AdjacentLabel, AdjacentEdges] : VoronoiCell.Adjacency)
		{
			for (auto& [From, To] : AdjacentEdges)
			{
				if (Doors.Contains({ From,To }) || Doors.Contains({ To, From }))
					continue;

				BuildWall(From, To - From);
			}
		}
	}

	for (int32 i = 0; i < MazeSize.X; ++i)
		BuildWall({ i, 0 }, { 0, -1 });

	for (int32 i = 0; i < MazeSize.X; ++i)
		BuildWall({ i, MazeSize.Y - 1 }, { 0, 1 });

	for (int32 i = 0; i < MazeSize.Y; ++i)
		BuildWall({ 0, i }, { -1, 0 });

	for (int32 i = 0; i < MazeSize.Y; ++i)
		BuildWall({ MazeSize.Y - 1, i }, { 1, 0 });

	//for (int32 Y = 0; Y < MazeSize.Y; ++Y)
	//	for (int32 X = 0; X < MazeSize.X; ++X)
	//	{
	//		FIntPoint Location{ X,Y };
	//		
	//		auto& Cell = GetCell(Location);
	//		auto& XPCell = GetCell(Location + FIntPoint{ 1, 0 });
	//		auto& YPCell = GetCell(Location + FIntPoint{ 0, 1 });

	//		if (X == 0)
	//			BuildWall(Location, { -1, 0 });

	//		if (Y == 0)
	//			BuildWall(Location, { 0, -1 });

	//		if (X == MazeSize.X - 1 || Cell.Label != XPCell.Label && !Cell.bXPDoor)
	//			BuildWall(Location, { 1, 0 });
	//		
	//		if (Y == MazeSize.Y - 1 || Cell.Label != YPCell.Label && !Cell.bYPDoor)
	//			BuildWall(Location, { 0, 1 });

	//	}
}

void APrototypingMaze::SpawnEnemies()
{
	FActorSpawnParameters SP;

	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 RemainingEnemies = NumEnemies;

	for (int32 i = 0; i < NumEnemies * 2 && RemainingEnemies > 0; ++i)
	{
		auto CellLocation = ProduceRandomCell();
		
		FVector Location = GetActorTransform().TransformPosition(FVector{ CellLocation });

		auto& Cell = GetCell(CellLocation);

		if (Cell.EnemyCount > 0)
			continue;

		Cell.EnemyCount = FMath::Min(FMath::Max(0, RandomStream.RandRange(EnemyClusterSize * 0.6, EnemyClusterSize * 1.4)), RemainingEnemies);

		RemainingEnemies -= Cell.EnemyCount;

		if (EnemyUnitClass.Get())
		{
			for (int32 j = 0; j < Cell.EnemyCount; ++j)
			{
				auto Angle = double(j) / Cell.EnemyCount * UE_PI * 2.0;

				auto Unit = GetWorld()->SpawnActor<ABaseUnit>(EnemyUnitClass.Get(), Location + FVector{ FMath::Cos(Angle) * 40.0, FMath::Sin(Angle) * 40.0, 100.0 }, FRotator::ZeroRotator, SP);
			}
		}

	}

}

void APrototypingMaze::DrawDebugCells()
{
	for (auto& VoronoiCell : VoronoiCells)
	{
		FColor Color = FColor::MakeRandomSeededColor(VoronoiCell.Label);

		for (auto CellLocation : VoronoiCell.Cells)
		{
			auto& Cell = GetCell(CellLocation);

			FVector Location{ CellLocation };

			Location = GetActorTransform().TransformPosition(Location);

			DrawDebugPoint(GetWorld(), Location, 30.f, Color, true);
		}

		for (auto& [OtherLabel, AdjacentCells] : VoronoiCell.Adjacency)
			for (auto& [From, To] : AdjacentCells)
			{

				DrawDebugDirectionalArrow(GetWorld(), GetCellWorldLocation(From), FMath::Lerp(GetCellWorldLocation(From), GetCellWorldLocation(To), 0.5), 30.f, Color, true, -1.f, 0, 10.f);

			}

	}

	for (auto [From, To] : Doors)
	{
		DrawDebugLine(GetWorld(), GetCellWorldLocation(From) + FVector::ZAxisVector * 100.f, GetCellWorldLocation(To) + FVector::ZAxisVector * 100.f, FColor::Blue, true, -1.f, 0, 20.f);
	}


	//for (int32 Y = 0; Y < MazeSize.Y; ++Y)
	//	for (int32 X = 0; X < MazeSize.X; ++X)
	//	{
	//		auto& Cell = GetCell({ X,Y });

	//		FVector Location{ (double)X, (double)Y, 0 };

	//		Location = GetActorTransform().TransformPosition(Location);

	//		FColor Color = FColor::MakeRandomSeededColor(Cell.Label);

	//		DrawDebugPoint(GetWorld(), Location, 30.f, Color, true);
	//		
	//		//if (Cell.bXPDoor)
	//		//	DrawDebugDirectionalArrow(GetWorld(), Location, Location + FVector{ GetActorScale().X * 0.5, 0.0, 0.0 }, 30.f, FColor::Green, true);

	//		//if (Cell.bYPDoor)
	//		//	DrawDebugDirectionalArrow(GetWorld(), Location, Location + FVector{ 0.0, GetActorScale().Y * 0.5, 0.0 }, 30.f, FColor::Green, true);

	//		//if (Cell.bXNDoor)
	//		//	DrawDebugDirectionalArrow(GetWorld(), Location, Location - FVector{ GetActorScale().X * 0.5, 0.0, 0.0 }, 30.f, FColor::Red, true);

	//		//if (Cell.bYNDoor)
	//		//	DrawDebugDirectionalArrow(GetWorld(), Location, Location - FVector{ 0.0, GetActorScale().Y * 0.5, 0.0 }, 30.f, FColor::Red, true);
	//	}



}

FIntPoint APrototypingMaze::ProduceRandomCell()
{
	return FIntPoint{ RandomStream.RandHelper(MazeSize.X), RandomStream.RandHelper(MazeSize.Y) };
}

