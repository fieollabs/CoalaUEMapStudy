// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "Core.h"
#include "Core/Public/Async/Async.h"

/**
 *
 */

class FCoalaTaskCreateWater : public FRunnable
{
public:
	static FCoalaTaskCreateWater* Runnable;

	FRunnableThread* Thread;
	//FCoalaStreetRenderConfig _usedConfig;
	class UCoalaWater* _water;
	FVector _area_fixpoint;
	bool isDone;
	FVector _center;
	class ACoalaMeshActor* _ret;
	class UProceduralMeshComponent* _out;
	class UMaterialInterface* _material;
	bool _generateUVs;
	float _outlineWidth;
	class UMaterialInterface* _outlineMaterial;

	FThreadSafeCounter StopTaskCounter;

	void generateMesh( class ACoalaMeshActor* ret, class UCoalaWater* water, FVector area_fixpoint, class UProceduralMeshComponent* out, FVector center, class UMaterialInterface* material, bool generateUVs, float outlineWidth, class UMaterialInterface* outlineMaterial);


	//Done?
	bool IsFinished()
	{
		return isDone;
	}

	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	FCoalaTaskCreateWater(class ACoalaMeshActor* ret, class UCoalaWater* water, FVector area_fixpoint, class UProceduralMeshComponent* out, FVector center, class UMaterialInterface* material, bool generateUVs, float outlineWidth, class UMaterialInterface* outlineMaterial);
	virtual ~FCoalaTaskCreateWater();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	class ACoalaMeshActor* GetResult();
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	static FCoalaTaskCreateWater* JoyInit(class ACoalaMeshActor* ret, class UCoalaWater* water, FVector area_fixpoint, class UProceduralMeshComponent* out, FVector center, class UMaterialInterface* material, bool generateUVs, float outlineWidth, class UMaterialInterface* outlineMaterial);

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();


};
