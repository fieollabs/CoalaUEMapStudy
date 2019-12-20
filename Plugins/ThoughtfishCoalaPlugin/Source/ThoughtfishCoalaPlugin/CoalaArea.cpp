// Copyright Thoughtfish GmbH, 2019
// http://www.thoughtfish.de

#include "CoalaArea.h"

bool
UCoalaCell::hastGametag( FString gametag )
{
	if( gameTags.Find(gametag) )
		return true;

	return false;
}

bool
UCoalaCell::isGametagHighest( FString gametag )
{
	int max_priority_in_list = 0;
	int priority_gametag_search = 0;
	for( auto it = gameTags.begin(); it != gameTags.end(); ++it )
	{
		if( it->Key.Compare(gametag) == 0 )
			priority_gametag_search = it->Value;
		if( it->Value > max_priority_in_list )
			max_priority_in_list = it->Value;
	}

	if( max_priority_in_list == priority_gametag_search )
		return true;
	
	return false;
}
