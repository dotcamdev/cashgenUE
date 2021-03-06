#include "cashgen.h"
#include "ZoneManager.h"
#include "WorldManager.h"
#include "MeshData.h"
#include "FZoneGeneratorWorker.h"

AZoneManager::AZoneManager()
{
	PrimaryActorTick.bCanEverTick = true;
	USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	RootComponent = SphereComponent;

	// A proc mesh component for each of the LOD levels - will make this configurable
	MyProcMeshComponents.Add(0, CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh0")));
	MyProcMeshComponents.Add(1, CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh1")));
	MyProcMeshComponents.Add(2, CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh2")));
	MyProcMeshComponents.Add(3, CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh3")));

	this->SetActorEnableCollision(true);
	MyProcMeshComponents[0]->AttachTo(RootComponent);
	MyProcMeshComponents[1]->AttachTo(RootComponent);
	MyProcMeshComponents[2]->AttachTo(RootComponent);
	MyProcMeshComponents[3]->AttachTo(RootComponent);
	MyProcMeshComponents[0]->BodyInstance.SetResponseToAllChannels(ECR_Block);
	MyProcMeshComponents[1]->BodyInstance.SetResponseToAllChannels(ECR_Block);
	MyProcMeshComponents[2]->BodyInstance.SetResponseToAllChannels(ECR_Block);
	MyProcMeshComponents[3]->BodyInstance.SetResponseToAllChannels(ECR_Block);

	// LOD 10 = do not render
	currentlyDisplayedLOD = 10;
}

// Initial setup of the zone
void AZoneManager::SetupZone(const int32 aZoneID, AWorldManager* aWorldManager, const Point aOffset, const FZoneConfig aZoneConfig, FVector* aWorldOffset)
{
	// The world grid offset of this zone
	MyOffset.x	= aOffset.x;
	MyOffset.y	= aOffset.y;

	// The full world offset (always apply this)
	worldOffset = aWorldOffset;
	// Config, manager pointers etc.
	MyConfig	= aZoneConfig;
	MyWorldManager = aWorldManager;
	MyZoneID = aZoneID;	
}

// Populates the mesh data structures for a given LOD
void AZoneManager::PopulateMeshData(const uint8 aLOD)
{
	int32 numXVerts = aLOD == 0 ? MyConfig.XUnits + 1 : (MyConfig.XUnits / (FMath::Pow(2, aLOD))) + 1;
	int32 numYVerts = aLOD == 0 ? MyConfig.YUnits + 1 : (MyConfig.YUnits / (FMath::Pow(2, aLOD))) + 1;

	MyLODMeshData.Add(aLOD, FMeshData());
	// Generate the per vertex data sets
	for (int32 i = 0; i < (numXVerts * numYVerts); ++i)
	{
		MyLODMeshData[aLOD].MyVertices.Add(FVector(i * 1.0f, i * 1.0f, 0.0f));
		MyLODMeshData[aLOD].MyNormals.Add(FVector(0.0f, 0.0f, 1.0f));
		MyLODMeshData[aLOD].MyUV0.Add(FVector2D(0.0f, 0.0f));
		MyLODMeshData[aLOD].MyVertexColors.Add(FColor::Black);
		MyLODMeshData[aLOD].MyTangents.Add(FProcMeshTangent(0.0f, 0.0f, 0.0f));
	}

	// Heightmap needs to be larger than the mesh
	// Using vectors here is a bit wasteful, but it does make normal/tangent or any other
	// Geometric calculations based on the heightmap a bit easier. Easy enough to change to floats
	for (int32 i = 0; i < (numXVerts + 2) * (numYVerts + 2); ++i)
	{
		MyLODMeshData[aLOD].MyHeightMap.Add(FVector(0.0f));
	}

	// Triangle indexes
	for (int32 i = 0; i < (numXVerts - 1) * (numYVerts - 1) * 6; ++i)
	{
		MyLODMeshData[aLOD].MyTriangles.Add(i);
	}

	CalculateTriangles(aLOD);
}


// Calculate the triangles and UVs for this LOD
// TODO: Create proper UVs
void AZoneManager::CalculateTriangles(const uint8 aLOD)
{
	int32 triCounter = 0;
	int32 thisX, thisY;
	int32 rowLength;

	rowLength = aLOD == 0 ? MyConfig.XUnits + 1 : (MyConfig.XUnits / (FMath::Pow(2, aLOD)) + 1);
	float maxUV = aLOD == 0 ? 1.0f : 1.0f / aLOD;
	
	int32 exX = aLOD == 0 ? MyConfig.XUnits : (MyConfig.XUnits / (FMath::Pow(2, aLOD)));
	int32 exY = aLOD == 0 ? MyConfig.YUnits : (MyConfig.YUnits / (FMath::Pow(2, aLOD)));

	for (int32 y = 0; y < exY ; ++y)
	{
		for (int32 x = 0; x < exX; ++x)
		{

			thisX = x;
			thisY = y;
			//TR
			(MyLODMeshData[aLOD].MyTriangles)[triCounter] = thisX + ((thisY + 1) * (rowLength));
			triCounter++;
			//BL
			(MyLODMeshData[aLOD].MyTriangles)[triCounter] = (thisX + 1) + (thisY * (rowLength));
			triCounter++;
			//BR
			(MyLODMeshData[aLOD].MyTriangles)[triCounter] = thisX + (thisY * (rowLength));
			triCounter++;

			//BL
			(MyLODMeshData[aLOD].MyTriangles)[triCounter] = (thisX + 1) + (thisY * (rowLength));
			triCounter++;
			//TR
			(MyLODMeshData[aLOD].MyTriangles)[triCounter] = thisX + ((thisY + 1) * (rowLength));
			triCounter++;
			// TL
			(MyLODMeshData[aLOD].MyTriangles)[triCounter] = (thisX + 1) + ((thisY + 1) * (rowLength));
			triCounter++;

			//TR
			MyLODMeshData[aLOD].MyUV0[thisX + ((thisY + 1) * (rowLength))] = FVector2D(thisX * maxUV, (thisY+1.0f) * maxUV);
			//BR
			MyLODMeshData[aLOD].MyUV0[thisX + (thisY * (rowLength))] = FVector2D(thisX * maxUV, thisY * maxUV);
			//BL
			MyLODMeshData[aLOD].MyUV0[(thisX + 1) + (thisY * (rowLength))] = FVector2D((thisX + 1.0f) * maxUV, thisY * maxUV);
			//TL
			MyLODMeshData[aLOD].MyUV0[(thisX + 1) + ((thisY + 1) * (rowLength))] = FVector2D((thisX +1.0f)* maxUV, (thisY+1.0f) * maxUV);

		}
	}
}

// Main method for regenerating a zone
// Inplace update means the zone isn't moving it's just a LOD change (from 1 to 0)
void AZoneManager::RegenerateZone(const uint8 aLOD, const bool isInPlaceLODUpdate)
{
	if (!isInPlaceLODUpdate) {
		for (uint8 i = 0; i < MyProcMeshComponents.Num(); ++i)
		{
			MyProcMeshComponents[i]->SetMeshSectionVisible(0, false);
		}
	}
	currentlyDisplayedLOD = aLOD;

	if (aLOD != 10)
	{
		FString threadName = "ZoneWorker" + FString::FromInt(MyOffset.x) + FString::FromInt(MyOffset.y) + FString::FromInt(aLOD);

		// If we haven't used this LOD before, populate the data structures and apply the material
		if (!MyLODMeshData.Contains(aLOD))
		{
			MyLODMeshStatus.Add(aLOD, eLODStatus::BUILDING_REQUIRES_CREATE);
			PopulateMeshData(aLOD);
			MyProcMeshComponents[aLOD]->SetMaterial(0, MyConfig.TerrainMaterial);
		}
		else
		{
			MyLODMeshStatus[aLOD] = eLODStatus::BUILDING;
		}

		Thread = FRunnableThread::Create(new FZoneGeneratorWorker(this,
			&MyConfig,
			&MyOffset,
			&MyLODMeshStatus,
			aLOD,
			&MyLODMeshData[aLOD].MyVertices,
			&MyLODMeshData[aLOD].MyTriangles,
			&MyLODMeshData[aLOD].MyNormals,
			&MyLODMeshData[aLOD].MyUV0,
			&MyLODMeshData[aLOD].MyVertexColors,
			&MyLODMeshData[aLOD].MyTangents,
			&MyLODMeshData[aLOD].MyHeightMap),
			*threadName,
			0, TPri_BelowNormal);
	}

	SetActorLocation(FVector((MyConfig.XUnits * MyConfig.UnitSize * MyOffset.x) - worldOffset->X, (MyConfig.YUnits * MyConfig.UnitSize * MyOffset.y) - worldOffset->Y, 0.0f));
}


// Performs the creation/update of the Procedural Mesh Component
void AZoneManager::UpdateMesh(const uint8 aLOD)
{
	// Create the mesh sections if they haven't been done already
	if (MyLODMeshStatus[aLOD] == eLODStatus::DRAWING_REQUIRES_CREATE)
	{
		// Only generate collision if this is LOD0
		MyProcMeshComponents[aLOD]->CreateMeshSection(0, MyLODMeshData[aLOD].MyVertices, MyLODMeshData[aLOD].MyTriangles, MyLODMeshData[aLOD].MyNormals, MyLODMeshData[aLOD].MyUV0, MyLODMeshData[aLOD].MyVertexColors, MyLODMeshData[aLOD].MyTangents, aLOD == 0);
		MyLODMeshStatus[aLOD] = IDLE;
	}
	// Or just update them
	else if (MyLODMeshStatus[aLOD] == eLODStatus::DRAWING)
	{
		MyProcMeshComponents[aLOD]->UpdateMeshSection(0, MyLODMeshData[aLOD].MyVertices, MyLODMeshData[aLOD].MyNormals, MyLODMeshData[aLOD].MyUV0, MyLODMeshData[aLOD].MyVertexColors, MyLODMeshData[aLOD].MyTangents);
		MyLODMeshStatus[aLOD] = IDLE;

		if (aLOD == 0)
		{
			for (auto& InstancedStaticMeshComponent : MyInstancedMeshComponents)
			{
				InstancedStaticMeshComponent->ClearInstances();
			}
		}
	}

	// Now show the new section
	for (uint8 i = 0; i < MyProcMeshComponents.Num(); ++i)
	{
		if (i == aLOD) {
			MyProcMeshComponents[i]->SetMeshSectionVisible(0, true);
		}
		else {
			MyProcMeshComponents[i]->SetMeshSectionVisible(0, false);
		}
	}

}

// Called when the game starts or when spawned
void AZoneManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AZoneManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Run through each LOD we have on the zone
	for (auto& lod : MyLODMeshStatus)
	{
		// A thread has finished generating updated mesh data, and is ready to draw
		if (lod.Value == eLODStatus::READY_TO_DRAW)
		{
			lod.Value = eLODStatus::PENDING_DRAW;
			// Hand the thread token back to the world manager
			MyWorldManager->MyAvailableThreads.Enqueue(1);
			// Add a render task to the world manager
			MyWorldManager->MyRenderQueue.Enqueue(FZoneJob(MyZoneID, lod.Key, false));
			// Clean up
			delete Thread;
			Thread = NULL;
		}
		else if (lod.Value == eLODStatus::READY_TO_DRAW_REQUIRES_CREATE)
		{
			lod.Value = eLODStatus::PENDING_DRAW_REQUIRES_CREATE;
			// Hand the thread token back to the world manager
			MyWorldManager->MyAvailableThreads.Enqueue(1);
			// Add a render task to the world manager
			MyWorldManager->MyRenderQueue.Enqueue(FZoneJob(MyZoneID, lod.Key, false));
			// Clean up
			delete Thread;
			Thread = NULL;
		}
	}
}

// Return the location of the center of the zone
FVector AZoneManager::GetCentrePos()
{
	return  FVector((MyOffset.x * MyConfig.XUnits * MyConfig.UnitSize) - worldOffset->X, (MyOffset.y * MyConfig.YUnits * MyConfig.UnitSize) - worldOffset->Y, 0.0f);
}

AZoneManager::~AZoneManager()
{
	// GC will do this anyway, but won't hurt to clear them down
	MyLODMeshData.Empty();
	MyLODMeshStatus.Empty();
}