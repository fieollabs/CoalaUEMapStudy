// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CoalaTestGameMode.h"
#include "CoalaTestPawn.h"

ACoalaTestGameMode::ACoalaTestGameMode()
{
	// set default pawn class to our character class
	DefaultPawnClass = ACoalaTestPawn::StaticClass();
}

