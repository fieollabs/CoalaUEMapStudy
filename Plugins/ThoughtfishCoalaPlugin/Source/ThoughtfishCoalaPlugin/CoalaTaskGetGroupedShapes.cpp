// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaTaskGetGroupedShapes.h"
#include "CoalaArea.h"
#include "TopDownPlayerController.h"
#include "Core.h"
#include "CoalaMeshGenerator.h"
#include "Runtime/Core/Public/Math/Vector.h"

//***********************************************************
//Thread Worker Starts as NULL, prior to being instanced
//		This line is essential! Compiler error without it
FCoalaTaskGetGroupedShapes* FCoalaTaskGetGroupedShapes::Runnable = NULL;
//***********************************************************

FCoalaTaskGetGroupedShapes::FCoalaTaskGetGroupedShapes(AActor* _spawnActor, UCoalaArea* _area, TArray<UCoalaCell*> _cells, FCoalaCellRenderConfig _defaultRenderConfig, TArray<FCoalaCellRenderConfig> _renderConfig, FVector _area_fixpoint, std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* _grouped_shapes, ACoalaMeshActor* ret, FActorSpawnParameters _spawnInfo)
{
	grouped = _grouped_shapes;
	spawnActor = _spawnActor;
	spawnInfo = _spawnInfo;
	actor = ret;
	area = _area;
	cells = _cells;
	defaultRenderConfig = _defaultRenderConfig;
	renderConfig = _renderConfig;
	area_fixpoint = _area_fixpoint;
	//Link to where data should be stored
	Thread = FRunnableThread::Create(this, TEXT("FCoalaTaskGetGroupedShapes"), 0, TPri_BelowNormal);
}

FCoalaTaskGetGroupedShapes::~FCoalaTaskGetGroupedShapes()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FCoalaTaskGetGroupedShapes::Init()
{
	//Init the Data 

	return true;
}
//Run
uint32 FCoalaTaskGetGroupedShapes::Run()
{

	generateGroupedShapes( spawnActor, area,  cells,  defaultRenderConfig,  renderConfig, area_fixpoint , grouped, actor, spawnInfo);
	return 0;
}

//stop
void FCoalaTaskGetGroupedShapes::Stop()
{
	StopTaskCounter.Increment();
}

FCoalaTaskGetGroupedShapes* FCoalaTaskGetGroupedShapes::JoyInit(AActor* _spawnActor, UCoalaArea* _area, TArray<UCoalaCell*> _cells, FCoalaCellRenderConfig _defaultRenderConfig, TArray<FCoalaCellRenderConfig> _renderConfig, FVector _area_fixpoint, std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* _grouped_shapes, ACoalaMeshActor* ret, FActorSpawnParameters _spawnInfo)
{
	//Create new instance of thread if it does not exist
	//		and the platform supports multi threading!
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FCoalaTaskGetGroupedShapes(_spawnActor, _area, _cells, _defaultRenderConfig, _renderConfig, _area_fixpoint, _grouped_shapes, ret, _spawnInfo);
	}
	return Runnable;
}

void FCoalaTaskGetGroupedShapes::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}



void FCoalaTaskGetGroupedShapes::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

void FCoalaTaskGetGroupedShapes::generateGroupedShapes(AActor* _spawnActor, UCoalaArea* _area, TArray<UCoalaCell*> _cells, FCoalaCellRenderConfig _defaultRenderConfig, TArray<FCoalaCellRenderConfig> _renderConfig, FVector _area_fixpoint, std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* _grouped_shapes, ACoalaMeshActor* ret, FActorSpawnParameters _spawnInfo)
{
	for (int i = 0; i < _cells.Num(); ++i)
	{
		UCoalaCell* cell = _cells[i];

for (auto it = cell->gameTags.begin(); it != cell->gameTags.end(); ++it)
{
	if( it.Key() == "green_cell" )
		int iBreak = 1;
}

		int material_index = 0;
		UMaterialInterface* material = 0;
		// TODO: Refactor to get gametag with hightes priority not only first match !
		{
			for (auto it = cell->gameTags.begin(); it != cell->gameTags.end(); ++it)
			{
				if (material)
					break;

				FString currentGametagName = it->Key;

				for (int b = 0; b < _renderConfig.Num(); ++b)
				{
					if (material)
						break;

					FCoalaCellRenderConfig cfg = _renderConfig[b];
					for (int a = 0; a < cfg.gametagNames.Num(); ++a)
					{
						if (material)
							break;

						FString possibleGametagName = cfg.gametagNames[a];
						if (possibleGametagName.Compare(currentGametagName) == 0)
						{
							//UE_LOG( LogTemp, Warning, TEXT( "FOUND" ) );
													// actor->createMesh( area->props->bounds, cell, cfg.material );
							material = cfg.material;
							material_index = b+1;
						}
					}
				}
			}
			if (!material)
				material = defaultRenderConfig.material;

			std::pair<int, UMaterialInterface*> group_index(material_index, material);

			// mesh data generation
			// calculate corners
			int count_cells_in_one_row = sqrt(area->grid.Num());
			double cell_width = (area->props->bounds->right - area->props->bounds->left) / count_cells_in_one_row;
			double cell_height = (area->props->bounds->top - area->props->bounds->bottom) / count_cells_in_one_row;
			{
				double cell_left = area->props->bounds->left + (cell->index->x * cell_width);
				double cell_right = area->props->bounds->left + ((cell->index->x + 1) * cell_width);
				double cell_top = area->props->bounds->bottom - ((-1)*cell->index->y * cell_height);
				double cell_bottom = area->props->bounds->bottom - ((-1)*(cell->index->y + 1) * cell_height);

				FVector location = CoalaConverter::ToScenePosition(cell_left, cell_top) - area_fixpoint;

				cell->top_left = CoalaConverter::ToScenePosition(cell_left, cell_top);
				cell->top_right = CoalaConverter::ToScenePosition(cell_right, cell_top);
				cell->bottom_right = CoalaConverter::ToScenePosition(cell_right, cell_bottom);
				cell->bottom_left = CoalaConverter::ToScenePosition(cell_left, cell_bottom);

				TArray<FVector> shape;
				// shape
				{
					shape.Add(cell->top_left - area_fixpoint);
					shape.Add(cell->top_right - area_fixpoint);
					shape.Add(cell->bottom_right - area_fixpoint);
					shape.Add(cell->bottom_left - area_fixpoint);
				}

				TArray<TArray<FVector> > holes;
				// holes
				{
					// none
				}

				// check if entry exsist
				// std::map< std::pair<int,UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > > grouped_shapes; // <material index, list of shapes>
				auto it_find = _grouped_shapes->find(group_index);
				if (it_find == _grouped_shapes->end())
				{
					// new entry
					(*_grouped_shapes)[group_index] = std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> >(TArray<TArray<FVector>>(), TArray<TArray<TArray<FVector>>>());
				}
				// add to collection
				(*_grouped_shapes)[group_index].first.Add(shape);
				(*_grouped_shapes)[group_index].second.Add(holes);
			}

		}
	}
	AsyncTask(ENamedThreads::GameThread, [_spawnActor, _area, _cells, _defaultRenderConfig, _renderConfig, _area_fixpoint, _grouped_shapes, ret, _spawnInfo]() {
		MeshGenerator::ContinueCellCreation(_spawnActor, _area, _cells, _defaultRenderConfig, _renderConfig, _area_fixpoint, _grouped_shapes, ret,_spawnInfo );
		Shutdown();
	});
}
