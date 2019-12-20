// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include <map>
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoalaRemoteTileRequest.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType, meta=(HiddenByDefault))
struct FCoalaRemoteTileRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="zoom")
	uint8 zoom;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="tile_x")
	int tile_x;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="tile_y")
	int tile_y;

	FORCEINLINE bool operator != (const FCoalaRemoteTileRequest &other) const
	{
		return this->zoom!=other.zoom && this->tile_x!=other.tile_x && this->tile_y!=other.tile_y;
	}
	
	FORCEINLINE bool operator == (const FCoalaRemoteTileRequest &other) const
	{
		return this->zoom==other.zoom && this->tile_x==other.tile_x && this->tile_y==other.tile_y;
	}
};

// that we can have this struct as a key in std::map (ignoring zoom)
namespace std
{
	template<>
	struct less<FCoalaRemoteTileRequest>
	{
	   bool operator() (const FCoalaRemoteTileRequest& lhs, const FCoalaRemoteTileRequest& rhs) const
	   {
		   return lhs.tile_y < rhs.tile_y || (lhs.tile_y == rhs.tile_y && lhs.tile_x < rhs.tile_x);
		   //return lhs.tile_x < rhs.tile_x && lhs.tile_y < rhs.tile_y && lhs.zoom < rhs.zoom;
	   }
	};
}

FORCEINLINE bool operator == (FCoalaRemoteTileRequest& a, FCoalaRemoteTileRequest b){return a.zoom==b.zoom && a.tile_x==b.tile_x && a.tile_y==b.tile_y; }
FORCEINLINE bool operator != (FCoalaRemoteTileRequest& a, FCoalaRemoteTileRequest b){return a.zoom!=b.zoom && a.tile_x!=b.tile_x && a.tile_y!=b.tile_y; }
