// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"
#include "Player/PlayerCharacter.h" // 需要包含 APlayerCharacter 的頭檔，因為我們要訪問它
#include "GameFramework/CharacterMovementComponent.h" // 訪問角色移動組件
#include "Components/CapsuleComponent.h" // 訪問膠囊碰撞體
#include "Components/SkeletalMeshComponent.h" // 訪問網格模型
#include "Kismet/KismetSystemLibrary.h" // 用於藍圖輔助函數，如 Sweep Trace
#include "Engine/DamageEvents.h" // 用於處理傷害事件
#include "Animation/AnimMontage.h" // 用於動畫蒙太奇
#include "Animation/AnimInstance.h" // 用於動畫實例
#include "Engine/Engine.h" // 用於 GEngine->AddOnScreenDebugMessage

// ====================================================================
// >>> 構造函數：UCombatComponent::UCombatComponent() <<<
// 設定組件的預設值
// ====================================================================
UCombatComponent::UCombatComponent()
{
	// 設定為每幀都 Tick，但我們暫時不需要，可以在 .h 中註釋掉 TickComponent
	// PrimaryComponentTick.bCanEverTick = true;

	CurrentAttackComboIndex = 0;
	bIsAttacking = false;
	bCanEnterNextCombo = false;
	bPendingNextComboInput = false;
	bIsDead = false; // 這裡先保留，未來可能移到 HealthComponent
}


// ====================================================================
// >>> BeginPlay() 函式：在遊戲開始時或組件被生成時呼叫 <<<
// 在這裡獲取 OwnerCharacter 的引用
// ====================================================================
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 在 BeginPlay 中獲取擁有的角色，因為 GetOwner() 在構造函數中可能還無效
	OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("CombatComponent must be attached to an APlayerCharacter!"));
	}
}


// Called every frame
// void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
// {
// 	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

// 	// ...
// }

// ====================================================================
// >>> 攻擊輸入處理 (整合 Combo 邏輯) <<<
// ====================================================================
void UCombatComponent::Attack()
{
    // 如果角色不存在、正在播放入場動畫，或者角色已經死亡，則不允許攻擊，直接返回。
    if (!OwnerCharacter || OwnerCharacter->bIsPlayingEntranceAnimation || bIsDead) return;

    // 判斷是否正在攻擊中。
    if (!bIsAttacking)
    {
        CurrentAttackComboIndex = 0; // 設定當前連擊段數為第一段 (0)
        PlayAttackComboSegment(); // 播放第一段攻擊動畫
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta, TEXT("Starting First Attack!"));
        return; // 啟動第一擊後，直接返回
    }

    // 如果 bIsAttacking 為 true，表示角色正在攻擊中，接下來判斷是否能進入下一段連擊。
    if (bCanEnterNextCombo)
    {
        TryEnterNextCombo(); // 嘗試進入下一段連擊
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Attempting Next Combo (Direct)!"));
        SetPendingNextComboInput(false); // 成功連擊後，清除任何緩衝的輸入旗標
    }
    else
    {
        SetPendingNextComboInput(true);
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Attack input buffered!"));
    }
}

// ====================================================================
// >>> Combo 攻擊實作 <<<
// 負責播放連擊的每一段動畫，並設定相關狀態和計時器。
// ====================================================================

void UCombatComponent::PlayAttackComboSegment()
{
    if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return; // 確保角色和網格存在

    if (AttackMontages.IsValidIndex(CurrentAttackComboIndex) && AttackMontages[CurrentAttackComboIndex])
    {
        UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            UAnimMontage* CurrentMontage = AttackMontages[CurrentAttackComboIndex];
            float Duration = AnimInstance->Montage_Play(CurrentMontage, 1.0f);
            
            if (Duration > 0.0f)
            {
                bIsAttacking = true;
                bCanEnterNextCombo = false;
                
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UCombatComponent::OnAttackMontageEnded);
                AnimInstance->OnMontageEnded.AddDynamic(this, &UCombatComponent::OnAttackMontageEnded);
                
                OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
                OwnerCharacter->GetCharacterMovement()->DisableMovement();
                
                GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle); // 使用 GetWorld() 獲取世界
                GetWorld()->GetTimerManager().SetTimer(ComboWindowTimerHandle, this, &UCombatComponent::OnComboWindowEnd, Duration + 0.1f, false);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed for AttackMontage index %d."), CurrentAttackComboIndex);
                ResetCombo();
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimInstance is null for PlayerCharacter during attack."));
            ResetCombo();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AttackMontage index %d is invalid or Montage is null!"), CurrentAttackComboIndex);
        ResetCombo();
    }
}

void UCombatComponent::TryEnterNextCombo()
{
    if (bCanEnterNextCombo && (CurrentAttackComboIndex + 1) < AttackMontages.Num())
    {
        CurrentAttackComboIndex++;
        PlayAttackComboSegment();
        GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("Entered next combo segment: %d"), CurrentAttackComboIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot enter next combo. Resetting combo."));
        ResetCombo();
    }
}

void UCombatComponent::ResetCombo()
{
    UE_LOG(LogTemp, Log, TEXT("Resetting Attack Combo."));
    CurrentAttackComboIndex = 0;
    bIsAttacking = false;
    bCanEnterNextCombo = false;
    bPendingNextComboInput = false;
    GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
    if (OwnerCharacter && OwnerCharacter->GetCharacterMovement()) // 確保角色存在才恢復移動
    {
        OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Combo Reset!"));
}

void UCombatComponent::OnComboWindowEnd()
{
    if (bIsAttacking && !bCanEnterNextCombo)
    {
        UE_LOG(LogTemp, Log, TEXT("Combo Window Ended without next input. Resetting combo."));
        ResetCombo();
    }
}

void UCombatComponent::SetCanEnterNextCombo(bool bCan)
{
    bCanEnterNextCombo = bCan;
    if (bCan)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Can Enter Next Combo!"));
        if (bPendingNextComboInput)
        {
            TryEnterNextCombo();
            SetPendingNextComboInput(false);
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Buffered input triggered next combo!"));
        }
    }
}

void UCombatComponent::SetPendingNextComboInput(bool bPending)
{
    bPendingNextComboInput = bPending;
    if (bPending)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Input Buffered!"));
    }
}

// ====================================================================
// >>> 攻擊命中檢查實作 <<<
// ====================================================================

void UCombatComponent::PerformNormalAttackHitCheck()
{
    if (!OwnerCharacter || !OwnerCharacter->GetMesh()) return; // 確保角色和網格存在

    UE_LOG(LogTemp, Log, TEXT("Performing Normal Attack Hit Check for Combo Segment: %d"), CurrentAttackComboIndex);

    FVector StartLocation = OwnerCharacter->GetMesh()->GetSocketLocation(TEXT("weapon_l"));
    FVector EndLocation = StartLocation + OwnerCharacter->GetActorForwardVector() * 150.0f;

    TArray<FHitResult> HitResults;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerCharacter); // 忽略 OwnerCharacter，防止自體命中
    Params.bTraceComplex = true;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    FCollisionShape SphereShape = FCollisionShape::MakeSphere(70.0f);

    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        StartLocation,
        EndLocation,
        SphereShape.GetSphereRadius(),
        ObjectTypes,
        false,
        TArray<AActor*>(), // 由於上面已經用 Params.AddIgnoredActor，這裡可以傳空 TArray
        EDrawDebugTrace::ForDuration,
        HitResults,
        true,
        FLinearColor::Red,
        FLinearColor::Green,
        5.0f
    );

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                if (HitActor != OwnerCharacter) // 再次確認不是命中自己
                {
                    UE_LOG(LogTemp, Log, TEXT("攻擊命中: %s"), *HitActor->GetName());
                    FDamageEvent DamageEvent;
                    HitActor->TakeDamage(25.0f, DamageEvent, OwnerCharacter->GetController(), OwnerCharacter); // 使用 OwnerCharacter 的 Controller 和 Actor
                }
            }
        }
    }
}

// ====================================================================
// >>> 動畫蒙太奇結束回調 <<<
// ====================================================================

void UCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 檢查結束的蒙太奇是否是我們攻擊蒙太奇數組中的一個，並且角色仍然處於攻擊狀態。
    if (AttackMontages.Contains(Montage) && bIsAttacking)
    {
        UE_LOG(LogTemp, Log, TEXT("Attack Montage Ended (Index: %d, Interrupted: %s)."), CurrentAttackComboIndex, bInterrupted ? TEXT("True") : TEXT("False"));
        
        if (OwnerCharacter && OwnerCharacter->GetMesh()) // 確保角色和網格存在
        {
            UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
            if (AnimInstance)
            {
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &UCombatComponent::OnAttackMontageEnded);
            }
        }
    }
}
