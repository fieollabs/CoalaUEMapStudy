// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaTaskCreateBuildings.h"
#include "CoalaMeshGenerator.h"
#include "KismetProceduralMeshLibrary.h"
#include "CoalaConverter.h"
#include "CoalaArea.h"


//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//		This line is essential! Compiler error without it
FCoalaTaskCreateBuildings* FCoalaTaskCreateBuildings::Runnable = NULL;
//***********************************************************

FCoalaTaskCreateBuildings::FCoalaTaskCreateBuildings(ACoalaMeshActor* ret, TArray<UCoalaBuilding*> buildings, int32 createMeshes, FVector area_fixpoint, UProceduralMeshComponent* out,  UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, bool generateCollisions, bool stretchUVs, float heightPerLevel)
{
	_out = out;
	_buildings = buildings;
	_area_fixpoint = area_fixpoint;
	isDone = false;
	_ret = ret;
	_materialFloor = materialFloor;
	_materialWall = materialWall;
	_materialRoof = materialRoof;
	_generateUVs = generateUVs;
	_createMeshes = createMeshes;
	_generateCollisions = generateCollisions;
	_stretchUVs = stretchUVs;
	_heightPerLevel = heightPerLevel;

	//Link to where data should be stored
	Thread = FRunnableThread::Create(this, TEXT("FCoalaTaskCreateBuildings"), 0, TPri_BelowNormal);
}

FCoalaTaskCreateBuildings::~FCoalaTaskCreateBuildings()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FCoalaTaskCreateBuildings::Init()
{
	//Init the Data 

	return true;
}
//Run
uint32 FCoalaTaskCreateBuildings::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.0001);


	generateMesh(_ret, _buildings, _createMeshes, _area_fixpoint, _out, _materialFloor, _materialWall, _materialRoof, _generateUVs, _generateCollisions, _stretchUVs, _heightPerLevel);
	//MeshGenerator::generateMesh( ret->mesh, shapes, holes, height, mesh_section_index_to_create, true, true, true);
	//_out->SetMaterial(mesh_section_index_to_create, material);

	isDone = true;
	return 0;
}

//stop
void FCoalaTaskCreateBuildings::Stop()
{
	StopTaskCounter.Increment();
}

FCoalaTaskCreateBuildings* FCoalaTaskCreateBuildings::JoyInit(ACoalaMeshActor* ret, TArray<UCoalaBuilding*> buildings, int32 createMeshes, FVector area_fixpoint, UProceduralMeshComponent* out,  UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, bool generateCollisions, bool stretchUVs, float heightPerLevel)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FCoalaTaskCreateBuildings( ret, buildings, createMeshes, area_fixpoint, out, materialFloor, materialWall, materialRoof, generateUVs, generateCollisions,stretchUVs, heightPerLevel);
	}
	return Runnable;
}

void FCoalaTaskCreateBuildings::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}



void FCoalaTaskCreateBuildings::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

ACoalaMeshActor*
FCoalaTaskCreateBuildings::GetResult()
{
	return _ret;
}

bool FCoalaTaskCreateBuildings::IsThreadFinished()
{
	if (Runnable) return Runnable->IsFinished();
	return false;
}
void FCoalaTaskCreateBuildings::generateMesh(ACoalaMeshActor* ret, TArray<UCoalaBuilding*> buildings, int32 createMeshes, FVector area_fixpoint, UProceduralMeshComponent* out,  UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, bool generateCollisions, bool stretchUVs, float heightPerLevel)
{

	TArray<TArray<FVector>> shapes;
	TArray<TArray<TArray<FVector>>> holes;
	TArray<int32> height;

	//array with no heights to give to the floor
	TArray<int32> emptyHeights;

	// transform shape from gps to pixel coodinates
	{
		for (int i = 0; i < buildings.Num(); ++i)
		{
			UCoalaBuilding* current = buildings[i];
			
			//FVector center = CoalaConverter::ToScenePosition(current->center()->lon, current->center()->lat);
			FVector center = FVector::ZeroVector;
			FVector position_relative_to_area = area_fixpoint - center;

			height.Add(heightPerLevel * current->height);
			emptyHeights.Add(0);

			TArray<FVector> shape;
			for (int a = 0; a < current->area.Num(); ++a)
			{
				shape.Add(CoalaConverter::ToScenePosition(current->area[a]->lon, current->area[a]->lat) - area_fixpoint);
			}
			shapes.Add(shape);

			TArray<TArray<FVector>> buildingHoles;
			for (int b = 0; b < current->holes.Num(); ++b)
			{
				TArray<UCoalaGPSCoordinates*> currentHole_org = current->holes[b];
				TArray<FVector> currentHole;
				for (int c = 0; c < currentHole_org.Num(); ++c)
				{
					currentHole.Add(CoalaConverter::ToScenePosition(currentHole_org[c]->lon, currentHole_org[c]->lat) - area_fixpoint);
				}
				buildingHoles.Add(currentHole);
			}
			holes.Add(buildingHoles);
			current->ClearInternalFlags(EInternalObjectFlags::Async);
		}
	}
	int sectionIndexToCreate = 0;
	// 1 - floors
	if (createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::FLOOR)
	{
		MeshGenerator::generateMesh(out, shapes, holes, emptyHeights, sectionIndexToCreate, generateUVs, generateCollisions, false);
		if (materialFloor)
		{
			out->SetMaterial(sectionIndexToCreate, materialFloor);
		}
		++sectionIndexToCreate;
	}

	// 2 - all walls
	if (createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::WALLS)
	{
		MeshGenerator::generateSingleWallMesh(out, shapes, holes, height, sectionIndexToCreate, generateUVs, false, generateCollisions);
		if (materialWall)
		{
			AsyncTask(ENamedThreads::GameThread, [=]() {
				out->SetMaterial(sectionIndexToCreate, materialWall);
			});
		}
		++sectionIndexToCreate;
	}

	// 3 - roofs
	if (createMeshes & (int32)OPTIONS_MESH_CREATION_BUILDING::ROOF)
	{
		//MeshGenerator::generateSingleMesh(out, shapes, holes, height, sectionIndexToCreate, generateUVs, false, generateCollisions);
		MeshGenerator::generateMesh(out, shapes, holes, height, sectionIndexToCreate, generateUVs, generateCollisions, false);
		if (materialRoof)
		{
			AsyncTask(ENamedThreads::GameThread, [=]() {
				out->SetMaterial(sectionIndexToCreate, materialRoof);
			});
		}
	}
}