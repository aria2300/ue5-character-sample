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
#include "Kismet/KismetMathLibrary.h" // 用於 GetForwardVector 等

#include "PlayerCharacter.generated.h" // 確保這是最後一個 #include

UCLASS()
class CHARACTERSAMPLE_API APlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // 構造函數：設定角色屬性的預設值
    APlayerCharacter();

public:    
    // Tick：每一幀都會呼叫 (可以關閉以提升性能，如果不需要每幀更新)
    virtual void Tick(float DeltaTime) override;

    // SetupPlayerInputComponent：將功能綁定到輸入系統
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ====================================================================
    // >>> 輸入屬性 (Enhanced Input System) <<<
    // 這些屬性會讓你在藍圖中可以設定對應的輸入資產
    // ====================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext; // 預設的輸入映射上下文

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction; // 移動輸入動作

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* JumpAction; // 跳躍輸入動作

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction; // 視角輸入動作

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* AttackAction; // 攻擊輸入動作

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
    // >>> 入場動畫屬性 <<<
    // ====================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Entrance")
    class UAnimMontage* EntranceMontage; // 用於入場動畫的 AnimMontage 資產

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Entrance")
    bool bIsPlayingEntranceAnimation; // 標誌：指示是否正在播放入場動畫

    // ====================================================================
    // >>> 普通攻擊動畫屬性與狀態 <<<
    // ====================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Attack")
    class UAnimMontage* NormalAttackMontage; // 用於普通攻擊的 AnimMontage 資產

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Attack")
    bool bIsAttacking; // 標誌：指示是否正在播放攻擊動畫

protected:
    // BeginPlay：在遊戲開始時或角色被生成時呼叫
    virtual void BeginPlay() override;

    // ====================================================================
    // >>> 輸入處理函數 <<<
    // ====================================================================

    void Move(const FInputActionValue& Value); // 處理移動輸入
    void Look(const FInputActionValue& Value); // 處理視角輸入
    
    // 覆寫 Character 類的跳躍函數，以處理輸入禁用邏輯
    virtual void Jump() override;
    virtual void StopJumping() override;

    // ====================================================================
    // >>> 入場動畫相關函數 <<<
    // ====================================================================

    /**
     * @brief 播放入場動畫蒙太奇，並禁用玩家輸入及移動。
     * 此函數會在 BeginPlay 時呼叫。
     */
    UFUNCTION(BlueprintCallable, Category = "Animation|Entrance") // 讓藍圖可以呼叫這個函數
    void PlayEntranceAnimation();

    /**
     * @brief 處理來自入場蒙太奇的 Anim Notify 事件。
     * 此函數通常會從動畫藍圖中透過 Anim Notify 事件呼叫。
     */
    UFUNCTION(BlueprintCallable, Category = "Animation|Entrance") // 讓藍圖可以呼叫這個函數
    void OnEntranceAnimationFinishedByNotify();

    /**
     * @brief AnimMontage 播放結束或被中斷時的回調函數。
     * 此函數作為安全網，以防 Anim Notify 因任何原因未能觸發。
     */
    UFUNCTION() // 宣告為 UFunction 以便被 Unreal 系統呼叫
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    // **新增：攻擊輸入處理函數**
    void Attack(const FInputActionValue& Value);

    // ====================================================================
    // >>> **新增：攻擊相關函數** <<<
    // ====================================================================

    /**
     * @brief 播放普通攻擊動畫蒙太奇並禁用移動/輸入。
     */
    UFUNCTION(BlueprintCallable, Category = "Animation|Attack")
    void PlayNormalAttack();

    /**
     * @brief 處理來自普通攻擊蒙太奇的 Anim Notify 事件，執行傷害判定。
     * 此函數通常從動畫藍圖中透過 Anim Notify 事件呼叫 (例如：AnimNotify_HitCheck)。
     */
    UFUNCTION(BlueprintCallable, Category = "Animation|Attack")
    void PerformNormalAttackHitCheck();

    /**
     * @brief 回調函數：普通攻擊蒙太奇播放結束或被中斷。
     * 用於在攻擊動畫結束後，恢復角色狀態。
     */
    UFUNCTION()
    void OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);


private:
    // ====================================================================
    // >>> 輔助函數 <<<
    // ====================================================================

    /**
     * @brief 輔助函數，用於啟用或禁用玩家輸入。
     * @param bEnabled 如果為 true 則啟用輸入，false 則禁用。
     */
    void SetPlayerInputEnabled(bool bEnabled);
};
