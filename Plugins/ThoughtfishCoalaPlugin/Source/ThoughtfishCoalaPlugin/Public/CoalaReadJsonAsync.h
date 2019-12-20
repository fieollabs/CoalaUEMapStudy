// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

#include "Core/Public/Async/Async.h"


class FCoalaReadJsonAsync : public FRunnable
{
public:
	static FCoalaReadJsonAsync* Runnable;

	FRunnableThread* Thread;
	class UCoalaArea* result;
	FString Raw;
	int _defaultBuildingLevel;
	bool _clampToDefaultBuildingLevel;
	int _limitMaxBuildingLevelTo;
	FThreadSafeCounter StopTaskCounter;
	static bool isDone;
	void JsonToCoalaArea(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, UCoalaArea* result);

	FCoalaReadJsonAsync(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, UCoalaArea* initializedObject);
	virtual ~FCoalaReadJsonAsync();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	//virtual void Stop();
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	static UCoalaArea* GetResult();
	static FCoalaReadJsonAsync* JoyInit(FString JsonRaw, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, UCoalaArea* initializedObject);

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();
	static void JsonToCoalaArea(FString jsonString, int defaultBuildingLevel, bool clampToDefaultBuildingLevel, int limitMaxBuildingLevelTo, bool async, UCoalaArea* ret);

};
