// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "Core/Public/Async/Async.h"

class FCoalaTaskGetGroupedShapes : public FRunnable
{
public:
	static FCoalaTaskGetGroupedShapes* Runnable;

	FRunnableThread* Thread;
	class ACoalaMeshActor* actor;
	class AActor* spawnActor;
	class UCoalaArea* area;
	struct FActorSpawnParameters spawnInfo;
	class TArray<UCoalaCell*> cells;
	struct FCoalaCellRenderConfig defaultRenderConfig;
	class TArray<FCoalaCellRenderConfig> renderConfig;
	struct FVector area_fixpoint;
	class std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped;

	FThreadSafeCounter StopTaskCounter;


	//~~~ Thread Core Functions ~~~
	void generateGroupedShapes(AActor* _spawnActor, class UCoalaArea* _area, TArray< class UCoalaCell*> _cells, FCoalaCellRenderConfig _defaultRenderConfig, TArray<FCoalaCellRenderConfig> _renderConfig, FVector _area_fixpoint, std::map< std::pair<int, class UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* _grouped_shapes, class ACoalaMeshActor* ret, FActorSpawnParameters spawnInfo);
	//Constructor / Destructor
	FCoalaTaskGetGroupedShapes(AActor* _spawnActor, class UCoalaArea* _area, TArray<UCoalaCell*> _cells, FCoalaCellRenderConfig _defaultRenderConfig, TArray<FCoalaCellRenderConfig> _renderConfig, FVector _area_fixpoint, std::map< std::pair<int, class UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* _grouped_shapes, class ACoalaMeshActor* ret, FActorSpawnParameters spawnInfo);
	virtual ~FCoalaTaskGetGroupedShapes();
	// Begin FRunnable interface
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface
	void EnsureCompletion();
	//~~~ Starting and Stopping Thread ~~~

	static FCoalaTaskGetGroupedShapes* JoyInit(AActor* _spawnActor, class UCoalaArea* _area, TArray<class UCoalaCell*> _cells, FCoalaCellRenderConfig _defaultRenderConfig, TArray<FCoalaCellRenderConfig> _renderConfig, FVector _area_fixpoint, class std::map< std::pair<int, class UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* _grouped_shapes, class ACoalaMeshActor* ret, FActorSpawnParameters spawnInfo);

	static void Shutdown();

};
