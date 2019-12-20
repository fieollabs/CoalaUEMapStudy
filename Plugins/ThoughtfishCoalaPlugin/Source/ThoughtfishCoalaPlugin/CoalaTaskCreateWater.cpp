// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaTaskCreateWater.h"
#include "CoalaMeshGenerator.h"
#include "KismetProceduralMeshLibrary.h"
#include "CoalaBlueprintUtility.h"
#include "CoalaArea.h"

//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//		This line is essential! Compiler error without it
FCoalaTaskCreateWater* FCoalaTaskCreateWater::Runnable = NULL;
//***********************************************************

FCoalaTaskCreateWater::FCoalaTaskCreateWater(ACoalaMeshActor* ret, UCoalaWater* water, FVector area_fixpoint, UProceduralMeshComponent* out, FVector center, UMaterialInterface* material, bool generateUVs, float outlineWidth, UMaterialInterface* outlineMaterial)
{
	_out = out;
	_water = water;
	_area_fixpoint = area_fixpoint;
	isDone = false;
	_ret = ret;
	_center = center;
	_material = material;
	_generateUVs = generateUVs;
	_outlineWidth = outlineWidth;
	_outlineMaterial = outlineMaterial;
	//Link to where data should be stored
	Thread = FRunnableThread::Create(this, TEXT("FCoalaTaskCreateWater"), 0, TPri_BelowNormal);
}

FCoalaTaskCreateWater::~FCoalaTaskCreateWater()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FCoalaTaskCreateWater::Init()
{
	//Init the Data 

	return true;
}
//Run
uint32 FCoalaTaskCreateWater::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.0001);


	generateMesh(_ret, _water, _area_fixpoint, _out, _center, _material, _generateUVs, _outlineWidth, _outlineMaterial);
	//MeshGenerator::generateMesh( ret->mesh, shapes, holes, height, mesh_section_index_to_create, true, true, true);
	//_out->SetMaterial(mesh_section_index_to_create, material);

	isDone = true;
	return 0;
}

//stop
void FCoalaTaskCreateWater::Stop()
{
	StopTaskCounter.Increment();
}

FCoalaTaskCreateWater* FCoalaTaskCreateWater::JoyInit(ACoalaMeshActor* ret, UCoalaWater* water, FVector area_fixpoint, UProceduralMeshComponent* out, FVector center, UMaterialInterface* material, bool generateUVs, float outlineWidth, UMaterialInterface* outlineMaterial)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FCoalaTaskCreateWater(ret, water, area_fixpoint, out, center, material, generateUVs, outlineWidth, outlineMaterial);
	}
	return Runnable;
}

void FCoalaTaskCreateWater::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}



void FCoalaTaskCreateWater::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

ACoalaMeshActor*
FCoalaTaskCreateWater::GetResult()
{
	return _ret;
}

bool FCoalaTaskCreateWater::IsThreadFinished()
{
	if (Runnable) return Runnable->IsFinished();
	return false;
}
void FCoalaTaskCreateWater::generateMesh(ACoalaMeshActor* ret, UCoalaWater* water, FVector area_fixpoint, UProceduralMeshComponent* out, FVector center, UMaterialInterface* material, bool generateUVs, float outlineWidth, UMaterialInterface* outlineMaterial)
{
	TArray<FVector> shape;
	TArray<TArray<FVector> > holes;
	{
		for (int i = 0; i < water->area.Num(); ++i)
		{
			UCoalaGPSCoordinates* current = water->area[i];
			shape.Add(CoalaConverter::ToScenePosition(current->lon, current->lat) - center);
		}

		for (int i = 0; i < water->holes.Num(); ++i)
		{
			TArray<UCoalaGPSCoordinates*> currentHole_org = water->holes[i];
			TArray<FVector> currentHole;
			for (int a = 0; a < currentHole_org.Num(); ++a)
			{
				UCoalaGPSCoordinates* current = currentHole_org[a];
				currentHole.Add(CoalaConverter::ToScenePosition(current->lon, current->lat) - center);
			}
			holes.Add(currentHole);
		}

		MeshGenerator::generateMesh(out, shape, holes, 0, 0, generateUVs, true, false);
		AsyncTask(ENamedThreads::GameThread, [out, material]() {
		if (material)
			out->SetMaterial(0, material);
		});
	}
	if (outlineWidth != 0.0f)
	{
		float outline_width = outlineWidth * UCoalaBlueprintUtility::GetCoalaScale();

		TArray<FVector> shape_outline_mesh;
		TArray<TArray<FVector> > holes_outline_mesh;

		MeshGenerator::generateOutlineMesh_v2(shape, holes, outline_width, shape_outline_mesh, holes_outline_mesh);

		MeshGenerator::generateMesh(out, shape_outline_mesh, holes_outline_mesh, 1, -1, generateUVs, true, false);
		AsyncTask(ENamedThreads::GameThread, [out,outlineMaterial]() {
			if (outlineMaterial)
				out->SetMaterial(1, outlineMaterial);
		});
	}
	
}