// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerCharacterController.h"
#include "Blueprint/UserWidget.h" // 需要這個來使用 CreateWidget
#include "HealthBarBaseWidget.h" // 需要這個來訪問 UHealthBarBaseWidget 的成員
#include "Player/PlayerCharacter.h" // 需要這個來 Cast 到 PlayerCharacter
#include "EnhancedInputSubsystems.h" // 如果你還要在這裡放輸入設定，就需要這個
#include "InputMappingContext.h" // 如果你還要在這裡放輸入設定，就需要這個


APlayerCharacterController::APlayerCharacterController()
{
    // 如果你沒有特定的初始化邏輯，這裡可以留空，或者放一些預設值設定
    // 例如：
    // bShowMouseCursor = true;
    // DefaultMouseCursor = EMouseCursor::Crosshairs;
}


void APlayerCharacterController::BeginPlay()
{
    Super::BeginPlay();

    // 僅本地玩家才創建 UI
    if (IsLocalPlayerController()) // 更精確的檢查是否為本地玩家控制器
    {
        CreateAndSetupHealthBarWidget(); // 呼叫輔助函式來創建血條
        
        // --- 這裡可以放置輸入系統設定 ---
        // 取得 Enhanced Input Local Player 子系統。
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
        if (Subsystem)
        {
            // 注意：DefaultMappingContext 現在應該在 PlayerController 的藍圖實例中設置
            // 或者你在 PlayerCharacterController 中聲明並在此處加載
            // 例如：Subsystem->AddMappingContext(DefaultMappingContext, 0); 
        }
    }
}

void APlayerCharacterController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // 當控制器擁有一個 Pawn 時，更新血條 Widget 的擁有角色
    if (HealthBarWidgetInstance)
    {
        APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(InPawn);
        if (PlayerChar)
        {
            HealthBarWidgetInstance->SetOwnerCharacterAndInitialize(PlayerChar);
            UE_LOG(LogTemp, Log, TEXT("PlayerCharacterController possessed new Character and updated HealthBar."));
        }
    }
}

void APlayerCharacterController::OnUnPossess()
{
    Super::OnUnPossess();

    // 當控制器失去一個 Pawn 時，可以選擇清理血條 Widget
    if (HealthBarWidgetInstance)
    {
        HealthBarWidgetInstance->RemoveFromParent(); // 從視口中移除血條 Widget
        HealthBarWidgetInstance = nullptr; // 清空指針
        UE_LOG(LogTemp, Log, TEXT("PlayerCharacterController unpossessed Character and cleared HealthBar."));
    }
}

void APlayerCharacterController::CreateAndSetupHealthBarWidget()
{
    if (HealthBarWidgetClass)
    {
        // 注意這裡的 Owner 是 this (PlayerController)
        HealthBarWidgetInstance = CreateWidget<UHealthBarBaseWidget>(this, HealthBarWidgetClass);

        if (HealthBarWidgetInstance)
        {
            // 在 BeginPlay 時，PlayerController 可能還沒有 Possess 任何 Character，或者還沒有生成。
            // 因此，這裡不能直接依賴 GetPawn() 或 GetCharacter()。
            // 我們會在 OnPossess 中設置血條的 OwnerCharacter。
            // 這裡只是創建並添加到視口。
            HealthBarWidgetInstance->AddToViewport();
            UE_LOG(LogTemp, Log, TEXT("HealthBar Widget created and added to viewport by PlayerCharacterController."));
            
            // 由於 PlayerController 在 BeginPlay 時可能還未 Possess Character
            // 所以在 OnPossess 中再進行 Character 的設定
            APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(GetPawn());
            if (PlayerChar)
            {
                HealthBarWidgetInstance->SetOwnerCharacterAndInitialize(PlayerChar);
                UE_LOG(LogTemp, Log, TEXT("HealthBar Widget initialized with current Character during CreateAndSetup."));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HealthBarWidgetClass is not set in PlayerCharacterController."));
    }
}
