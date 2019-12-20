// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "Core.h"
#include "Core/Public/Async/Async.h"

/**
 *
 */

class FCoalaTaskCreateBuildings : public FRunnable
{
public:
	static FCoalaTaskCreateBuildings* Runnable;

	FRunnableThread* Thread;
	//FCoalaStreetRenderConfig _usedConfig;
	TArray<class UCoalaBuilding*> _buildings;
	FVector _area_fixpoint;
	bool isDone;
	class ACoalaMeshActor* _ret;
	class UProceduralMeshComponent* _out;
	class UMaterialInterface* _materialFloor;
	class UMaterialInterface* _materialWall;
	class UMaterialInterface* _materialRoof;
	bool _generateUVs;
	float _outlineWidth;
	int32 _createMeshes;
	class UMaterialInterface* _outlineMaterial;
	bool _generateCollisions;
	bool _stretchUVs ;
	float _heightPerLevel;

	FThreadSafeCounter StopTaskCounter;

	void generateMesh(class ACoalaMeshActor* ret, TArray<class UCoalaBuilding*> buildings, int32 createMeshes, FVector area_fixpoint, class UProceduralMeshComponent* out, class UMaterialInterface* materialFloor, class UMaterialInterface* materialWall, class UMaterialInterface* materialRoof, bool generateUVs, bool generateCollisions, bool stretchUVs, float heightPerLevel);

	//Done?
	bool IsFinished()
	{
		return isDone;
	}

	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	FCoalaTaskCreateBuildings(class ACoalaMeshActor* ret, TArray<class UCoalaBuilding*> buildings, int32 createMeshes, FVector area_fixpoint, class UProceduralMeshComponent* out, class UMaterialInterface* materialFloor, class UMaterialInterface* materialWall, class UMaterialInterface* materialRoof, bool generateUVs, bool generateCollisions, bool stretchUVs, float heightPerLevel);
	virtual ~FCoalaTaskCreateBuildings();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	class ACoalaMeshActor* GetResult();
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	static FCoalaTaskCreateBuildings* JoyInit(class ACoalaMeshActor* ret, TArray<class UCoalaBuilding*> buildings, int32 createMeshes, FVector area_fixpoint, class UProceduralMeshComponent* out, class UMaterialInterface* materialFloor, class UMaterialInterface* materialWall, class UMaterialInterface* materialRoof, bool generateUVs, bool generateCollisions, bool stretchUVs, float heightPerLevel);

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();


};
