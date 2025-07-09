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
#include "Core/CharacterBase.h" // 引入基礎角色類別

#include "PlayerCharacter.generated.h" // 確保這是最後一個 #include

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
    // >>> Combo 攻擊相關函數 <<<
    // ====================================================================
    UFUNCTION(BlueprintCallable, Category = "Attack") // 提供給 Anim Notify 調用
    void PerformNormalAttackHitCheck(); // 執行攻擊判定

    UFUNCTION() // UFUNCTION 必須要有，因為它要被 AddDynamic 委託綁定
    void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 攻擊蒙太奇結束處理 (用於 Combo 邏輯)

    UFUNCTION(BlueprintCallable, Category = "Attack") // 提供給 Anim Notify 調用，設置下一段 Combo 的輸入窗口
    void SetCanEnterNextCombo(bool bCan);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetPendingNextComboInput(bool bPending);

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

    // 攻擊輸入處理函數
    void Attack(const FInputActionValue& Value);

    // ====================================================================
    // >>> Combo 攻擊相關屬性 <<<
    // ====================================================================
    bool bPendingNextComboInput; // 新增：標記是否有待處理的下一段Combo輸入

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
    bool bIsAttacking; // 是否正在進行攻擊（包括 Combo 的任何一段）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
    int32 CurrentAttackComboIndex; // 當前的 Combo 段數 (從 0 開始)

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack") // EditDefaultsOnly 方便在藍圖默認值中設置
    TArray<UAnimMontage*> AttackMontages; // 儲存所有攻擊蒙太奇的數組 (按照 Combo 順序)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
    bool bCanEnterNextCombo; // 是否可以在當前攻擊蒙太奇的特定時間點輸入下一段 Combo

    FTimerHandle ComboWindowTimerHandle; // 用於管理 Combo 輸入窗口的定時器

    // 考慮讓攻擊函數更通用，或者創建一個 StartAttackCombo
    // UFUNCTION(BlueprintCallable, Category = "Attack")
    // void StartAttackCombo(); // 如果攻擊觸發邏輯在藍圖，可以這樣

    // 內部 Combo 邏輯函數
    void PlayAttackComboSegment(); // 播放指定 Combo 段數的攻擊動畫
    void TryEnterNextCombo(); // 嘗試進入下一段 Combo
    void ResetCombo(); // 重設 Combo 狀態
    
    UFUNCTION() // UFUNCTION 必須要有，因為它要被 SetTimerbyEvent 調用
    void OnComboWindowEnd(); // Combo 輸入窗口結束時調用

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
