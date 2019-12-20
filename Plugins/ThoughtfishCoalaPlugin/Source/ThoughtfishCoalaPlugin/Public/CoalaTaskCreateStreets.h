// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoalaMeshGenerator.h"
#include "Core.h"
#include "Core/Public/Async/Async.h"
/**
 * 
 */

class FCoalaTaskCreateStreets : public FRunnable
{
public:
	static FCoalaTaskCreateStreets* Runnable;

	FRunnableThread* Thread;
	FCoalaStreetRenderConfig _usedConfig;
	class UCoalaStreets* _streets;
	FVector _area_fixpoint;
	bool isDone;
	class ACoalaMeshActor* _ret;
	class UProceduralMeshComponent* _out;

	FThreadSafeCounter StopTaskCounter;

	void generateMesh(class ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, class UCoalaStreets* streets, FVector area_fixpoint, class UProceduralMeshComponent* out);

	void generateStreetMesh(class UProceduralMeshComponent *& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs, FCoalaStreetRenderConfig usedConfig);


	//Done?
	bool IsFinished()
	{
		return isDone;
	}

	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	FCoalaTaskCreateStreets(class ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, class UCoalaStreets* streets, FVector area_fixpoint, class UProceduralMeshComponent* out);
	virtual ~FCoalaTaskCreateStreets();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	class ACoalaMeshActor* GetResult();
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	static FCoalaTaskCreateStreets* JoyInit(class ACoalaMeshActor* ret, FCoalaStreetRenderConfig usedConfig, class UCoalaStreets* streets, FVector area_fixpoint, class UProceduralMeshComponent* out);

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();
};
