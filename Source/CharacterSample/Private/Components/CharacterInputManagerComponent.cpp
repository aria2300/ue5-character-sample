// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CharacterInputManagerComponent.h"
#include "Player/PlayerCharacter.h" // 必須包含 APlayerCharacter 的頭檔，因為我們要訪問它
#include "Components/CombatComponent.h" // 必須包含 CombatComponent 的頭檔，因為我們要綁定其攻擊函式

// Enhanced Input 相關的頭檔
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

// ====================================================================
// >>> 構造函數：UCharacterInputManagerComponent::UCharacterInputManagerComponent() <<<
// 設定組件的預設值
// ====================================================================
UCharacterInputManagerComponent::UCharacterInputManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// PrimaryComponentTick.bCanEverTick = true;
}


// ====================================================================
// >>> BeginPlay() 函式：在遊戲開始時或組件被生成時呼叫 <<<
// 在這裡獲取 OwnerCharacter 和 CombatComponent 的引用，並初始化輸入映射上下文。
// ====================================================================
void UCharacterInputManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 獲取 CombatComponent 的引用，用於綁定攻擊動作
	CombatComponentRef = Cast<APlayerCharacter>(GetOwner())->FindComponentByClass<UCombatComponent>(); 
	if (!CombatComponentRef)
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterInputManagerComponent: CombatComponent not found on OwnerCharacter!"));
	}

	// --- 複製 PlayerCharacter::BeginPlay 中的輸入系統設定 ---
	APlayerController* PlayerController = Cast<APlayerController>(Cast<APlayerCharacter>(GetOwner())->GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
            // 添加預設的輸入映射上下文。
            // DefaultMappingContext 是一個 UInputMappingContext* 變數，你需要在藍圖中為它賦值。
            if (DefaultMappingContext)
            {
			    Subsystem->AddMappingContext(DefaultMappingContext, 0); // 優先級設為 0
                UE_LOG(LogTemp, Log, TEXT("CharacterInputManagerComponent: DefaultMappingContext added."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("CharacterInputManagerComponent: DefaultMappingContext is not set!"));
            }
		}
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterInputManagerComponent: EnhancedInputLocalPlayerSubsystem is null!"));
        }
	}
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterInputManagerComponent: PlayerController is null in BeginPlay!"));
    }
}

// ====================================================================
// >>> SetupInputBindings() 函式：負責將輸入動作綁定到對應的處理函式 <<<
// 這個函式將在 APlayerCharacter 的 SetupPlayerInputComponent 中被呼叫。
// ====================================================================
void UCharacterInputManagerComponent::SetupInputBindings(UInputComponent* PlayerInputComponent, APlayerCharacter* InOwnerCharacter, UCombatComponent* InCombatComponent) // <-- 修改函式簽名
{
    // 在這裡將傳入的 InCombatComponent 賦值給成員變數 CombatComponentRef
    CombatComponentRef = InCombatComponent;

    // 檢查傳入的參數
    if (!InOwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterInputManagerComponent: SetupInputBindings called with null InOwnerCharacter!"));
        return;
    }

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInputComponent)
    {
        // --- 綁定移動輸入 ---
        if (MoveAction)
        {
            // 將 MoveAction 綁定到 InOwnerCharacter 的 Move 函式
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, InOwnerCharacter, &APlayerCharacter::Move); // <-- 使用 InOwnerCharacter
        }
        
        // --- 綁定跳躍輸入 ---
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, InOwnerCharacter, &APlayerCharacter::Jump); // <-- 使用 InOwnerCharacter
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, InOwnerCharacter, &APlayerCharacter::StopJumping); // <-- 使用 InOwnerCharacter
        }

        // --- 綁定視角輸入 ---
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, InOwnerCharacter, &APlayerCharacter::Look); // <-- 使用 InOwnerCharacter
        }

        // --- 綁定攻擊輸入 ---
        if (AttackAction && CombatComponentRef)
        {
            EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, CombatComponentRef, &UCombatComponent::Attack);
        }
        else if (!CombatComponentRef)
        {
             UE_LOG(LogTemp, Warning, TEXT("CharacterInputManagerComponent: CombatComponentRef is null, AttackAction not bound."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterInputManagerComponent: PlayerInputComponent is not an EnhancedInputComponent!"));
    }
}

// ====================================================================
// >>> 暫時不實現 Handle...Input 函式 <<<
// 因為我們直接將輸入事件綁定到 OwnerCharacter 的函式上。
// 這些 Handle...Input 函式只在 CharacterInputManagerComponent 內部處理輸入時才需要。
// 目前它們只是佔位符，如果未來決定完全由這個組件來處理輸入邏輯，再填充它們。
// ====================================================================

void UCharacterInputManagerComponent::HandleMoveInput(const FInputActionValue& Value)
{
    // 如果未來移動邏輯完全由組件處理，則在此實現
}

void UCharacterInputManagerComponent::HandleLookInput(const FInputActionValue& Value)
{
    // 如果未來視角邏輯完全由組件處理，則在此實現
}

void UCharacterInputManagerComponent::HandleJumpInputStarted(const FInputActionValue& Value)
{
    // 如果未來跳躍邏輯完全由組件處理，則在此實現
}

void UCharacterInputManagerComponent::HandleJumpInputCompleted(const FInputActionValue& Value)
{
    // 如果未來停止跳躍邏輯完全由組件處理，則在此實現
}

void UCharacterInputManagerComponent::HandleAttackInputStarted(const FInputActionValue& Value)
{
    // 如果未來攻擊邏輯完全由組件處理，則在此實現
}
