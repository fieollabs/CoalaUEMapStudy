// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "Runtime/Engine/Classes/Engine/StaticMesh.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaDecorator.generated.h"

UENUM(BlueprintType,Blueprintable,Meta = (Bitmask, Bitflags, UseEnumValuesAsMaskValuesInEditor="true", BitmaskEnum="OPTIONS_AREA_DECORATION_IGNORE") )
enum class OPTIONS_AREA_DECORATION_IGNORE : uint8
{
	WATER = 1,	
	STREETS = 2,		
	BUILDINGS = 4
};
ENUM_CLASS_FLAGS( OPTIONS_AREA_DECORATION_IGNORE )

UENUM(BlueprintType,Blueprintable,Meta = (Bitmask, Bitflags, UseEnumValuesAsMaskValuesInEditor="true", BitmaskEnum="OPTIONS_AREA_DECORATION_USE_CONFIG_TO") )
enum class OPTIONS_AREA_DECORATION_USE_CONFIG_TO : uint8
{
	CELLS = 1,	
//	WATER = 2,		
//	BUILDINGS = 4
};
ENUM_CLASS_FLAGS( OPTIONS_AREA_DECORATION_USE_CONFIG_TO )

USTRUCT(BlueprintType, meta=(HiddenByDefault))
struct FCoalaAreaDecorationConfiguration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config")
	TArray<FString> gametag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config", meta=(Bitmask, Bitflags, UseEnumValuesAsMaskValuesInEditor="true", BitmaskEnum="OPTIONS_AREA_DECORATION_USE_CONFIG_TO"))
	OPTIONS_AREA_DECORATION_USE_CONFIG_TO useAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config", meta=(Bitmask, Bitflags, UseEnumValuesAsMaskValuesInEditor="true", BitmaskEnum="OPTIONS_AREA_DECORATION_IGNORE"))
	int32 skipIf;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config")
	class UStaticMesh* decoration;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config")
	FVector randomScaleMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config")
	FVector randomScaleMin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config")
	uint8 countRetriesIfPositionIsOccupied;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coala area decoration config")
	bool onlyIfGametagIsHighest;
};

UCLASS()
class UCoalaDecorator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

		UFUNCTION(BlueprintCallable, Category="Coala|CoalaDecorator")
		static void DecorateArea( class UCoalaArea* area, TArray<FCoalaAreaDecorationConfiguration> configs );
		
	private:
		static class ACoalaActor* decorateCells( int config_index, class UCoalaArea* area, FCoalaAreaDecorationConfiguration config );
};
