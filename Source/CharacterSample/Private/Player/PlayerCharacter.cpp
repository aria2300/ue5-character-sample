// PlayerCharacter.cpp

#include "Player/PlayerCharacter.h" // 包含此角色的頭檔
#include "GameFramework/CharacterMovementComponent.h" // 用於控制角色移動
#include "Components/CapsuleComponent.h" // 用於角色的碰撞體
#include "Components/SkeletalMeshComponent.h" // 用於角色的網格模型
#include "Kismet/KismetSystemLibrary.h" // 用於藍圖輔助函數，如 Sweep Trace
#include "Engine/DamageEvents.h" // 用於處理傷害事件
#include "Blueprint/UserWidget.h" // 用於 UI Widget 的創建
#include "GameFramework/SpringArmComponent.h" // 包含 SpringArm 組件的頭檔 (雖然在 .h 中聲明，但這裡可能也需要)
#include "Camera/CameraComponent.h" // 包含攝影機組件的頭檔 (同上)
#include "InputMappingContext.h" // 包含 Enhanced Input Mapping Context 的頭檔
#include "EnhancedInputSubsystems.h" // 包含 Enhanced Input Subsystem 的頭檔
#include "EnhancedInputComponent.h" // 包含 Enhanced Input Component 的頭檔
#include "Kismet/KismetMathLibrary.h" // 用於數學輔助函數，如 GetForwardVector
#include "Components/CombatComponent.h" // <-- 新增：包含 CombatComponent 的頭檔
#include "Engine/Engine.h" // 用於 GEngine->AddOnScreenDebugMessage

// ====================================================================
// >>> 構造函數：APlayerCharacter::APlayerCharacter() <<<
// 設定角色的預設值與組件行為
// ====================================================================
APlayerCharacter::APlayerCharacter()
{
    // 啟用 Tick 函式：設定此角色每幀都會呼叫 Tick() 函式，用於持續更新邏輯。
    // 如果不需要每幀更新，可以將其關閉以優化效能。
    PrimaryActorTick.bCanEverTick = true; 

    // ====================================================================
    // >>> 角色移動設定，實現「無雙類」轉向 <<<
    // 透過禁用控制器對角色本體的旋轉控制，並啟用角色面向移動方向，來達到流暢轉向。
    // ====================================================================

    // 禁用角色身體的 Yaw 軸旋轉跟隨控制器（攝影機）的 Yaw 軸旋轉。
    // 這表示當玩家移動滑鼠轉動攝影機時，角色的身體不會立即跟著轉動。
    bUseControllerRotationYaw = false; 
    bUseControllerRotationPitch = false; // 禁用 Pitch 軸跟隨
    bUseControllerRotationRoll = false;  // 禁用 Roll 軸跟隨

    // 取得角色移動組件的引用。
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        // 啟用：讓角色自動面向其移動的方向。這是實現無雙式轉向的關鍵。
        // 只要有移動輸入，角色就會平滑地旋轉以面對移動的方向。
        MovementComp->bOrientRotationToMovement = true; 
        
        // 設定角色的轉向速度。這個值通常設得非常快 (例如 3600 度/秒)，
        // 使其轉向看起來幾乎是瞬時的，符合無雙遊戲的快速反應。
        MovementComp->RotationRate = FRotator(0.0f, 3600.0f, 0.0f); 

        // 設定預設移動速度、跳躍初速度和空中可控性。
        MovementComp->MaxWalkSpeed = 600.0f;
        MovementComp->JumpZVelocity = 600.f;
        MovementComp->AirControl = 0.2f; // 值越高，空中轉向和控制越靈敏
    }

    // --- 在這裡創建和初始化 CombatComponent ---
    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
    // 可以為組件設定一些預設值，但通常它的配置會在藍圖中進行，或者在 CombatComponent 自己的構造函數中。
    // CombatComponent->SetIsReplicated(true); // 如果需要網路複製，可以在這裡設置

    // 設定攝影機臂 (SpringArmComponent) 和攝影機 (CameraComponent)。
    // 假設你的角色藍圖中已經有這些組件。
    USpringArmComponent* CameraBoom = FindComponentByClass<USpringArmComponent>();
    if (CameraBoom)
    {
        // 攝影機臂將會跟隨控制器的 Yaw 軸旋轉。
        // 這允許攝影機獨立於角色身體旋轉，玩家可以自由環顧四周。
        CameraBoom->bUsePawnControlRotation = true; 
        CameraBoom->bInheritPitch = true; // 繼承控制器 Pitch 軸旋轉
        CameraBoom->bInheritRoll = true;  // 繼承控制器 Roll 軸旋轉
        CameraBoom->bDoCollisionTest = true; // 啟用攝影機碰撞檢測，防止攝影機穿牆或卡住
    }

    UCameraComponent* FollowCamera = FindComponentByClass<UCameraComponent>();
    if (FollowCamera)
    {
        // 攝影機本身不直接旋轉，它會自動跟隨其父組件 (Spring Arm) 的旋轉。
        FollowCamera->bUsePawnControlRotation = false; 
    }

    // 初始化入場動畫的旗標，預設為未播放。
    bIsPlayingEntranceAnimation = false;

    // 預設啟用角色的膠囊碰撞體進行查詢和物理模擬。
    // 這使得角色能夠與世界中的其他物體進行碰撞檢測。
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); 
}

// ====================================================================
// >>> BeginPlay() 函式：在遊戲開始時或角色被生成時呼叫 <<<
// 主要用於初始化遊戲狀態、UI 和輸入系統。
// ====================================================================
void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay(); // 呼叫父類 (ACharacter) 的 BeginPlay 函式

    // --- 輸入系統設定 ---
    // 取得玩家控制器。
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (PlayerController)
    {
        // 取得 Enhanced Input Local Player 子系統。這是處理新版輸入的關鍵組件。
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
        if (Subsystem)
        {
            // 添加預設的輸入映射上下文。
            // `DefaultMappingContext` 是一個 UInputMappingContext* 變數，你需要在藍圖中為它賦值。
            Subsystem->AddMappingContext(DefaultMappingContext, 0); // 優先級設為 0
        }
    }

    // ====================================================================
    // >>> 遊戲初始狀態：禁用輸入並播放入場動畫 <<<
    // 這是遊戲開場時的標準流程，確保玩家在動畫播放時無法控制角色。
    // ====================================================================
    SetPlayerInputEnabled(false); // 遊戲開始時，先禁用玩家輸入
    PlayEntranceAnimation();      // 播放角色的入場動畫
}

// ====================================================================
// >>> Tick() 函式：每幀呼叫，更新角色的動畫狀態變數 <<<
// 這些變數通常會被用於動畫藍圖 (Anim Blueprint) 中，來控制動畫的混合與播放。
// ====================================================================
void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime); // 呼叫父類 (ACharacter) 的 Tick 函式

    // 更新角色速度、是否下落以及移動方向等動畫相關變數。
    if (GetCharacterMovement())
    {
        CurrentSpeed = GetVelocity().Size(); // 取得當前水平速度
        bIsFalling = GetCharacterMovement()->IsFalling(); // 判斷角色是否正在下落 (跳躍或掉落)

        // 計算移動方向的角度。由於 bOrientRotationToMovement 為 true，角色總是面向移動方向，
        // 因此 MovementDirection 在角色的局部空間中通常會接近 0 (表示向前)。
        // 這個變數在動畫藍圖中仍然有用，例如用於混合側身移動或後退動畫。
        if (CurrentSpeed > KINDA_SMALL_NUMBER) // 如果有明顯的移動速度 (避免浮點數精度問題)
        {
            FVector WorldVelocity = GetVelocity();
            WorldVelocity.Z = 0.f; // 忽略垂直速度，只考慮水平方向的速度

            if (WorldVelocity.SizeSquared() > KINDA_SMALL_NUMBER) // 確保速度向量不是零向量
            {
                WorldVelocity.Normalize(); // 正規化速度向量，使其長度為 1

                // 取得角色的當前 Yaw 旋轉 (只考慮水平旋轉)
                const FRotator CharacterYawRotation(0, GetActorRotation().Yaw, 0);
                // 將世界速度向量轉換為相對於角色自身局部空間的速度向量
                FVector LocalVelocity = CharacterYawRotation.UnrotateVector(WorldVelocity);

                // 計算移動方向的角度 (使用 Atan2(Y, X) 得到弧度，再轉換為角度)。
                // 0 度表示向前，90 度表示向右，-90 度表示向左，180 度表示向後。
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

// ====================================================================
// >>> SetupPlayerInputComponent() 函式：將功能綁定到輸入系統 <<<
// 設定玩家按鍵與函式的對應關係。
// ====================================================================
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent); // 呼叫父類的 SetupPlayerInputComponent

    // 將基礎的 UInputComponent 轉換為 UEnhancedInputComponent，以便使用 Enhanced Input System。
    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInputComponent)
    {
        // 綁定移動輸入動作 (MoveAction)，當輸入被觸發時，呼叫 Move 函式。
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        }
        
        // 綁定跳躍輸入動作 (JumpAction)。
        // "Started" 事件：按下跳躍鍵時呼叫 Jump。
        // "Completed" 事件：鬆開跳躍鍵時呼叫 StopJumping。
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJumping);
        }

        // 綁定視角輸入動作 (LookAction)，用於控制攝影機。
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        }

        // 綁定攻擊輸入到 AttackAction & CombatComponent。
        // "Started" 事件：按下攻擊鍵時呼叫 Attack。
        // ETriggerEvent::Started 確保只有在按鍵的初始按下瞬間觸發一次，避免長按持續觸發。
        if (AttackAction && CombatComponent) // 確保 CombatComponent 存在
        {
            EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, CombatComponent, &UCombatComponent::Attack);
        }
    }
}

// ====================================================================
// >>> 輸入處理實作 <<<
// 包含在播放入場動畫期間禁用輸入的檢查，確保玩家在特定狀態下無法操作。
// ====================================================================

// 處理移動輸入。
void APlayerCharacter::Move(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫，直接返回，不處理移動輸入。
    if (bIsPlayingEntranceAnimation) return;

    FVector2D MovementVector = Value.Get<FVector2D>(); // 取得輸入的 2D 向量 (X 和 Y 軸)

    if (Controller != nullptr) // 確保控制器存在
    {
        // 取得控制器的旋轉 (特別是 Yaw 軸)，以便根據攝影機方向推導移動方向。
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, ControlRotation.Yaw, 0); 

        // 根據攝影機的 Yaw 軸取得前向 (Forward) 和右向 (Right) 向量。
        const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(YawRotation);
        const FVector RightDirection = UKismetMathLibrary::GetRightVector(YawRotation);

        // 添加移動輸入。由於 MovementComp->bOrientRotationToMovement 已啟用，
        // 角色會自動朝向這個計算出的移動方向旋轉。
        AddMovementInput(ForwardDirection, MovementVector.Y); // 前進/後退 (Y 軸對應控制器前向)
        AddMovementInput(RightDirection, MovementVector.X);  // 左/右移動 (X 軸對應控制器右向)
    }
}

// 處理視角輸入。
void APlayerCharacter::Look(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫，直接返回，不處理視角輸入。
    if (bIsPlayingEntranceAnimation) return;

    FVector2D LookAxisVector = Value.Get<FVector2D>(); // 取得視角輸入的 2D 向量 (X 軸為 Yaw，Y 軸為 Pitch)

    if (Controller != nullptr) // 確保控制器存在
    {
        // 這些呼叫會旋轉控制器 (並透過 Spring Arm 旋轉攝影機)。
        // 角色的身體 Yaw 軸將不會跟隨，因為 bUseControllerRotationYaw 設定為 false。
        AddControllerYawInput(LookAxisVector.X); // 水平視角旋轉
        AddControllerPitchInput(LookAxisVector.Y); // 垂直視角旋轉
    }
}

// 處理跳躍輸入。
void APlayerCharacter::Jump()
{
    // 如果正在播放入場動畫，直接返回，不處理跳躍輸入。
    if (bIsPlayingEntranceAnimation) return;

    Super::Jump(); // 呼叫基底 Character 類的 Jump 函式，執行預設跳躍行為
}

// 處理停止跳躍輸入（鬆開跳躍鍵時）。
void APlayerCharacter::StopJumping()
{
    // 如果正在播放入場動畫，直接返回，不處理停止跳躍輸入。
    if (bIsPlayingEntranceAnimation) return;

    Super::StopJumping(); // 呼叫基底 Character 類的 StopJumping 函式
}

// ====================================================================
// >>> 入場動畫實作 <<<
// 控制遊戲開始時的動畫播放與玩家控制權的啟用。
// ====================================================================

// 播放入場動畫。
void APlayerCharacter::PlayEntranceAnimation()
{
    if (EntranceMontage) // 檢查是否有設定入場動畫 Montage (UAnimMontage 變數，需在藍圖中指定)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 取得角色的動畫實例
        if (AnimInstance)
        {
            // 播放蒙太奇，速度為 1.0f (正常速度)。返回蒙太奇的實際持續時間。
            float Duration = AnimInstance->Montage_Play(EntranceMontage, 1.0f); 
            
            if (Duration > 0.0f) // 如果蒙太奇成功播放 (持續時間大於 0)
            {
                bIsPlayingEntranceAnimation = true; // 設定旗標為 true，表示正在播放入場動畫
                
                // 綁定到 Montage 結束事件作為安全網。
                // 先移除現有的委託，以防止重複綁定 (如果 PlayEntranceAnimation 被再次呼叫)。
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnMontageEnded); 
                // 添加動態委託，當蒙太奇結束時，呼叫 OnMontageEnded 函式。
                AnimInstance->OnMontageEnded.AddDynamic(this, &APlayerCharacter::OnMontageEnded);
                
                // 在動畫期間禁用角色移動組件，防止玩家移動。
                GetCharacterMovement()->DisableMovement();
                // 在動畫期間禁用膠囊碰撞體的碰撞，防止角色被推動或卡住。
                GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
            }
            else
            {
                // 如果 Montage_Play 返回 0.0 (例如，蒙太奇無效或播放失敗)
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed for EntranceMontage. Enabling Player Input."));
                SetPlayerInputEnabled(true); // 如果蒙太奇無法播放，立即重新啟用輸入，避免卡住
            }
        }
        else // 如果 AnimInstance 為空 (表示網格或動畫實例有問題)
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimInstance is null for PlayerCharacter. Enabling Player Input."));
            SetPlayerInputEnabled(true); // 如果 AnimInstance 為空，立即重新啟用輸入
        }
    }
    else // 如果沒有設定入場動畫 Montage
    {
        UE_LOG(LogTemp, Warning, TEXT("EntranceMontage is not set on PlayerCharacter! Enabling Player Input."));
        SetPlayerInputEnabled(true); // 如果沒有指定蒙太奇，直接啟用輸入
    }
}

// 由動畫藍圖透過 Anim Notify 呼叫。
// 標誌著玩家應該重新獲得控制權的精確時刻。這是預期的入場動畫結束方式。
void APlayerCharacter::OnEntranceAnimationFinishedByNotify()
{
    // 只有當我們處於入場動畫狀態時才執行，避免重複執行或在錯誤時機執行。
    if (bIsPlayingEntranceAnimation) 
    {
        UE_LOG(LogTemp, Log, TEXT("Entrance Animation Finished by Anim Notify! Enabling Player Input."));
        bIsPlayingEntranceAnimation = false; // 設定旗標為 false，表示入場動畫結束
        SetPlayerInputEnabled(true); // 啟用玩家輸入

        // 重新啟用角色移動，將移動模式設定回步行。
        GetCharacterMovement()->SetMovementMode(MOVE_Walking); 
        // 重新啟用膠囊碰撞體，使其能夠再次與世界互動。
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // 在通過 Notify 成功完成後，移除 OnMontageEnded 委託，以避免多餘的調用。
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnMontageEnded);
        }
    }
}

// 進場動畫結束的保險機制。
// 這個函式由 `AnimInstance->OnMontageEnded` 委託調用。
// 如果蒙太奇因任何原因結束 (完成或中斷)，並且玩家輸入尚未被 Notify 重新啟用，則強制重新啟用它。
void APlayerCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 檢查結束的蒙太奇是否是我們的入場蒙太奇，並且我們仍然處於「正在播放入場動畫」的狀態。
    if (Montage == EntranceMontage && bIsPlayingEntranceAnimation) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Entrance Montage Ended (safety net triggered)! Forcing Input Enabled."));
        bIsPlayingEntranceAnimation = false; // 設定旗標為 false
        SetPlayerInputEnabled(true); // 強制啟用玩家輸入
        GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 恢復移動
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 恢復碰撞

        // 總是移除委託，以防止陳舊的綁定或重複調用。
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnMontageEnded);
        }
    }
}

// ====================================================================
// >>> 輔助函式實作 <<<
// 通用功能，如啟用/禁用玩家輸入。
// ====================================================================

// 啟用或禁用玩家輸入的輔助函式。
void APlayerCharacter::SetPlayerInputEnabled(bool bEnabled)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController()); // 取得玩家控制器
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
