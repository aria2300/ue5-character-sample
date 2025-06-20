// Fill out your copyright notice in the Description page of Project Settings.
// PlayerCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// 包含 Enhanced Input 相關的頭文件
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h" // ULocalPlayer::GetSubsystem 的頭文件
#include "EnhancedInputComponent.h" // UEnhancedInputComponent 的頭文件
#include "GameFramework/SpringArmComponent.h" // SpringArmComponent 的頭文件 (如果使用相機臂)
#include "Camera/CameraComponent.h" // CameraComponent 的頭文件 (如果使用相機)
#include "GameFramework/CharacterMovementComponent.h" // CharacterMovementComponent 的頭文件

#include "PlayerCharacter.generated.h" // 確保這是最後一個 #include

UCLASS()
class CHARACTERSAMPLE_API APlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    APlayerCharacter();

protected:
    virtual void BeginPlay() override;

public:    
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // 呼叫這一步，以便綁定輸入
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ====================================================================
    // >>> Enhanced Input 相關屬性 <<<
    // ====================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* JumpAction;
    
    // 新增：Look/Turn Action (用於滑鼠或搖桿控制視角)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction; // 這個 Action 的 Value Type 應該是 Vector2D

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;

    // ====================================================================
    // >>> 動畫相關屬性 (暴露給藍圖用於 ABP) <<<
    // ====================================================================

    /** 角色的當前速度 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    float CurrentSpeed;

    /** 角色的移動方向 (相對於角色的前向)，用於動畫藍圖 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    float MovementDirection; // -180 到 180 度

    /** 角色是否正在空中 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Animation")
    bool bIsFalling;

protected:
    // ====================================================================
    // >>> Input Action 綁定函數 <<<
    // ====================================================================

    // 移動函數，接收 FInputActionValue (來自 IA_Move 的 Vector2D 值)
    void Move(const FInputActionValue& Value);

    // 視角控制函數，接收 FInputActionValue (來自 IA_Look 的 Vector2D 值)
    void Look(const FInputActionValue& Value);
};
