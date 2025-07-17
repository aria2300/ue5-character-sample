// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h" // ULocalPlayer::GetSubsystem 的頭文件
#include "EnhancedInputComponent.h" // UEnhancedInputComponent 的頭文件

#include "CharacterInputManagerComponent.generated.h"

// 前向聲明 APlayerCharacter，因為這個組件需要引用它所附加的角色
class APlayerCharacter;

// 前向聲明 UCombatComponent，因為我們需要將 AttackAction 綁定到 CombatComponent 的函式
class UCombatComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHARACTERSAMPLE_API UCharacterInputManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterInputManagerComponent();

    // ====================================================================
    // >>> 輸入屬性 (Enhanced Input System) <<<
    // 這些屬性會讓你在藍圖中可以設定對應的輸入資產
    // ====================================================================
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext; // 預設的輸入映射上下文

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction; // 移動輸入動作

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction; // 跳躍輸入動作

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction; // 視角輸入動作

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* AttackAction; // 攻擊輸入動作

	// ====================================================================
	// >>> 輸入綁定函式 <<<
	// ====================================================================
	/**
	 * @brief 初始化並綁定所有的 Enhanced Input 動作。
	 * 此函式將在 OwnerCharacter 的 SetupPlayerInputComponent 中呼叫。
	 * @param PlayerInputComponent 玩家輸入組件。
	 * @param InOwnerCharacter 附加此組件的 APlayerCharacter 實例。
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetupInputBindings(UInputComponent* PlayerInputComponent, APlayerCharacter* InOwnerCharacter, UCombatComponent* InCombatComponent);


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// ====================================================================
	// >>> 內部引用：擁有的角色及其組件 <<<
	// ====================================================================
	UPROPERTY()
	UCombatComponent* CombatComponentRef; // 指向 OwnerCharacter 的 CombatComponent

	// ====================================================================
	// >>> 輸入處理的內部實現 <<<
	// 這些函式將響應綁定的輸入動作
	// ====================================================================
	
	// 注意：Move, Look, Jump, StopJumping 這些行為本身仍然在 PlayerCharacter 中
	// 這裡只是定義了它們的簽名，以便可以將它們綁定到 PlayerCharacter 的實作上
	// 或者，如果這些行為也需要完全獨立，未來也可以考慮移動到各自的組件
	// 但目前為了保持角色移動邏輯的完整性，我們將這些輸入事件轉發給 OwnerCharacter。

	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleJumpInputStarted(const FInputActionValue& Value);
	void HandleJumpInputCompleted(const FInputActionValue& Value);
	void HandleAttackInputStarted(const FInputActionValue& Value);
};
