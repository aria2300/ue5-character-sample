// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterSampleGameMode.h"
#include "CharacterSampleCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACharacterSampleGameMode::ACharacterSampleGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/PlayerCharacter/BP_PlayerCharacter.BP_PlayerCharacter_C"));

	if (PlayerPawnBPClass.Class != NULL || PlayerPawnBPClass.Succeeded())
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
    else
    {
        // 如果找不到，可以印出一個錯誤訊息方便除錯
        UE_LOG(LogTemp, Warning, TEXT("Failed to find PlayerPawnClass in MyGameModeCPP."));
    }
}
