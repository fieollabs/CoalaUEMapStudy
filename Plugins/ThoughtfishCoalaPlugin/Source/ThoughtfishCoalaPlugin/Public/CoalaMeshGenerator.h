// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include <map>
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaMeshGenerator.generated.h"


USTRUCT(BlueprintType, meta=(HiddenByDefault))
struct THOUGHTFISHCOALAPLUGIN_API FCoalaCellRenderConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala cell render config" )
	TArray<FString> gametagNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala cell render config")
	class UMaterialInterface* material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala cell render config")
	bool onlyIfGametagIsHighest;
};


USTRUCT(BlueprintType, meta=(HiddenByDefault))
struct THOUGHTFISHCOALAPLUGIN_API FCoalaStreetRenderConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala street render config" )
	TArray<FString> types;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala street render config")
	float width;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala street render config")
	class UMaterialInterface* material;
};

UENUM(BlueprintType,Blueprintable,Meta = (Bitmask, Bitflags, UseEnumValuesAsMaskValuesInEditor = "true", BitmaskEnum="OPTIONS_MESH_CREATION_BUILDING") )
enum class OPTIONS_MESH_CREATION_BUILDING : uint8
{
	FLOOR = 1,	
	WALLS = 2,		
	ROOF = 4
};
ENUM_CLASS_FLAGS( OPTIONS_MESH_CREATION_BUILDING )

UCLASS()
class THOUGHTFISHCOALAPLUGIN_API UCoalaMeshGenerator
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
		UFUNCTION(BlueprintCallable, Category="Coala|DEV")
		static class ACoalaMeshActor* CreateAreaDimensions(AActor* spawnActor, class UCoalaArea* area, class UMaterialInterface* material = 0);
		
		UFUNCTION(BlueprintCallable, Category="Coala|MeshGenerationExcample")
		static class ACoalaMeshActor* CreateCell(AActor* spawnActor, class UCoalaArea* area, class UCoalaCell* cell, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig );
			
		UFUNCTION(BlueprintCallable, Category="Coala|MeshGenerationExcample")
		static class ACoalaMeshActor* CreateCells(AActor* spawnActor, class UCoalaArea* area, TArray<class UCoalaCell*> cells, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig);

		UFUNCTION(BlueprintCallable, Category="Coala|MeshGenerationExcample")
		static class ACoalaMeshActor* CreateWater(AActor* spawnActor, class UCoalaArea* area, class UCoalaWater* water, class UMaterialInterface* material = 0, bool generateUVs = false, float outlineWidth = 0.0f, class UMaterialInterface* outlineMaterial = 0 );

		UFUNCTION(BlueprintCallable, Category="Coala|MeshGenerationExcample")
		static class ACoalaMeshActor* CreateBuilding(AActor* spawnActor, class UCoalaArea* area, class UCoalaBuilding* building, UPARAM(meta=(Bitmask,BitmaskEnum="OPTIONS_MESH_CREATION_BUILDING")) int32 createMeshes, class UMaterialInterface* materialFloor = 0, class UMaterialInterface* materialWall = 0, class UMaterialInterface* materialRoof = 0, bool generateUVs = false, float heightPerLevel = 500.0f, bool generateCollisions = false );

		UFUNCTION(BlueprintCallable, Category = "Coala|MeshGenerationExcample")
		static class ACoalaMeshActor* CreateBuildings(AActor* spawnActor, class UCoalaArea* area, TArray<UCoalaBuilding*> buildings, UPARAM(meta = (Bitmask, BitmaskEnum = "OPTIONS_MESH_CREATION_BUILDING")) int32 createMeshes, class UMaterialInterface* materialFloor = 0, class UMaterialInterface* materialWall = 0, class UMaterialInterface* materialRoof = 0, bool generateUVs = false, float heightPerLevel = 500.0f, bool generateCollisions = false);
		
		UFUNCTION(BlueprintCallable, Category="Coala|MeshGenerationExcample")
		static void PlacePoi(AActor*& inst, AActor* spawnActor, class UCoalaPOI* poi, class UCoalaArea* area, TMap<FString,TSubclassOf<class AActor>> poiConfiguration );
		
		UFUNCTION(BlueprintCallable, Category="Coala|MeshGenerationExcample")
		static class ACoalaMeshActor* CreateStreets(AActor* spawnActor, class UCoalaArea* area, class UCoalaStreets* streets, FCoalaStreetRenderConfig defaultRenderConfig, TArray<FCoalaStreetRenderConfig> renderConfig );

		UFUNCTION(BlueprintCallable, Category = "Coala|MeshAsync")
		static void CreateStreetsAsync(AActor* spawnActor, UCoalaArea* area, UCoalaStreets* streets, FCoalaStreetRenderConfig defaultRenderConfig, TArray<FCoalaStreetRenderConfig> renderConfig);

		UFUNCTION(BlueprintCallable, Category = "Coala|MeshAsync")
		static void CreateWaterAsync(AActor* spawnActor, UCoalaArea* area, UCoalaWater* water, UMaterialInterface* material, bool generateUVs, float outlineWidth, UMaterialInterface* outlineMaterial);

		UFUNCTION(BlueprintCallable, Category = "Coala|MeshAsync")
		static void CreateBuildingsAsync(AActor* spawnActor, UCoalaArea* area, TArray<UCoalaBuilding*> buildings, int32 createMeshes, UMaterialInterface* materialFloor, UMaterialInterface* materialWall, UMaterialInterface* materialRoof, bool generateUVs, float heightPerLevel, bool generateCollisions);

		UFUNCTION(BlueprintCallable, Category = "Coala|MeshAsync")
		static void CreateCellsAsync(AActor* spawnActor, UCoalaArea* area, TArray<UCoalaCell*> cells, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig);
		/*
		UFUNCTION(BlueprintCallable, Category = "Coala|MeshAsync")
		static void PlacePoisAsync(TArray <AActor*>& instCol, AActor* spawnActor, TArray<UCoalaPOI*> poi, UCoalaArea* area, TMap<FString, TSubclassOf<AActor>> poiConfiguration);
		*/

		// no blueprint logic
		static void checkAndCreateSceneObjectIfNeeded( class UCoalaArea* area, AActor* spawnActor );
};

namespace OPTIONS_SHAPE_GENERATION_FROM_LINE
{
	enum type
	{
		BOTH_SIDES = 1,
		ONLY_LEFT,
		ONLY_RIGHT,

		INCREASE_SIZE
	};
}

class THOUGHTFISHCOALAPLUGIN_API MeshGenerator
{
	public:
		// generate one mesh from ONE shape with many holes
		static void generateMesh( class UProceduralMeshComponent*& out, TArray<FVector> basicShape, TArray<TArray<FVector>> holes, int segmentIndexToCreateMesh, float zOffset, bool generateUVs, bool generateCollisions, bool stretchUVs );
		// generate on mesh from MANY shapes with many holes
		static void generateMesh( class UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh, bool generateUVs, bool generateCollisions, bool stretchUVs);
		
	//	private:
		static int generateWallMesh( class UProceduralMeshComponent*& out, TArray<FVector> basicShape, TArray<TArray<FVector>> holes, float buildingHight, int meshSegmentStartIndex = 0, bool generateUVs = false, bool generateCollisions = false );
		static void generateBuildingMesh( UProceduralMeshComponent*& out, TArray<FVector> basicShape, TArray<TArray<FVector>> holes, float buildingHight, bool generateUVs = false, bool generateCollisions = false );

		static void generateWallMeshSegment( class UProceduralMeshComponent*& out, int indexMeshSection, FVector p1, FVector p2, float wall_height, bool generateUVs = false, bool generateCollisions = false );
		static void ContinueCellCreation(AActor* spawnActor, UCoalaArea* area, TArray<UCoalaCell*> cells, FCoalaCellRenderConfig defaultRenderConfig, TArray<FCoalaCellRenderConfig> renderConfig, FVector area_fixpoint, class std::map< std::pair<int, UMaterialInterface*>, std::pair< TArray<TArray<FVector>>, TArray<TArray<TArray<FVector>>> > >* grouped_shapes, ACoalaMeshActor* ret, FActorSpawnParameters spawnInfo);

		// street
		static void calculateAndDrawStreetBeginSegment( FVector start, FVector end, TArray<FVector>& addTo, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option, bool drawMiddleVectors = false );
		static void calculateAndDrawStreetCenterSegment( FVector start, FVector mid, FVector end, TArray<FVector>& addTo, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option, bool drawMiddleVectors = false );
		static void calculateAndDrawStreetEndSegment( FVector start, FVector end, TArray<FVector>& addTo, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option, bool drawMiddleVectors = false );

		static void drawLines( TArray<FVector>& streetData, FColor color, float thickness = 1 );
		static void drawLine( FVector p1, FVector p2, FColor color, float thickness = 1 );
		static void drawDegreFanAround( FVector start, FVector end, float drawDegre = 0.0f );

		static float getAngle( FVector p1, FVector p2, FVector p3, bool drawTwoDirectionVectors = false );
		
		//single mesh
		static void generateSingleWallMesh(class UProceduralMeshComponent*& out, TArray<TArray<FVector>> shapes, TArray<TArray<TArray<FVector>>> holes, TArray<int32> zOffset, int segmentIndexToCreateMesh = 0, bool generateUVs = false, bool stretchUVs = false, bool generateCollisions = false);
		static void generateSingleWallMeshSegment(class UProceduralMeshComponent*& out, FVector p1, FVector p2, float wall_height, int indicesOffset, TArray<FVector>& vertices, TArray<int32>& triangles, TArray<FVector2D>& UV0, bool generateUVs = false);

		static TArray<uint32_t> triangulate(const TArray<FVector>& shapeVertices, const TArray<TArray<FVector>>& holeVertices);

		static TArray<FVector> getShape( class UCoalaStreet* streetData, float width );
		static TArray<FVector> getShape( TArray<FVector>& streetData, float width, OPTIONS_SHAPE_GENERATION_FROM_LINE::type generation_option = OPTIONS_SHAPE_GENERATION_FROM_LINE::BOTH_SIDES );

		static TArray<TArray<FVector> > generateOutlineMesh( TArray<FVector> shape, TArray<TArray<FVector> > holes );
		static void generateOutlineMesh_v2( TArray<FVector> shape, TArray<TArray<FVector> > holes, float outline_width, TArray<FVector>& shape_increased, TArray<TArray<FVector> >& holes_increased );

	private:
		static void _createMeshOnMainThread( class UProceduralMeshComponent*& out, const int segmentIndexToCreateMesh, const TArray<FVector>& vertices, const TArray<int32>& triangles, const TArray<FVector>& normals, const TArray<FVector2D>& UV0, const TArray<FLinearColor>& vertexColors, const TArray<FProcMeshTangent>& tangents, bool generateCollisions );
};
