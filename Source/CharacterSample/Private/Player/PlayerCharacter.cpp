// PlayerCharacter.cpp

#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h" // 用於 Sweep Trace 函數 (如 UKismetSystemLibrary::SphereTraceMultiByChannel)
#include "Engine/DamageEvents.h" // 用於 FDamageEvent
// 必須包含 UserWidget.h，因為 CreateWidget 函數的原型需要它。
#include "Blueprint/UserWidget.h" 
// 包含你的 HealthBarBaseWidget C++ 類別的頭檔，以便能夠使用它的完整定義（例如 SetOwnerCharacterAndInitialize 函數）。
#include "HealthBarBaseWidget.h" 


// Sets default values
APlayerCharacter::APlayerCharacter()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // ====================================================================
    // >>> 角色移動設定，實現「無雙類」轉向 <<<
    // ====================================================================

    // 禁用角色身體的 Yaw 軸旋轉跟隨控制器（攝影機）的 Yaw 軸旋轉
    bUseControllerRotationYaw = false; // 角色的身體將不會跟隨攝影機左右轉動
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // 取得角色移動組件
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        // 啟用：讓角色自動面向其移動的方向 (這是實現無雙式轉向的關鍵)
        MovementComp->bOrientRotationToMovement = true; 
        
        // 設定角色的轉向速度，通常設得非常快，使其轉向看起來幾乎是瞬時的
        // 您可以調整這個值 (例如：2000.0f, 3600.0f, 或更高) 來達到最符合您需求的「瞬時」感。
        MovementComp->RotationRate = FRotator(0.0f, 3600.0f, 0.0f); // 範例：每秒 3600 度，非常快速

        // 設定預設移動速度 (根據您的遊戲設計調整)
        MovementComp->MaxWalkSpeed = 600.0f;
        MovementComp->JumpZVelocity = 600.f;
        MovementComp->AirControl = 0.2f;
    }

    // 設定 SpringArm 和攝影機 (假設您的角色有這些組件)
    // Spring Arm 使用 Pawn Control Rotation 以允許攝影機獨立於角色身體旋轉
    USpringArmComponent* CameraBoom = FindComponentByClass<USpringArmComponent>();
    if (CameraBoom)
    {
        CameraBoom->bUsePawnControlRotation = true; // Spring Arm 將會跟隨控制器的 Yaw 軸旋轉
        CameraBoom->bInheritPitch = true; // 繼承 Pitch 軸旋轉
        CameraBoom->bInheritRoll = true;  // 繼承 Roll 軸旋轉
        CameraBoom->bDoCollisionTest = true; // 啟用碰撞檢測，防止攝影機穿牆
    }

    UCameraComponent* FollowCamera = FindComponentByClass<UCameraComponent>();
    if (FollowCamera)
    {
        FollowCamera->bUsePawnControlRotation = false; // 攝影機本身不直接旋轉，它會跟隨 Spring Arm
    }

    // 初始化入場動畫的旗標
    bIsPlayingEntranceAnimation = false;

    // 預設啟用膠囊碰撞體的碰撞
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

// BeginPlay：在遊戲開始時或角色被生成時呼叫
void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // --- UI 創建邏輯 ---
    // 判斷是否是本地玩家控制的角色，這是為了防止在多人遊戲中，服務器或其他客戶端重複創建 UI。
    if (IsLocallyControlled())
    {
        // 檢查藍圖中是否已經設置了 HealthBarWidgetClass。
        // 如果沒有設置，HealthBarWidgetClass 會是 nullptr。
        if (HealthBarWidgetClass)
        {
            // 使用 CreateWidget 函數來創建你的血條 Widget 實例。
            // Cast<UHealthBarBaseWidget> 確保返回的實例是指向 UHealthBarBaseWidget 類型。
            HealthBarWidgetInstance = CreateWidget<UHealthBarBaseWidget>(GetWorld(), HealthBarWidgetClass);

            // 檢查是否成功創建了 Widget 實例。
            if (HealthBarWidgetInstance)
            {
                // 呼叫你的 HealthBarBaseWidget 中定義的初始化函數，將當前角色 (this) 傳遞給它。
                // 這樣血條 Widget 就能知道它屬於哪個角色，並開始監聽該角色的生命值變化。
                HealthBarWidgetInstance->SetOwnerCharacterAndInitialize(this);

                // 將創建的血條 Widget 添加到遊戲視口中，使其能夠被玩家看到。
                HealthBarWidgetInstance->AddToViewport();
            }
        }
    }

    // 取得玩家控制器
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        // 取得 Enhanced Input Local Player 子系統
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
        if (Subsystem)
        {
            // 添加預設的輸入映射上下文
            Subsystem->AddMappingContext(DefaultMappingContext, 0); // 優先級設為 0
        }
    }

    // ====================================================================
    // >>> 遊戲初始狀態：禁用輸入並播放入場動畫 <<<
    // ====================================================================
    SetPlayerInputEnabled(false); // 遊戲開始時，先禁用玩家輸入
    PlayEntranceAnimation();      // 播放角色的入場動畫
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ====================================================================
    // >>> 動畫狀態變數更新 <<<
    // ====================================================================
    if (GetCharacterMovement())
    {
        CurrentSpeed = GetVelocity().Size(); // 取得當前速度
        bIsFalling = GetCharacterMovement()->IsFalling(); // 判斷是否正在下落

        // 由於 bOrientRotationToMovement 為 true，角色總是面向移動方向。
        // 因此，MovementDirection 通常會接近 0 (相對於自身總是向前)。
        // 這個變數主要用於動畫藍圖中，如果需要混合側身移動或後退動畫時，它仍然有用。
        if (CurrentSpeed > KINDA_SMALL_NUMBER) // 如果有明顯的移動速度
        {
            FVector WorldVelocity = GetVelocity();
            WorldVelocity.Z = 0.f; // 忽略垂直速度，只考慮水平方向

            if (WorldVelocity.SizeSquared() > KINDA_SMALL_NUMBER) // 確保速度不是零向量
            {
                WorldVelocity.Normalize(); // 正規化速度向量

                // 取得角色的當前 Yaw 旋轉 (只考慮水平旋轉)
                const FRotator CharacterYawRotation(0, GetActorRotation().Yaw, 0);
                // 將世界速度向量轉換為相對於角色自身局部空間的速度向量
                FVector LocalVelocity = CharacterYawRotation.UnrotateVector(WorldVelocity);

                // 計算移動方向的角度 (使用 Atan2(Y, X) 得到弧度，再轉換為角度)
                MovementDirection = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
            }
            else
            {
                MovementDirection = 0.0f; // 沒有移動，方向為 0
            }
        }
        else
        {
            MovementDirection = 0.0f; // 沒有移動，方向為 0
        }
    }
}

// SetupPlayerInputComponent：將功能綁定到輸入系統
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInputComponent)
    {
        // 綁定移動輸入動作
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        }
        
        // 綁定跳躍輸入動作
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJumping);
        }

        // 綁定視角輸入動作
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        }

        // 綁定攻擊輸入動作
        if (AttackAction)
        {
            EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::Attack);
        }
    }
}

// ====================================================================
// >>> 輸入處理實作 <<<
// 包含在播放入場動畫期間禁用輸入的檢查
// ====================================================================

void APlayerCharacter::Move(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫，直接返回，不處理移動輸入
    if (bIsPlayingEntranceAnimation) return;

    FVector2D MovementVector = Value.Get<FVector2D>(); // 取得輸入的 2D 向量

    if (Controller != nullptr)
    {
        // 取得控制器的旋轉 (特別是 Yaw 軸)，以便根據攝影機方向推導移動方向
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, ControlRotation.Yaw, 0); 

        // 根據攝影機的 Yaw 軸取得前向和右向向量
        const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(YawRotation);
        const FVector RightDirection = UKismetMathLibrary::GetRightVector(YawRotation);

        // 添加移動輸入，角色將會自動朝向這個移動方向 (因為 bOrientRotationToMovement 已啟用)
        AddMovementInput(ForwardDirection, MovementVector.Y); // 前進/後退 (Y 軸)
        AddMovementInput(RightDirection, MovementVector.X);  // 左/右移動 (X 軸)
    }
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫，直接返回，不處理視角輸入
    if (bIsPlayingEntranceAnimation) return;

    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 這些呼叫會旋轉控制器 (並透過 Spring Arm 旋轉攝影機)
        // 角色的身體 Yaw 軸將不會跟隨，因為 bUseControllerRotationYaw 設定為 false
        AddControllerYawInput(LookAxisVector.X); 
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void APlayerCharacter::Jump()
{
    // 如果正在播放入場動畫，直接返回，不處理跳躍輸入
    if (bIsPlayingEntranceAnimation) return;

    Super::Jump(); // 呼叫基底 Character 類的 Jump 函數
}

void APlayerCharacter::StopJumping()
{
    // 如果正在播放入場動畫，直接返回，不處理停止跳躍輸入
    if (bIsPlayingEntranceAnimation) return;

    Super::StopJumping(); // 呼叫基底 Character 類的 StopJumping 函數
}

// ====================================================================
// >>> 入場動畫實作 <<<
// ====================================================================

void APlayerCharacter::PlayEntranceAnimation()
{
    if (EntranceMontage) // 檢查是否有設定入場動畫 Montage
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 取得角色的動畫實例
        if (AnimInstance)
        {
            // 播放蒙太奇，速度為 1.0f (正常速度)
            float Duration = AnimInstance->Montage_Play(EntranceMontage, 1.0f); 
            
            if (Duration > 0.0f) // 如果蒙太奇成功播放 (持續時間大於 0)
            {
                bIsPlayingEntranceAnimation = true; // 設定旗標為 true，表示正在播放入場動畫
                
                // 綁定到 Montage 結束事件作為安全網 (以防 Notify 失敗或蒙太奇被中斷)
                // 先移除現有的委託，以防止重複綁定 (如果 PlayEntranceAnimation 被再次呼叫)
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnMontageEnded); 
                AnimInstance->OnMontageEnded.AddDynamic(this, &APlayerCharacter::OnMontageEnded);
                
                // 禁用角色移動組件
                GetCharacterMovement()->DisableMovement();
                // 在動畫期間禁用碰撞，防止角色被推動或卡住
                GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
            }
            else
            {
                // 如果 Montage_Play 返回 0.0 (例如，蒙太奇無效或播放失敗)
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed for EntranceMontage. Enabling Player Input."));
                SetPlayerInputEnabled(true); // 如果蒙太奇無法播放，重新啟用輸入
            }
        }
        else // 如果 AnimInstance 為空
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimInstance is null for PlayerCharacter. Enabling Player Input."));
            SetPlayerInputEnabled(true); // 如果 AnimInstance 為空，重新啟用輸入
        }
    }
    else // 如果沒有設定入場動畫 Montage
    {
        UE_LOG(LogTemp, Warning, TEXT("EntranceMontage is not set on PlayerCharacter! Enabling Player Input."));
        SetPlayerInputEnabled(true); // 如果沒有指定蒙太奇，直接啟用輸入
    }
}

void APlayerCharacter::OnEntranceAnimationFinishedByNotify()
{
    // 此函數旨在由動畫藍圖透過 Anim Notify 呼叫。
    // 它標誌著玩家應該重新獲得控制權的精確時刻。
    if (bIsPlayingEntranceAnimation) // 只有當我們處於入場動畫狀態時才執行
    {
        UE_LOG(LogTemp, Log, TEXT("Entrance Animation Finished by Anim Notify! Enabling Player Input."));
        bIsPlayingEntranceAnimation = false; // 設定旗標為 false，表示入場動畫結束
        SetPlayerInputEnabled(true); // 啟用玩家輸入

        // 重新啟用角色移動
        GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 將移動模式設定回步行
        // 重新啟用碰撞
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // 在通過 Notify 成功完成後，移除 OnMontageEnded 委託，以避免多餘的調用
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnMontageEnded);
        }
    }
}

void APlayerCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 這是一個安全網。如果蒙太奇因任何原因結束 (完成或中斷)，
    // 並且玩家輸入尚未被 Notify 重新啟用，則強制重新啟用它。
    if (Montage == EntranceMontage && bIsPlayingEntranceAnimation) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Entrance Montage Ended (safety net triggered)! Forcing Input Enabled."));
        bIsPlayingEntranceAnimation = false;
        SetPlayerInputEnabled(true);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // 總是移除委託，以防止陳舊的綁定
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnMontageEnded);
        }
    }
}

// ====================================================================
// >>> 攻擊輸入處理 (整合 Combo 邏輯) <<<
// ====================================================================
void APlayerCharacter::Attack(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫，或者角色已經死亡，則不允許攻擊
    if (bIsPlayingEntranceAnimation || bIsDead) return; // bIsDead 是 CharacterBase 的屬性

    if (!bIsAttacking) // 如果當前不在攻擊狀態，則啟動第一次攻擊
    {
        CurrentAttackComboIndex = 0; // 從第一個 Combo 段開始
        PlayAttackComboSegment();
    }
    else if (bCanEnterNextCombo) // 如果正在攻擊中，並且處於可輸入下一段 Combo 的窗口
    {
        TryEnterNextCombo();
    }
    // else 如果 bIsAttacking 為 true 但 bCanEnterNextCombo 為 false，則忽略本次攻擊輸入（玩家輸入過早或過晚）
}

// ====================================================================
// >>> Combo 攻擊實作 <<<
// ====================================================================

void APlayerCharacter::PlayAttackComboSegment()
{
    // 確保有設定攻擊蒙太奇，並且當前索引有效
    if (AttackMontages.IsValidIndex(CurrentAttackComboIndex) && AttackMontages[CurrentAttackComboIndex])
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            UAnimMontage* CurrentMontage = AttackMontages[CurrentAttackComboIndex];
            float Duration = AnimInstance->Montage_Play(CurrentMontage, 1.0f); 
            
            if (Duration > 0.0f)
            {
                bIsAttacking = true;
                bCanEnterNextCombo = false; // 播放新攻擊段時，重置為不能輸入下一段，直到 Notify 觸發

                // 綁定到攻擊蒙太奇結束事件
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnAttackMontageEnded); 
                AnimInstance->OnMontageEnded.AddDynamic(this, &APlayerCharacter::OnAttackMontageEnded);
                
                GetCharacterMovement()->StopMovementImmediately(); 
                GetCharacterMovement()->DisableMovement(); // 攻擊期間禁用移動
                
                // 清除任何舊的 Combo Window Timer
                GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
                // 在動畫中設置 Anim Notify 來觸發 SetCanEnterNextCombo(true) 和 PerformNormalAttackHitCheck
                // 並在 Combo Window 結束後重置 Combo，如果沒有新的輸入。
                // 這個定時器時間需要根據你的動畫和 Anim Notify 的設置來調整。
                // 假設 Combo Window 在當前攻擊動畫的 0.5 秒後結束，如果沒有新的輸入就重置。
                // 這需要你在蒙太奇中添加一個 AnimNotify 來精確控制 bCanEnterNextCombo
                // 如果沒有精確的 AnimNotify，這裡設置的 ComboWindowTimerHandle 將作為一個安全網。
                // 通常會在攻擊動畫的特定幀後設置這個定時器
                GetWorldTimerManager().SetTimer(ComboWindowTimerHandle, this, &APlayerCharacter::OnComboWindowEnd, Duration * 0.75f, false);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed for AttackMontage index %d."), CurrentAttackComboIndex);
                ResetCombo(); // 播放失敗，重置 Combo
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
        ResetCombo(); // 蒙太奇無效，重置 Combo
    }
}

void APlayerCharacter::TryEnterNextCombo()
{
    // 如果可以進入下一段 Combo，並且還沒達到 Combo 最大段數
    if (bCanEnterNextCombo && (CurrentAttackComboIndex + 1) < AttackMontages.Num())
    {
        CurrentAttackComboIndex++; // 進入下一段
        PlayAttackComboSegment(); // 播放下一段攻擊
        GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle); // 清除舊的 Combo Window Timer
        UE_LOG(LogTemp, Log, TEXT("Entered next combo segment: %d"), CurrentAttackComboIndex);
    }
    else
    {
        // 不能進入下一段 Combo（例如，達到最大段數或時間窗已過），則重置 Combo
        UE_LOG(LogTemp, Warning, TEXT("Cannot enter next combo. Resetting combo."));
        ResetCombo();
    }
}

void APlayerCharacter::ResetCombo()
{
    UE_LOG(LogTemp, Log, TEXT("Resetting Attack Combo."));
    CurrentAttackComboIndex = 0;
    bIsAttacking = false;
    bCanEnterNextCombo = false;
    GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle); // 清除任何剩餘的 Combo Window Timer
    GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 恢復移動模式
    // 注意：這裡假設在攻擊期間禁用移動，恢復時將其設為步行模式。
    // 如果你在攻擊期間禁用了碰撞，也要在這裡重新啟用。
}

void APlayerCharacter::OnComboWindowEnd()
{
    // 這個函數由 ComboWindowTimerHandle 調用
    // 如果定時器結束時 bIsAttacking 仍然為 true 且 bCanEnterNextCombo 沒有被新的輸入設置為 true，
    // 說明玩家在 Combo Window 內沒有輸入下一段，此時應重置 Combo。
    if (bIsAttacking && !bCanEnterNextCombo) // 確保仍處於攻擊狀態，且沒有成功進入下一段
    {
        UE_LOG(LogTemp, Log, TEXT("Combo Window Ended without next input. Resetting combo."));
        ResetCombo();
    }
}

void APlayerCharacter::SetCanEnterNextCombo(bool bCan)
{
    bCanEnterNextCombo = bCan;
    if (bCan)
    {
        UE_LOG(LogTemp, Log, TEXT("Can enter next combo now."));
        // 如果這個函數由 Anim Notify 觸發，可能需要在這裡清除舊的 OnComboWindowEnd 定時器，
        // 因為新的 Combo Window 會在 TryEnterNextCombo 中設置。
        // 但由於 TryEnterNextCombo 會立即清除，這裡不清除也沒關係。
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Can Enter Next Combo!"));
    }
}

void APlayerCharacter::PerformNormalAttackHitCheck()
{
    // 這個函數由 Anim Notify 呼叫
    UE_LOG(LogTemp, Log, TEXT("Performing Normal Attack Hit Check for Combo Segment: %d"), CurrentAttackComboIndex);

    FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("weapon_l")); // 或其他合適的 Socket
    FVector EndLocation = StartLocation + GetActorForwardVector() * 150.0f; 
    TArray<FHitResult> HitResults;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
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
        TArray<AActor*>(), 
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
                // 如果碰撞到的是自己，則略過
                // TODO: 這裡可以加入更精確的敵人判斷
                if (HitActor != this)
                {
                    UE_LOG(LogTemp, Log, TEXT("攻擊命中: %s"), *HitActor->GetName());
                    FDamageEvent DamageEvent;
                    HitActor->TakeDamage(25.0f, DamageEvent, GetController(), this);
                }
            }
        }
    }
}

void APlayerCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 如果結束的蒙太奇是我們的攻擊蒙太奇數組中的一個
    // 並且我們確實處於攻擊狀態 (bIsAttacking)
    if (AttackMontages.Contains(Montage) && bIsAttacking)
    {
        UE_LOG(LogTemp, Log, TEXT("Attack Montage Ended (Index: %d, Interrupted: %s)."), CurrentAttackComboIndex, bInterrupted ? TEXT("True") : TEXT("False"));
        
        // 無論蒙太奇是正常結束還是被中斷，都嘗試重置 Combo
        // ResetCombo() 會處理所有狀態的重置
        ResetCombo();

        // 總是移除委託，以防止多次綁定
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnAttackMontageEnded);
        }
    }
}

// ====================================================================
// >>> 輔助函數實作 <<<
// ====================================================================

void APlayerCharacter::SetPlayerInputEnabled(bool bEnabled)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        if (bEnabled)
        {
            PlayerController->EnableInput(PlayerController); // 啟用玩家控制器輸入
            UE_LOG(LogTemp, Log, TEXT("玩家輸入已啟用。")); // 輸出日誌訊息
        }
        else
        {
            PlayerController->DisableInput(PlayerController); // 禁用玩家控制器輸入
            UE_LOG(LogTemp, Log, TEXT("玩家輸入已禁用。")); // 輸出日誌訊息
        }
    }
}
