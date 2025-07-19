// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/EntranceAnimationComponent.h"
#include "Player/PlayerCharacter.h" // 需要包含 APlayerCharacter 的頭文件，以便訪問其成員
#include "Components/SkeletalMeshComponent.h" // 因為需要 GetMesh() 來獲取 AnimInstance
#include "Animation/AnimInstance.h" // 為了呼叫 AnimInstance->Montage_Play/RemoveDynamic/AddDynamic
#include "GameFramework/CharacterMovementComponent.h" // 因為需要 GetCharacterMovement()
#include "Components/CapsuleComponent.h" // 因為需要 GetCapsuleComponent()
#include "Engine/Engine.h" // 用於 UE_LOG 訊息

// Sets default values for this component's properties
UEntranceAnimationComponent::UEntranceAnimationComponent()
{
	// 啟用 Tick 函式，如果組件需要每幀更新，可以將其開啟。
	// 對於入場動畫這種一次性或事件驅動的組件，通常不需要 Tick，可以保持關閉以優化。
	PrimaryComponentTick.bCanEverTick = false; 

	// 初始化入場動畫的旗標，預設為未播放。
	bIsPlayingEntranceAnimation = false;
}


// Called when the game starts
void UEntranceAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	// 嘗試將擁有者轉換為 APlayerCharacter 類型
	OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("EntranceAnimationComponent: This component must be attached to an APlayerCharacter!"));
		// 如果沒有正確的 OwnerCharacter，此組件的許多功能將無法正常工作
	}
}

// ====================================================================
// >>> 入場動畫實作 (從 PlayerCharacter.cpp 移入，並調整所有對擁有者組件的引用) <<<
// 控制遊戲開始時的動畫播放與玩家控制權的啟用。
// ====================================================================

// 播放入場動畫。
void UEntranceAnimationComponent::PlayEntranceAnimation()
{
    // 確保組件有有效的擁有者角色
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("EntranceAnimationComponent: OwnerCharacter is null in PlayEntranceAnimation. Cannot play."));
        return;
    }

    if (EntranceMontage) // 檢查是否有設定入場動畫 Montage (UAnimMontage 變數，需在藍圖中指定)
    {
        // 透過 OwnerCharacter 獲取 SkeletalMeshComponent，再獲取其 AnimInstance
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance(); 
        if (AnimInstance)
        {
            // 播放蒙太奇，速度為 1.0f (正常速度)。返回蒙太奇的實際持續時間。
            float Duration = AnimInstance->Montage_Play(EntranceMontage, 1.0f); 
            
            if (Duration > 0.0f) // 如果蒙太奇成功播放 (持續時間大於 0)
            {
                bIsPlayingEntranceAnimation = true; // 設定旗標為 true，表示正在播放入場動畫
                
                // 綁定到 Montage 結束事件作為安全網。
                // 先移除現有的委託，以防止重複綁定 (如果 PlayEntranceAnimation 被再次呼叫)。
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UEntranceAnimationComponent::OnMontageEnded); 
                // 添加動態委託，當蒙太奇結束時，呼叫 OnMontageEnded 函式。
                AnimInstance->OnMontageEnded.AddDynamic(this, &UEntranceAnimationComponent::OnMontageEnded);
                
                // 在動畫期間禁用角色移動組件，防止玩家移動。
                // 透過 OwnerCharacter 獲取角色移動組件
                OwnerCharacter->GetCharacterMovement()->DisableMovement();
                // 在動畫期間禁用膠囊碰撞體的碰撞，防止角色被推動或卡住。
                // 透過 OwnerCharacter 獲取膠囊碰撞體
                OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); 

                // 禁用玩家輸入 (透過 OwnerCharacter 呼叫其輔助函式)
                OwnerCharacter->SetPlayerInputEnabled(false);
            }
            else
            {
                // 如果 Montage_Play 返回 0.0 (例如，蒙太奇無效或播放失敗)
                UE_LOG(LogTemp, Warning, TEXT("EntranceAnimationComponent: Montage_Play failed for EntranceMontage. Enabling Player Input."));
                // 如果蒙太奇無法播放，立即重新啟用輸入，避免卡住
                OwnerCharacter->SetPlayerInputEnabled(true); 
            }
        }
        else // 如果 AnimInstance 為空 (表示網格或動畫實例有問題)
        {
            UE_LOG(LogTemp, Warning, TEXT("EntranceAnimationComponent: AnimInstance is null for OwnerCharacter. Enabling Player Input."));
            // 如果 AnimInstance 為空，立即重新啟用輸入
            OwnerCharacter->SetPlayerInputEnabled(true); 
        }
    }
    else // 如果沒有設定入場動畫 Montage
    {
        UE_LOG(LogTemp, Warning, TEXT("EntranceAnimationComponent: EntranceMontage is not set! Enabling Player Input."));
        // 如果沒有指定蒙太奇，直接啟用輸入
        OwnerCharacter->SetPlayerInputEnabled(true); 
    }
}

// 由動畫藍圖透過 Anim Notify 呼叫。
// 標誌著玩家應該重新獲得控制權的精確時刻。這是預期的入場動畫結束方式。
void UEntranceAnimationComponent::OnEntranceAnimationFinishedByNotify()
{
    // 確保組件有有效的擁有者角色
    if (!OwnerCharacter) return;

    // 只有當我們處於入場動畫狀態時才執行，避免重複執行或在錯誤時機執行。
    if (bIsPlayingEntranceAnimation) 
    {
        UE_LOG(LogTemp, Log, TEXT("EntranceAnimationComponent: Entrance Animation Finished by Anim Notify! Enabling Player Input."));
        bIsPlayingEntranceAnimation = false; // 設定旗標為 false，表示入場動畫結束
        
        // 啟用玩家輸入 (透過 OwnerCharacter 呼叫)
        OwnerCharacter->SetPlayerInputEnabled(true); 

        // 重新啟用角色移動，將移動模式設定回步行。
        OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking); 
        // 重新啟用膠囊碰撞體，使其能夠再次與世界互動。
        OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // 在通過 Notify 成功完成後，移除 OnMontageEnded 委託，以避免多餘的調用。
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &UEntranceAnimationComponent::OnMontageEnded);
        }
    }
}

// 進場動畫結束的保險機制。
// 這個函式由 `AnimInstance->OnMontageEnded` 委託調用。
// 如果蒙太奇因任何原因結束 (完成或中斷)，並且玩家輸入尚未被 Notify 重新啟用，則強制重新啟用它。
void UEntranceAnimationComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 確保組件有有效的擁有者角色
    if (!OwnerCharacter) return;

    // 檢查結束的蒙太奇是否是我們的入場蒙太奇，並且我們仍然處於「正在播放入場動畫」的狀態。
    if (Montage == EntranceMontage && bIsPlayingEntranceAnimation) 
    {
        UE_LOG(LogTemp, Warning, TEXT("EntranceAnimationComponent: Entrance Montage Ended (safety net triggered)! Forcing Input Enabled."));
        bIsPlayingEntranceAnimation = false; // 設定旗標為 false
        
        // 強制啟用玩家輸入 (透過 OwnerCharacter 呼叫)
        OwnerCharacter->SetPlayerInputEnabled(true); 
        OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 恢復移動
        OwnerCharacter->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 恢復碰撞

        // 總是移除委託，以防止陳舊的綁定或重複調用。
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &UEntranceAnimationComponent::OnMontageEnded);
        }
    }
}
