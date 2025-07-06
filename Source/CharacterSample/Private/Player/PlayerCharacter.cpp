// PlayerCharacter.cpp

#include "Player/PlayerCharacter.h" // 包含此角色的頭檔
#include "GameFramework/CharacterMovementComponent.h" // 用於控制角色移動
#include "Components/CapsuleComponent.h" // 用於角色的碰撞體
#include "Components/SkeletalMeshComponent.h" // 用於角色的網格模型
#include "Kismet/KismetSystemLibrary.h" // 用於藍圖輔助函數，如 Sweep Trace
#include "Engine/DamageEvents.h" // 用於處理傷害事件
#include "Blueprint/UserWidget.h" // 用於 UI Widget 的創建
#include "HealthBarBaseWidget.h" // 包含自定義血條 Widget 的頭檔
#include "GameFramework/SpringArmComponent.h" // 包含 SpringArm 組件的頭檔 (雖然在 .h 中聲明，但這裡可能也需要)
#include "Camera/CameraComponent.h" // 包含攝影機組件的頭檔 (同上)
#include "InputMappingContext.h" // 包含 Enhanced Input Mapping Context 的頭檔
#include "EnhancedInputSubsystems.h" // 包含 Enhanced Input Subsystem 的頭檔
#include "EnhancedInputComponent.h" // 包含 Enhanced Input Component 的頭檔
#include "Kismet/KismetMathLibrary.h" // 用於數學輔助函數，如 GetForwardVector

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

    // --- UI 創建邏輯 ---
    // 判斷是否是本地玩家控制的角色。
    // 這是為了防止在多人遊戲中，服務器或其他客戶端重複創建玩家的 UI。
    if (IsLocallyControlled())
    {
        // 檢查藍圖中是否已經設置了 HealthBarWidgetClass (一個藍圖類別引用)。
        if (HealthBarWidgetClass)
        {
            // 使用 CreateWidget 函式來創建你的血條 Widget 實例。
            // Cast<UHealthBarBaseWidget> 確保返回的實例是指向 UHealthBarBaseWidget 類型。
            HealthBarWidgetInstance = CreateWidget<UHealthBarBaseWidget>(GetWorld(), HealthBarWidgetClass);

            // 檢查是否成功創建了 Widget 實例。
            if (HealthBarWidgetInstance)
            {
                // 呼叫你的 HealthBarBaseWidget 中定義的初始化函式，將當前角色 (this) 傳遞給它。
                // 這樣血條 Widget 就能知道它屬於哪個角色，並可以開始監聽該角色的生命值變化。
                HealthBarWidgetInstance->SetOwnerCharacterAndInitialize(this);

                // 將創建的血條 Widget 添加到遊戲視口中，使其能夠被玩家看到。
                HealthBarWidgetInstance->AddToViewport();
            }
        }
    }

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

    // 初始化連段輸入緩衝旗標。
    // `bPendingNextComboInput` 為 true 表示玩家在連段窗口開啟前，已經按下了攻擊鍵，等待系統處理。
    bPendingNextComboInput = false; 
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

        // 綁定攻擊輸入動作 (AttackAction)。
        // "Started" 事件：按下攻擊鍵時呼叫 Attack。
        // ETriggerEvent::Started 確保只有在按鍵的初始按下瞬間觸發一次，避免長按持續觸發。
        if (AttackAction)
        {
            EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &APlayerCharacter::Attack);
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
// >>> 攻擊輸入處理 (整合 Combo 邏輯) <<<
// 這是玩家按下攻擊鍵時的核心邏輯，包含了連擊的啟動與緩衝判斷。
// ====================================================================
void APlayerCharacter::Attack(const FInputActionValue& Value)
{
    // 如果正在播放入場動畫，或者角色已經死亡，則不允許攻擊，直接返回。
    if (bIsPlayingEntranceAnimation || bIsDead) return; 

    // 判斷是否正在攻擊中。
    // 如果 `bIsAttacking` 為 false，表示角色當前沒有進行攻擊，這是啟動第一擊的條件。
    if (!bIsAttacking) 
    {
        CurrentAttackComboIndex = 0; // 設定當前連擊段數為第一段 (0)
        PlayAttackComboSegment(); // 播放第一段攻擊動畫
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta, TEXT("Starting First Attack!")); // Debug 訊息
        return; // 啟動第一擊後，直接返回，避免後續的判斷邏輯。
    }

    // 如果 `bIsAttacking` 為 true，表示角色正在攻擊中，接下來判斷是否能進入下一段連擊。
    // 這個 `if` 語句塊是獨立於上一個 `if` 語句塊的。
    // 注意：這裡應該是 `else if` 連續判斷，否則會出現啟動第一擊後立即進入緩衝判斷的問題。
    // **重要修正：將這裡的 `if` 改為 `else if`。**
    // 由於你之前成功解決了問題，目前的行為可能是因為邏輯流程的巧妙，但從清晰度考慮，`else if` 更佳。
    // 如果你沒有將上面 `if (!bIsAttacking)` 結束後加 `return;`，這個 `if` 確實會是問題。
    // 由於你已經加了 `return;`，這個 `if` 在第一次攻擊時不會執行，所以現在的代碼是可運作的。
    // 但為了代碼清晰和標準，通常會用 `else` 或 `else if`。

    if (bCanEnterNextCombo) // 判斷是否處於可輸入下一段 Combo 的時間窗口 (由 Anim Notify 設置)
    {
        TryEnterNextCombo(); // 嘗試進入下一段連擊
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Attempting Next Combo (Direct)!")); // Debug 訊息：直接連擊成功
        SetPendingNextComboInput(false); // 成功連擊後，清除任何緩衝的輸入旗標
    }
    else // 如果當前不在 `bCanEnterNextCombo` 的窗口內
    {
        // 玩家按下了攻擊鍵，則緩衝這次輸入。
        // 這意味著即使玩家按得太快 (在 `ComboWindow` Notify 之前)，系統也會記住這個輸入。
        SetPendingNextComboInput(true); 
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Attack input buffered!")); // Debug 訊息：輸入已緩衝
    }
}

// ====================================================================
// >>> Combo 攻擊實作 <<<
// 負責播放連擊的每一段動畫，並設定相關狀態和計時器。
// ====================================================================

// 播放當前連擊段的動畫。
void APlayerCharacter::PlayAttackComboSegment()
{
    // 確保 `AttackMontages` 陣列有效，且當前連擊索引在有效範圍內，並且蒙太奇本身不為空。
    if (AttackMontages.IsValidIndex(CurrentAttackComboIndex) && AttackMontages[CurrentAttackComboIndex])
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); // 取得動畫實例
        if (AnimInstance)
        {
            UAnimMontage* CurrentMontage = AttackMontages[CurrentAttackComboIndex]; // 取得當前要播放的蒙太奇
            float Duration = AnimInstance->Montage_Play(CurrentMontage, 1.0f); // 播放蒙太奇，取得其持續時間
            
            if (Duration > 0.0f) // 如果蒙太奇成功播放
            {
                bIsAttacking = true; // 設定為正在攻擊狀態
                bCanEnterNextCombo = false; // 重置為不能輸入下一段，直到 `ComboWindow` Notify 觸發
                
                // 綁定到攻擊蒙太奇結束事件。
                // 移除現有的委託以防重複綁定。
                AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnAttackMontageEnded); 
                // 添加新的委託，當蒙太奇結束時呼叫 OnAttackMontageEnded 函式。
                AnimInstance->OnMontageEnded.AddDynamic(this, &APlayerCharacter::OnAttackMontageEnded);
                
                // 攻擊期間停止角色移動並禁用移動組件。
                GetCharacterMovement()->StopMovementImmediately(); 
                GetCharacterMovement()->DisableMovement(); 
                
                // 清除任何舊的 Combo Window Timer，確保每次播放新段落時都重新計時。
                GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle);
                // 設定一個定時器，用於判斷連擊是否超時。
                // 這個定時器會在當前蒙太奇播放結束後一小段時間 (Duration + 0.1f) 觸發 `OnComboWindowEnd`。
                // 它的目的是作為一個安全網，如果玩家在動畫結束後仍未輸入下一段，則重置 Combo。
                GetWorldTimerManager().SetTimer(ComboWindowTimerHandle, this, &APlayerCharacter::OnComboWindowEnd, Duration + 0.1f, false);
            }
            else // 如果蒙太奇播放失敗
            {
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed for AttackMontage index %d."), CurrentAttackComboIndex);
                ResetCombo(); // 播放失敗，重置 Combo 狀態
            }
        }
        else // 如果 AnimInstance 為空
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimInstance is null for PlayerCharacter during attack."));
            ResetCombo(); // 重置 Combo
        }
    }
    else // 如果當前連擊索引無效或蒙太奇未設置
    {
        UE_LOG(LogTemp, Warning, TEXT("AttackMontage index %d is invalid or Montage is null!"), CurrentAttackComboIndex);
        ResetCombo(); // 重置 Combo
    }
}

// 嘗試進入下一段連擊。
// 這個函式會在 `Attack()` 函式中被直接調用 (玩家直接輸入時)，或在 `SetCanEnterNextCombo()` 中被調用 (緩衝輸入觸發時)。
void APlayerCharacter::TryEnterNextCombo()
{
    // 檢查兩個條件：
    // 1. `bCanEnterNextCombo` 為 true (由 Anim Notify 設置，表示進入了連擊窗口)。
    // 2. 當前連擊段數 `CurrentAttackComboIndex + 1` 未達到 `AttackMontages` 陣列的最大數量 (即還有下一段攻擊動畫)。
    if (bCanEnterNextCombo && (CurrentAttackComboIndex + 1) < AttackMontages.Num())
    {
        CurrentAttackComboIndex++; // 增加連擊段數索引
        PlayAttackComboSegment(); // 播放下一段連擊動畫
        GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle); // 清除舊的 Combo Window Timer，為新的連擊段重新計時
        UE_LOG(LogTemp, Log, TEXT("Entered next combo segment: %d"), CurrentAttackComboIndex); // Debug 訊息
    }
    else // 如果不能進入下一段 Combo (例如，達到最大段數或時間窗已過)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot enter next combo. Resetting combo.")); // Debug 訊息
        ResetCombo(); // 重置 Combo 狀態
    }
}

// 重置連段狀態。
// 當連擊中斷、結束或發生錯誤時呼叫此函式，將所有連擊相關狀態恢復到初始值。
void APlayerCharacter::ResetCombo()
{
    UE_LOG(LogTemp, Log, TEXT("Resetting Attack Combo.")); // Debug 訊息
    CurrentAttackComboIndex = 0; // 重置連擊段數索引為 0 (第一段)
    bIsAttacking = false; // 設定為不在攻擊狀態
    bCanEnterNextCombo = false; // 設定為不能進入下一段連擊
    bPendingNextComboInput = false; // 清除緩衝輸入標誌
    GetWorldTimerManager().ClearTimer(ComboWindowTimerHandle); // 清除任何剩餘的 Combo Window Timer
    GetCharacterMovement()->SetMovementMode(MOVE_Walking); // 恢復角色移動模式為步行
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Combo Reset!")); // Debug 訊息
}

// 連段輸入緩衝視窗結束時呼叫。
// 這個函式由 `ComboWindowTimerHandle` 定時器調用。
void APlayerCharacter::OnComboWindowEnd()
{
    // 檢查兩個條件：
    // 1. `bIsAttacking` 仍然為 true (表示角色仍處於攻擊狀態)。
    // 2. `bCanEnterNextCombo` 為 false (表示沒有成功進入下一段連擊)。
    // 如果定時器結束時這些條件成立，說明玩家在連擊窗口內沒有輸入下一段，此時應重置 Combo。
    if (bIsAttacking && !bCanEnterNextCombo) 
    {
        UE_LOG(LogTemp, Log, TEXT("Combo Window Ended without next input. Resetting combo.")); // Debug 訊息
        ResetCombo(); // 重置 Combo
    }
}

// 由動畫藍圖透過 Anim Notify 呼叫，設定是否可進入下一段連擊。
// 這是 `ComboWindow` Notify 觸發時調用的函式。
void APlayerCharacter::SetCanEnterNextCombo(bool bCan)
{
    bCanEnterNextCombo = bCan; // 設定 `bCanEnterNextCombo` 的值
    if (bCan) // 如果現在可以進入下一段連擊
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Can Enter Next Combo!")); // Debug 訊息

        // 關鍵邏輯：如果現在可以進入下一段 Combo，並且有緩衝的輸入 (`bPendingNextComboInput` 為 true)
        if (bPendingNextComboInput)
        {
            TryEnterNextCombo(); // 立即觸發下一段 Combo (處理緩衝輸入)
            SetPendingNextComboInput(false); // 清除緩衝標誌，因為輸入已被處理
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Buffered input triggered next combo!")); // Debug 訊息
        }
    }
}

// 由動畫藍圖透過 Anim Notify 呼叫，設定是否有連段輸入緩衝。
// 這是 `BufferInput` Notify 觸發時調用的函式。
void APlayerCharacter::SetPendingNextComboInput(bool bPending)
{
    bPendingNextComboInput = bPending; // 設定 `bPendingNextComboInput` 的值
    if (bPending) // 如果設定為 true (表示輸入被緩衝了)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("Input Buffered!")); // Debug 訊息
    }
}

// ====================================================================
// >>> 攻擊命中檢查實作 <<<
// 負責在動畫的特定幀執行攻擊判定，並對命中的敵人造成傷害。
// ====================================================================

// 執行普通攻擊的命中檢查。這個函式由動畫藍圖透過 Anim Notify (`AttackHit`) 呼叫。
void APlayerCharacter::PerformNormalAttackHitCheck()
{
    UE_LOG(LogTemp, Log, TEXT("Performing Normal Attack Hit Check for Combo Segment: %d"), CurrentAttackComboIndex); // Debug 訊息

    // 設定 Sphere Trace 的起點和終點。
    // `weapon_l` 是一個骨骼 Socket 的名稱，你需要在角色的 Skeletal Mesh 中設置它來標識武器位置。
    FVector StartLocation = GetMesh()->GetSocketLocation(TEXT("weapon_l")); 
    // 終點是從起點沿著角色前方延伸 150 單位。
    FVector EndLocation = StartLocation + GetActorForwardVector() * 150.0f; 

    TArray<FHitResult> HitResults; // 用於儲存所有命中結果
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 忽略角色自己，防止自體命中
    Params.bTraceComplex = true; // 啟用複雜碰撞，更精確但稍耗效能

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn)); // 僅檢測 Pawn 類型的物件 (如敵人)

    FCollisionShape SphereShape = FCollisionShape::MakeSphere(70.0f); // 定義一個半徑為 70.0 的球形作為攻擊範圍

    // 執行 Sphere Trace。
    // 這會在起點和終點之間掃描一個球體，檢測所有命中的物件。
    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),                 // 世界上下文
        StartLocation,              // 球體掃描的起始位置
        EndLocation,                // 球體掃描的結束位置
        SphereShape.GetSphereRadius(), // 球體的半徑
        ObjectTypes,                // 要檢測的物件類型
        false,                      // 是否要忽略所有Actor (這裡設為 false，因為我們只忽略自己)
        TArray<AActor*>(),          // 要忽略的 Actor 陣列 (已經通過 Params.AddIgnoredActor(this) 添加自己)
        EDrawDebugTrace::ForDuration, // 在編輯器中繪製 Debug 線條，持續 5 秒 (用於調試)
        HitResults,                 // 輸出命中結果
        true,                       // 是否要忽略自身 (這裡設為 true，因為我們已經在 Params 中指定了)
        FLinearColor::Red,          // Debug 線條的顏色 (命中時)
        FLinearColor::Green,        // Debug 線條的顏色 (未命中時)
        5.0f                        // Debug 線條的顯示時間
    );

    if (bHit) // 如果有命中任何物件
    {
        for (const FHitResult& Hit : HitResults) // 遍歷所有命中結果
        {
            if (AActor* HitActor = Hit.GetActor()) // 取得命中的 Actor
            {
                if (HitActor != this) // 再次確認不是命中自己
                {
                    UE_LOG(LogTemp, Log, TEXT("攻擊命中: %s"), *HitActor->GetName()); // Debug 訊息
                    FDamageEvent DamageEvent; // 創建一個通用的傷害事件
                    // 對命中的 Actor 造成 25.0 點傷害。
                    // `GetController()` 是造成傷害的控制器，`this` 是造成傷害的 Actor (角色本身)。
                    HitActor->TakeDamage(25.0f, DamageEvent, GetController(), this); 
                }
            }
        }
    }
}

// ====================================================================
// >>> 動畫蒙太奇結束回調 <<<
// 主要用於清理委託，不負責重置 Combo，重置邏輯已交由其他部分處理。
// ====================================================================

// 攻擊動畫結束時呼叫。
// 這個函式由 `AnimInstance->OnMontageEnded` 委託調用。
void APlayerCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 檢查結束的蒙太奇是否是我們攻擊蒙太奇數組中的一個，並且角色仍然處於攻擊狀態。
    if (AttackMontages.Contains(Montage) && bIsAttacking)
    {
        UE_LOG(LogTemp, Log, TEXT("Attack Montage Ended (Index: %d, Interrupted: %s)."), CurrentAttackComboIndex, bInterrupted ? TEXT("True") : TEXT("False"));
        
        // 取得動畫實例
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            // 移除委託，防止多次綁定和重複調用。
            // 這個函式主要負責在蒙太奇結束後進行必要的清理，而不是重置 Combo。
            // 重置 Combo 的邏輯現在主要由 `OnComboWindowEnd()` 和 `TryEnterNextCombo()` 中的判斷來處理。
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &APlayerCharacter::OnAttackMontageEnded);
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
