// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

// 因為組件需要使用 UAnimMontage 類型
#include "Animation/AnimMontage.h" 
// 因為組件需要訪問擁有者（Character）的移動組件和膠囊體
#include "GameFramework/CharacterMovementComponent.h" 
#include "Components/CapsuleComponent.h"

#include "EntranceAnimationComponent.generated.h"

// 前向聲明 APlayerCharacter，因為這個組件會與其特定的擁有者互動
class APlayerCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHARACTERSAMPLE_API UEntranceAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEntranceAnimationComponent();

	// ====================================================================
    // >>> 入場動畫屬性 (從 APlayerCharacter 移入) <<<
    // ====================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Entrance")
    class UAnimMontage* EntranceMontage; // 用於入場動畫的 AnimMontage 資產

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Entrance")
    bool bIsPlayingEntranceAnimation; // 標誌：指示是否正在播放入場動畫

    /**
     * @brief 播放入場動畫蒙太奇，並禁用玩家輸入及移動。
     * 此函數預計在 APlayerCharacter 的 BeginPlay 中呼叫，或由藍圖呼叫。
     */
    UFUNCTION(BlueprintCallable, Category = "Animation|Entrance") // 讓藍圖可以呼叫這個函數
    void PlayEntranceAnimation();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// ====================================================================
    // >>> 內部引用：擁有的角色 <<<
    // 雖然這個組件可以直接使用 GetOwner()，但為了方便類型轉換和明確目的，
    // 我們會在 BeginPlay 中將 GetOwner() 轉換並保存為 APlayerCharacter*。
    // 這有助於減少在其他函式中重複的 Cast 操作。
    // ====================================================================
    UPROPERTY() // UPROPERTY 確保垃圾回收器不會回收此引用
    APlayerCharacter* OwnerCharacter; // 指向擁有此組件的 APlayerCharacter 實例

    // ====================================================================
    // >>> 入場動畫相關函數 (從 APlayerCharacter 移入) <<<
    // ====================================================================
    
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
};
