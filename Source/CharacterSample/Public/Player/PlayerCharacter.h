// Fill out your copyright notice in the Description page of Project Settings.
// PlayerCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 包含 Enhanced Input 相關的頭文件
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/SpringArmComponent.h" // SpringArmComponent 的頭文件 (如果使用相機臂)
#include "Camera/CameraComponent.h" // CameraComponent 的頭文件 (如果使用相機)
#include "GameFramework/CharacterMovementComponent.h" // CharacterMovementComponent 的頭文件
#include "Kismet/KismetMathLibrary.h" // 用於 GetForwardVector 等
#include "Core/CharacterBase.h" // 引入基礎角色類別

#include "PlayerCharacter.generated.h" // 確保這是最後一個 #include

// 前向聲明 CombatComponent
class UCombatComponent;
// 前向聲明 CharacterInputManagerComponent
class UCharacterInputManagerComponent; // 前向聲明我們的輸入管理組件
class UEntranceAnimationComponent; // 前向聲明入場動畫組件
class UAnimMontage; // 雖然攻擊蒙太奇移走了，但入場動畫還在這裡

UCLASS()
class CHARACTERSAMPLE_API APlayerCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    // 構造函數：設定角色屬性的預設值
    APlayerCharacter();

    // Tick：每一幀都會呼叫 (可以關閉以提升性能，如果不需要每幀更新)
    virtual void Tick(float DeltaTime) override;

    // SetupPlayerInputComponent：將功能綁定到輸入系統
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ====================================================================
    // >>> 動畫相關屬性 <<<
    // 這些變數會傳遞給動畫藍圖，用於控制動畫混合
    // ====================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    float CurrentSpeed; // 角色的當前速度

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    float MovementDirection; // 相對於角色前方向量的移動方向 (角度)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    bool bIsFalling; // 角色是否正在下落
	
	// ====================================================================
	// >>> 組件引用 <<<
	// ====================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCombatComponent* CombatComponent;

    // ====================================================================
    // >>> 輸入管理組件引用 <<<
    // ====================================================================
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCharacterInputManagerComponent* CharacterInputManagerComponent;

    // 指向入場動畫組件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UEntranceAnimationComponent* EntranceAnimationComponent;

    // ====================================================================
    // >>> 輸入處理函數 <<<
    // ====================================================================

    void Move(const FInputActionValue& Value); // 處理移動輸入
    void Look(const FInputActionValue& Value); // 處理視角輸入
    
    // 覆寫 Character 類的跳躍函數，以處理輸入禁用邏輯
    virtual void Jump() override;
    virtual void StopJumping() override;

    // ====================================================================
    // >>> 輔助函數 <<<
    // ====================================================================

    /**
     * @brief 輔助函數，用於啟用或禁用玩家輸入。
     * @param bEnabled 如果為 true 則啟用輸入，false 則禁用。
     */
    void SetPlayerInputEnabled(bool bEnabled);

protected:
    // BeginPlay：在遊戲開始時或角色被生成時呼叫
    virtual void BeginPlay() override;
};
