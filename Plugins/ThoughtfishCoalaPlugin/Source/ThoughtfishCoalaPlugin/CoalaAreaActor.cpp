// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaAreaActor.h"
#include "CoalaMeshActor.h"

// Sets default values
ACoalaAreaActor::ACoalaAreaActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
}

// Called when the game starts or when spawned
void ACoalaAreaActor::BeginPlay()
{
	Super::BeginPlay();
	
	_refAreaDimensions = 0;
	_refAllCells = 0;
	_refAllWaters = 0;
	_refAllBuildings = 0;
	_refAllPOIs = 0;
	_refAllStreets = 0;

	_refAllDecorations = 0;
}

// Called every frame
void ACoalaAreaActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int ACoalaAreaActor::cleanupHoldedCoalaActors()
{
	int ret = 0;

	if( _refAreaDimensions )
	{
		ret += _refAreaDimensions->cleanupAllAttachedActors();
		_refAreaDimensions->Destroy();
		_refAreaDimensions = 0;
	}

	if( _refAllCells )
	{
		ret += _refAllCells->cleanupAllAttachedActors();
		_refAllCells->Destroy();
		_refAllCells = 0;
	}

	if( _refAllWaters )
	{
		ret += _refAllWaters->cleanupAllAttachedActors();
		_refAllWaters->Destroy();
		_refAllWaters = 0;
	}

	if( _refAllBuildings )
	{
		ret += _refAllBuildings->cleanupAllAttachedActors();
		_refAllBuildings->Destroy();
		_refAllBuildings = 0;
	}

	if( _refAllPOIs )
	{
		ret += _refAllPOIs->cleanupAllAttachedActors();
		_refAllPOIs->Destroy();
		_refAllPOIs = 0;
	}
	
	if( _refAllStreets )
	{
		ret += _refAllStreets->cleanupAllAttachedActors();
		_refAllStreets->Destroy();
		_refAllStreets = 0;
	}

	if( _refAllDecorations )
	{
		ret += _refAllDecorations->cleanupAllAttachedActors();
		_refAllDecorations->Destroy();
		_refAllDecorations = 0;
	}

	return ret;
}
