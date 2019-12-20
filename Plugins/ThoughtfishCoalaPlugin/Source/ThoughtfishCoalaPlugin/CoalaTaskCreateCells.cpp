// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaTaskCreateCells.h"
#include "libs/mapbox/earcut.h"
#include "KismetProceduralMeshLibrary.h"
#include "CoalaMeshGenerator.h"
#include "CoalaBlueprintUtility.h"
#include "CoalaAreaController.h"
#include <vector>


//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//		This line is essential! Compiler error without it
FCoalaTaskCreateCells* FCoalaTaskCreateCells::Runnable = NULL;
//***********************************************************

FCoalaTaskCreateCells::FCoalaTaskCreateCells( std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes, UProceduralMeshComponent* out, bool generateUVs, bool generateCollisions, bool stretchUVs, ACoalaMeshActor* ret)
{
	_out = out;
	grouped = grouped_shapes;
	_generateUVs = generateUVs;
	_generateCollisions = generateCollisions;
	_stretchUVs = stretchUVs;
	isDone = false;
	_ret = ret;
	//Link to where data should be stored
	Thread = FRunnableThread::Create(this, TEXT("FCoalaTaskCreateCells"), 0, TPri_BelowNormal); 
}

FCoalaTaskCreateCells::~FCoalaTaskCreateCells()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FCoalaTaskCreateCells::Init()
{
	//Init the Data 

	return true;
}
//Run
uint32 FCoalaTaskCreateCells::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.0001);

	TArray<int> height = { 0 };
	for (auto it = grouped->begin(); it != grouped->end(); ++it)
	{
		std::pair<int, UMaterialInterface*> current_index = it->first;
		std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > entry = it->second;
		TArray<TArray<FVector>> shapes = entry.first;
		TArray<TArray<TArray<FVector>>> holes = entry.second;

		int mesh_section_index_to_create = current_index.first;
		UMaterialInterface* material = current_index.second;
		generateMesh(_out, shapes, holes, height, mesh_section_index_to_create, _generateUVs, _generateCollisions, _stretchUVs, material, mesh_section_index_to_create);
		//MeshGenerator::generateMesh( ret->mesh, shapes, holes, height, mesh_section_index_to_create, true, true, true);
		//_out->SetMaterial(mesh_section_index_to_create, material);
	}
	Runnable->isDone = true;
	delete grouped;
	return 0;
}

//stop
void FCoalaTaskCreateCells::Stop()
{
	StopTaskCounter.Increment();
}

FCoalaTaskCreateCells* FCoalaTaskCreateCells::JoyInit( std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes, UProceduralMeshComponent* out, bool generateUVs, bool generateCollisions, bool stretchUVs, ACoalaMeshActor* ret)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FCoalaTaskCreateCells(grouped_shapes, out,  generateUVs,  generateCollisions,  stretchUVs, ret);
	}
	return Runnable;
}

void FCoalaTaskCreateCells::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}



void FCoalaTaskCreateCells::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

ACoalaMeshActor*
FCoalaTaskCreateCells::GetResult()
{
	return _ret;
}

bool FCoalaTaskCreateCells::IsThreadFinished()
{
	if (Runnable) return Runnable->IsFinished();
	return false;
}
void FCoalaTaskCreateCells::generateMesh(UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs, UMaterialInterface* material, int mesh_section_index_to_create)
{
	TArray<FVector> vertices;
	TArray<int32> triangles;
	int indicesOffset = 0;
	//UE_LOG(LogTemp, Warning, TEXT("amount shapes %d:"), shapes.Num());
	int heightArrayLength = zOffset.Num();
	if (heightArrayLength > 0)
	{
		for (int z = 0; z < shapes.Num(); ++z)
		{

			TArray<FVector> currentShape = shapes[z];
			for (int y = 0; y < currentShape.Num(); ++y)
			{
				FVector current = currentShape[y];
				current.Z = zOffset[z%heightArrayLength];
				currentShape[y] = current;
			}
			shapes[z] = currentShape;
		}

		for (int z = 0; z < holes.Num(); ++z)
		{
			TArray<TArray<FVector>> currentBuilding = holes[z];
			for (int y = 0; y < currentBuilding.Num(); ++y)
			{
				TArray<FVector> currentHole = currentBuilding[y];
				for (int u = 0; u < currentHole.Num(); ++u)
				{
					FVector current = currentHole[u];
					current.Z = zOffset[z%heightArrayLength];
					currentHole[u] = current;
				}
				currentBuilding[y] = currentHole;
			}
			holes[z] = currentBuilding;
		}
	}

	for (int a = 0; a < shapes.Num(); ++a)
	{
		TArray<FVector> current = shapes[a];

		for (int b = 0; b < current.Num(); ++b)
			vertices.Add(current[b]);

		TArray<TArray<FVector>> currenthole = holes[a];
		for (int b = 0; b < currenthole.Num(); ++b)
		{
			TArray<FVector> temphole = currenthole[b];
			for (int d = 0; d < temphole.Num(); ++d)
				vertices.Add(temphole[d]);
		}
		
		TArray<uint32> indices = MeshGenerator::triangulate(current, currenthole);
		for (int c = 0; c < indices.Num(); ++c)
			triangles.Add(indices[c] + indicesOffset);

		indicesOffset = vertices.Num();
	}
	TArray<FVector> normals;
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));
	//	normals.Add(FVector(1, 0, 0));

	TArray<FVector2D> UV0;
	//	UV0.Add(FVector2D(0, 0));
	//	UV0.Add(FVector2D(10, 0));
	//	UV0.Add(FVector2D(0, 10));
	if (_generateUVs)
	{
		for (int i = 0; i < shapes.Num(); ++i)
		{
			TArray<FVector> shape = shapes[i];
			float min_x = shape[0].X;
			float max_x = shape[0].X;

			float min_y = shape[0].Y;
			float max_y = shape[0].Y;

			for (int a = 0; a < shape.Num(); ++a)
			{
				FVector current = shape[a];

				if (current.X < min_x) min_x = current.X;
				if (current.X > max_x) max_x = current.X;
				if (current.Y < min_y) min_y = current.Y;
				if (current.Y > max_y) max_y = current.Y;
			}

			for (int a = 0; a < shape.Num(); ++a)
			{
				FVector current = shape[a];

				FVector2D uv;
				if (_stretchUVs)
				{
					uv = FVector2D(
						((current.X - min_x) / (max_x - min_x)),
						((current.Y - min_y) / (max_y - min_y))
					);
				}
				else
				{
					// using tileset value as tiling value
					uv = FVector2D(
						((current.X - min_x) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale())),
						((current.Y - min_y) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale()))
					);
				}

				UV0.Add(uv);
			}
			TArray<TArray<FVector>> shapeHoles = holes[i];
			for (int a = 0; a < shapeHoles.Num(); ++a)
			{
				TArray<FVector> currentHole = shapeHoles[a];
				for (int b = 0; b < currentHole.Num(); ++b)
				{
					FVector current = currentHole[b];

					FVector2D uv(
						((current.X - min_x) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale())),
						((current.Y - min_y) / (COALA_BUILDING_MESH_UV_TILESIZE*UCoalaBlueprintUtility::GetCoalaScale()))
					);

					UV0.Add(uv);
				}
			}
		}
	}

	TArray<FProcMeshTangent> tangents;
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));
	//	tangents.Add(FProcMeshTangent(0, 1, 0));

	TArray<FLinearColor> vertexColors;
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));
	//	vertexColors.Add(FLinearColor(0.75, 0.75, 0.75, 1.0));

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(vertices, triangles, UV0, normals, tangents);
	AsyncTask(ENamedThreads::GameThread, [=]() {
		_out->bUseAsyncCooking = true;
		_out->CreateMeshSection_LinearColor(segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions);
		_out->SetMaterial(mesh_section_index_to_create, material);
		//Shutdown();
	});
}
