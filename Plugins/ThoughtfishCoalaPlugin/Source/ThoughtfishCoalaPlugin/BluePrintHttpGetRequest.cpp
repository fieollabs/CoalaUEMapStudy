// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "BluePrintHttpGetRequest.h"

UBluePrintHttpGetRequest*
UBluePrintHttpGetRequest::StartRequest( const FString& url )
{
	UBluePrintHttpGetRequest* target = NewObject<UBluePrintHttpGetRequest>();

	// Create the HTTP request
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb( "GET" );
	// make shure url starts with "http"
	HttpRequest->SetURL( url );
	HttpRequest->OnProcessRequestComplete().BindUObject( target, &UBluePrintHttpGetRequest::OnResponseReceived );

//	target->AddToRoot();

	HttpRequest->ProcessRequest();

	return target;
}

void 
UBluePrintHttpGetRequest::OnResponseReceived( FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful )
{

	if( !bWasSuccessful )
	{
		OnError.Broadcast( TEXT("Error while getting data from Server.") );
		return;
	}

/*	FString output;
	output += FString( "Coala server response: " );
	output += Response->GetContentAsString();
	OnSuccess.Broadcast( output );
*/
	OnSuccess.Broadcast( Response->GetContentAsString() );
}