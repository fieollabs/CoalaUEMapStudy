// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaTaskCreateStreets.h"
#include "KismetProceduralMeshLibrary.h"
#include "CoalaBlueprintUtility.h"
#include "CoalaArea.h"


//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//		This line is essential! Compiler error without it
FCoalaTaskCreateStreets* FCoalaTaskCreateStreets::Runnable = NULL;
//***********************************************************

FCoalaTaskCreateStreets::FCoalaTaskCreateStreets( ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, UCoalaStreets* streets, FVector area_fixpoint, UProceduralMeshComponent* out)
{
	_out = out;
	_usedConfig = usedConfig;
	_streets = streets;
	_area_fixpoint = area_fixpoint;
	isDone = false;
	_ret = ret;
	//Link to where data should be stored
	Thread = FRunnableThread::Create(this, TEXT("FCoalaTaskCreateStreets"), 0, TPri_BelowNormal);
}

FCoalaTaskCreateStreets::~FCoalaTaskCreateStreets()
{
	delete Thread;
	/*
	if(_out)
		delete Runnable->_out;
	if (_streets)
		delete Runnable->_streets;
	if (_ret)
		delete Runnable->_ret;
		*/
	Thread = NULL;
}

//Init
bool FCoalaTaskCreateStreets::Init()
{
	//Init the Data 

	return true;
}
//Run
uint32 FCoalaTaskCreateStreets::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.0001);


	generateMesh(_ret, _usedConfig, _streets, _area_fixpoint, _out);
	//MeshGenerator::generateMesh( ret->mesh, shapes, holes, height, mesh_section_index_to_create, true, true, true);
	//_out->SetMaterial(mesh_section_index_to_create, material);
	
	isDone = true;
	return 0;
}

//stop
void FCoalaTaskCreateStreets::Stop()
{
	StopTaskCounter.Increment();
}

FCoalaTaskCreateStreets* FCoalaTaskCreateStreets::JoyInit(ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, UCoalaStreets* streets, FVector area_fixpoint, UProceduralMeshComponent* out)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FCoalaTaskCreateStreets( ret, usedConfig, streets, area_fixpoint, out);
	}
	return Runnable;
}

void FCoalaTaskCreateStreets::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}



void FCoalaTaskCreateStreets::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		Runnable->_out = nullptr;
		Runnable->_streets = nullptr;
		Runnable->_ret = nullptr;

		delete Runnable;
		Runnable = NULL;
	}
}

ACoalaMeshActor*
FCoalaTaskCreateStreets::GetResult()
{
	return _ret;
}

bool FCoalaTaskCreateStreets::IsThreadFinished()
{
	if (Runnable) return Runnable->IsFinished();
	return false;
}

void FCoalaTaskCreateStreets::generateMesh(ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, UCoalaStreets* streets, FVector area_fixpoint, UProceduralMeshComponent* out)
{
		TArray<TArray<FVector>> shapes;
		TArray<TArray<TArray<FVector> > > holes;

		for (int i = 0; i < streets->data.Num(); ++i)
		{
			UCoalaStreet* currentStreet = streets->data[i];

			// convert data to world coordinated
			TArray<FVector> convertedShape;
			for (int a = 0; a < currentStreet->points.Num(); ++a)
			{
				UCoalaGPSCoordinates* p = currentStreet->points[a];

				convertedShape.Add(CoalaConverter::ToScenePosition(p->lon, p->lat) - area_fixpoint);
			}


			TArray<TArray<FVector> > no_holes;
			{
				// no holes for streets
			}

			TArray<FVector> streetShape = MeshGenerator::getShape(convertedShape, usedConfig.width*UCoalaBlueprintUtility::GetCoalaScale());

			shapes.Add(streetShape);
			holes.Add(no_holes);
		}

		TArray<int> no_height = { 0 };
		generateStreetMesh(_out, shapes, holes, no_height, 0, false, true, false, usedConfig);
}
void
FCoalaTaskCreateStreets::generateStreetMesh(UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs, FCoalaStreetRenderConfig usedConfig)
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
	if (generateUVs)
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
				if (stretchUVs)
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
	//UKismetProceduralMeshLibrary::CalculateTangentsForMesh(vertices, triangles, UV0, normals, tangents);
	AsyncTask(ENamedThreads::GameThread, [segmentIndexToCreateMesh,vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions, out, usedConfig]() {
		out->bUseAsyncCooking = true;
		out->CreateMeshSection_LinearColor(segmentIndexToCreateMesh, vertices, triangles, normals, UV0, vertexColors, tangents, generateCollisions);
		if (usedConfig.material)
			out->SetMaterial(0, usedConfig.material);
	});
}


