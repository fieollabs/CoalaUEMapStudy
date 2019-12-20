// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "Core.h"
#include "KismetProceduralMeshLibrary.h"
#include "Core/Public/Async/Async.h"
#include <map>


class FCoalaTaskCreateCells 
: public FRunnable
{
public:
	static FCoalaTaskCreateCells* Runnable;

	FRunnableThread* Thread;
	bool _generateUVs;
	bool _generateCollisions;
	bool _stretchUVs;
	bool isDone;
	class ACoalaMeshActor* _ret;
	class UProceduralMeshComponent* _out;
	std::map< std::pair<int, class UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped;

	FThreadSafeCounter StopTaskCounter;

	void generateMesh(class UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs, class UMaterialInterface* material, int mesh_section_index_to_create);


	//Done?
	bool IsFinished()
	{
		return isDone;
	}

	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	FCoalaTaskCreateCells( std::map< std::pair<int, class UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes, class UProceduralMeshComponent* _out, bool generateUVs, bool generateCollisions, bool stretchUVs, class ACoalaMeshActor* ret);
	virtual ~FCoalaTaskCreateCells();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	class ACoalaMeshActor* GetResult();
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	static class FCoalaTaskCreateCells* JoyInit( class std::map< std::pair<int, class UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes, class UProceduralMeshComponent* _out, bool generateUVs, bool generateCollisions, bool stretchUVs, class ACoalaMeshActor* ret);

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();


};
