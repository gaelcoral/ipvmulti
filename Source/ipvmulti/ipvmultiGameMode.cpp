// Copyright Epic Games, Inc. All Rights Reserved.

#include "ipvmultiGameMode.h"
#include "ipvmultiCharacter.h"
#include "UObject/ConstructorHelpers.h"

AipvmultiGameMode::AipvmultiGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
