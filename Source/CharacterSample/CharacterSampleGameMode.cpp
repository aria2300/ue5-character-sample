// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterSampleGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/PlayerCharacter.h"
#include "Player/PlayerCharacterController.h"


ACharacterSampleGameMode::ACharacterSampleGameMode()
{
	// 設定 PlayerCharacterControllerClass 為你的 C++ APlayerCharacterController
    // PlayerControllerClass = APlayerCharacterController::StaticClass();
    // 使用 ConstructorHelpers 載入藍圖 PlayerController
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/ThirdPerson/Blueprints/PlayerCharacter/BP_PlayerCharacterController.BP_PlayerCharacterController_C")); // 請確認你的藍圖路徑
    if (PlayerControllerBPClass.Class != NULL && PlayerControllerBPClass.Succeeded())
    {
        PlayerControllerClass = PlayerControllerBPClass.Class;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find BP_PlayerCharacterController. Falling back to C++ APlayerCharacterController."));
        PlayerControllerClass = APlayerCharacterController::StaticClass(); // 安全網，但此時 HealthBarWidgetClass 可能為空
    }

    // 繼續使用 ConstructorHelpers 載入你的藍圖角色
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/PlayerCharacter/BP_PlayerCharacter.BP_PlayerCharacter_C"));

    if (PlayerPawnBPClass.Class != NULL && PlayerPawnBPClass.Succeeded()) // 這裡用 && 比較嚴謹
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find BP_PlayerCharacter.BP_PlayerCharacter_C in GameMode. Falling back to default C++ Pawn if any."));
        // 如果在藍圖找不到時，至少生成 C++ 版本
        DefaultPawnClass = APlayerCharacter::StaticClass();
    }	
}
