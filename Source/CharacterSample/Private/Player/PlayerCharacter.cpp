// PlayerCharacter.cpp

#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h" // 用於 Sweep Trace 函數 (如 UKismetSystemLibrary::SphereTraceMultiByChannel)
#include "Engine/DamageEvents.h" // 用於 FDamageEvent

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

// 攻擊輸入處理
void APlayerCharacter::Attack(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫或已經在攻擊中，則不允許再次攻擊
    if (bIsPlayingEntranceAnimation || bIsAttacking) return;

    // 呼叫播放普通攻擊動畫的函數
    PlayNormalAttack();
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
// >>> **新增：普通攻擊實作** <<<
// ====================================================================

void APlayerCharacter::PlayNormalAttack()
{
    if (NormalAttackMontage) // 檢查是否有設定普通攻擊 Montage
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 取得動畫實例
        if (AnimInstance)
        {
            // 播放普通攻擊蒙太奇
            float Duration = AnimInstance->Montage_Play(NormalAttackMontage, 1.0f); 
            
            if (Duration > 0.0f) // 如果成功播放
            {
                bIsAttacking = true; // 設定攻擊狀態為 true

                // 綁定到普通攻擊 Montage 結束事件作為安全網
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnNormalAttackMontageEnded); 
                AnimInstance->OnMontageEnded.AddDynamic(this, &APlayerCharacter::OnNormalAttackMontageEnded);
                
                // 在攻擊期間禁用移動（或你可以設定一個減速）
                GetCharacterMovement()->StopMovementImmediately(); // 立即停止移動
                GetCharacterMovement()->DisableMovement(); // 禁用移動輸入影響
                
                // 這裡暫時不禁用碰撞，因為攻擊需要接觸敵人
                // 如果你的攻擊動畫需要 Root Motion，則要確保 CharacterMovementComponent 的 Root Motion 設定正確
            }
            else
            {
                // 如果 Montage_Play 失敗，設定攻擊狀態為 false
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed for NormalAttackMontage."));
                bIsAttacking = false;
                GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 恢復移動模式
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimInstance is null for PlayerCharacter during attack."));
            bIsAttacking = false;
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NormalAttackMontage is not set on PlayerCharacter!"));
        bIsAttacking = false;
    }
}

void APlayerCharacter::PerformNormalAttackHitCheck()
{
    // 這個函數將由 Anim Notify 呼叫
    UE_LOG(LogTemp, Log, TEXT("Performing Normal Attack Hit Check!"));

    // 攻擊判定參數設定
    FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("weapon_l")); // 假設攻擊範圍從角色右手開始
    // 如果沒有 hand_r 這個骨骼或 Socket，請替換為其他合適的 Socket 名稱，
    // 或使用 GetActorLocation() + GetActorForwardVector() * Offset 來決定起始點。

    // 延伸攻擊範圍，這裡向角色前方延伸 150 單位
    FVector EndLocation = StartLocation + GetActorForwardVector() * 150.0f; 

    // 用於儲存碰撞結果
    TArray<FHitResult> HitResults;

    // 碰撞查詢參數
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 忽略角色自己，避免自體碰撞
    Params.bTraceComplex = true; // 進行精確碰撞，而不是簡單體積

    // 碰撞對象類型：只掃描 Pawn 和 PhysicsBody (通常敵人是 Pawn，環境可破壞物可能是 PhysicsBody)
    // 這些通道需要你在 Project Settings -> Collision 中設定。
    // 暫時可以先用 Default 的 WorldDynamic 或 ECC_Pawn
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 假設敵人是 Pawn 類型
    // ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody)); // 如果有可破壞的物理物體

    // 掃描形狀：球體掃描 (或 BoxTrace, CapsuleTrace)
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(70.0f); // 掃描半徑 70 單位

    // 執行多重球體掃描
    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        StartLocation,
        EndLocation,
        SphereShape.GetSphereRadius(), // 球體半徑
        ObjectTypes,
        false, // 不畫出除錯線
        TArray<AActor*>(), // 額外忽略的 Actor 列表 (Params中已設定忽略自己)
        EDrawDebugTrace::ForDuration, // 除錯線持續顯示一段時間
        HitResults,
        true, // 是否忽略自身
        FLinearColor::Red, // 除錯線命中顏色
        FLinearColor::Green, // 除錯線未命中顏色
        5.0f // 除錯線顯示時間
    );

    if (bHit)
    {
        for (const FHitResult& Hit : HitResults)
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                // 檢查是否是敵人 (或可受傷的 Actor)
                // 這裡可以加入更精確的判斷，例如：
                // if (HitActor->ActorHasTag(TEXT("Enemy")))
                // 或 if (Cast<AEnemyCharacter>(HitActor))
                
                UE_LOG(LogTemp, Log, TEXT("攻擊命中: %s"), *HitActor->GetName());

                // 造成傷害：使用 Unreal Engine 內建的 TakeDamage 函數
                // 這是最基礎的傷害處理，未來會用 HealthComponent 來取代
                FDamageEvent DamageEvent; // 簡單的傷害事件
                HitActor->TakeDamage(25.0f, DamageEvent, GetController(), this); // 造成 25 點傷害
            }
        }
    }
}

void APlayerCharacter::OnNormalAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 如果結束的蒙太奇是我們的普通攻擊蒙太奇
    if (Montage == NormalAttackMontage)
    {
        UE_LOG(LogTemp, Log, TEXT("Normal Attack Montage Ended."));
        bIsAttacking = false; // 重設攻擊狀態為 false

        // 重新啟用移動
        GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 將移動模式設定回步行

        // 移除委託，防止多次綁定
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnNormalAttackMontageEnded);
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
