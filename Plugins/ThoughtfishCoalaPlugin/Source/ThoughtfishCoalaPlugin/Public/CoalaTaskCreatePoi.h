// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once
/*
#include "Core.h"
#include "Async.h"
#include "CoalaMeshGenerator.h"

class FCoalaTaskCreatePoi : public FRunnable
{
public:
	static FCoalaTaskCreatePoi* Runnable;

	FRunnableThread* Thread;
	FCoalaStreetRenderConfig _usedConfig;
	UCoalaStreets* _streets;
	FVector _area_fixpoint;
	bool isDone;
	ACoalaMeshActor* _ret;
	class UProceduralMeshComponent* _out;
	std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > > grouped;

	FThreadSafeCounter StopTaskCounter;

	void generateMesh(ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, UCoalaStreets* streets, FVector area_fixpoint, UProceduralMeshComponent* out);

	void generateStreetMesh(UProceduralMeshComponent *& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs, FCoalaStreetRenderConfig usedConfig);


	//Done?
	bool IsFinished()
	{
		return isDone;
	}

	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	FCoalaTaskCreatePoi(ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, UCoalaStreets* streets, FVector area_fixpoint, UProceduralMeshComponent* out);
	virtual ~FCoalaTaskCreatePoi();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	ACoalaMeshActor* GetResult();
	// End FRunnable interface

	
	void EnsureCompletion();

	static FCoalaTaskCreatePoi* JoyInit(ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, UCoalaStreets* streets, FVector area_fixpoint, UProceduralMeshComponent* out);

	
	static void Shutdown();

	static bool IsThreadFinished();
};
*/