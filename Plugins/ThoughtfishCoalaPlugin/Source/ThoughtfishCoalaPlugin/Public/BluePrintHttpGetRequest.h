// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Http.h"
#include "Json.h"
#include "BluePrintHttpGetRequest.generated.h"


// Generate a delegate for the OnGetResult event
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FOnGetResult, const bool, bSuccess, class UJsonFieldData*, JSON, const EJSONResult, Status );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnSuccess, FString, response );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnError, FString, reason );

UCLASS( BlueprintType, Blueprintable, Category = "Coala|http" )
class UBluePrintHttpGetRequest
: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		//UFUNCTION( BlueprintCallable, meta = (DisplayName = "Get JSON Request"), Category = "Coala|http" )
		UFUNCTION( BlueprintCallable, Category = "Coala|http" )
		static UBluePrintHttpGetRequest* StartRequest( const FString& url );

		void OnResponseReceived( FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful );

		UPROPERTY( BlueprintAssignable, Category = "Coala|http" )
		FOnSuccess OnSuccess;
		UPROPERTY( BlueprintAssignable, Category = "Coala|http" )
		FOnError OnError;
};
