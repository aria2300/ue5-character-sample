// PlayerCharacter.cpp

#include "Player/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h" // 用於 GetForwardVector 等


// Sets default values
APlayerCharacter::APlayerCharacter()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

// ====================================================================
    // >>> 三國無雙類型轉向的核心設定 <<<
    // ====================================================================

    // 禁用角色（通常是 Mesh）跟隨控制器 Yaw 軸旋轉
    bUseControllerRotationYaw = false; // 角色身體不再跟隨攝影機左右轉動
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // 獲取 CharacterMovementComponent
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        // 啟用：讓角色自動面向其移動的方向
        MovementComp->bOrientRotationToMovement = true; 
        
        // 設定角色的轉向速度，通常設得很高，使其轉向看起來很迅速
        // 你可以調整這個值來達到最符合三國無雙的「瞬時」感
        MovementComp->RotationRate = FRotator(0.0f, 1000.0f, 0.0f); // 例子：每秒 1000 度，非常快
                                                                 // 設為更大的值如 2000.0f 或 3600.0f (10圈/秒) 甚至 99999.0f 
                                                                 // 會更接近瞬時轉身。

        // 設置移動速度（根據你的遊戲設計）
        MovementComp->MaxWalkSpeed = 600.0f; // 範例值，根據需要調整
        MovementComp->JumpZVelocity = 600.f; // 範例值，根據需要調整
        MovementComp->AirControl = 0.2f;     // 範例值，根據需要調整
    }

    // 設置攝影機和 SpringArm (如果你的角色有這些組件)
    // 通常三國無雙類型的遊戲，攝影機也可能會比較固定地跟隨角色背面
    // 或者有自動調整的功能。如果你的攝影機也要跟隨控制器的 Yaw，請確保 SpringArm 的 Use Pawn Control Rotation 為 true
    USpringArmComponent* CameraBoom = FindComponentByClass<USpringArmComponent>();
    if (CameraBoom)
    {
        CameraBoom->bUsePawnControlRotation = true; // Spring Arm 會跟隨控制器的 Yaw 軸旋轉
        // CameraBoom->bInheritPitch = true; // 根據需要設置
        // CameraBoom->bInheritRoll = true;  // 根據需要設置
        // CameraBoom->bDoCollisionTest = true; // 確保碰撞檢測
    }

    UCameraComponent* FollowCamera = FindComponentByClass<UCameraComponent>();
    if (FollowCamera)
    {
        FollowCamera->bUsePawnControlRotation = false; // 相機本身不旋轉，而是由 Spring Arm 旋轉
    }
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // ====================================================================
    // >>> 在遊戲開始時添加 Input Mapping Context <<<
    // ====================================================================
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0); 
                UE_LOG(LogTemp, Log, TEXT("Added DefaultMappingContext to Player Subsystem."));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("DefaultMappingContext is NULL, cannot add to Player Subsystem."));
            }
        }
    }
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ====================================================================
    // >>> 動畫狀態變數更新 (MovementDirection 的計算可能需要重新思考) <<<
    // ====================================================================
    if (GetCharacterMovement())
    {
        CurrentSpeed = GetVelocity().Size();
        bIsFalling = GetCharacterMovement()->IsFalling();

        // 由於 bOrientRotationToMovement = true，角色總是面向移動方向
        // 所以 MovementDirection 永遠應該接近 0 (相對於自身總是向前)
        // 這意味著你可能不再需要 MovementDirection，或者可以直接將它設為 0
        // 如果你仍然有需要平移動畫的場景，MovementDirection 依然有用
        if (CurrentSpeed > KINDA_SMALL_NUMBER) 
        {
            // 如果你只想要純粹的前進動畫，可以簡化這部分
            // MovementDirection = 0.0f; // 简单粗暴地设为 0
            
            // 如果你仍然希望在某些情況下能判斷平移（例如攻擊時的微調）
            // 則這個計算仍然有效，但預期值會更接近 0
            FVector WorldVelocity = GetVelocity();
            WorldVelocity.Z = 0.f; 

            if (WorldVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
            {
                WorldVelocity.Normalize(); 

                const FRotator CharacterYawRotation(0, GetActorRotation().Yaw, 0);
                FVector LocalVelocity = CharacterYawRotation.UnrotateVector(WorldVelocity);

                MovementDirection = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
            }
            else
            {
                MovementDirection = 0.0f; 
            }
        }
        else
        {
            MovementDirection = 0.0f; 
        }
    }
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInputComponent)
    {
        // 綁定 Move Action
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        }
        
        // 綁定 Jump Action
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Jump);
        }

        // 綁定 Look Action (控制攝影機，但不影響角色身體的 Yaw)
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        }
    }
}

// ====================================================================
// >>> Move 函數保持不變，因為 AddMovementInput 會配合 bOrientRotationToMovement 工作 <<<
// ====================================================================
void APlayerCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 獲取控制器的旋轉，但只使用 Yaw
        // 這樣可以確保即使角色身體面向移動方向，你仍然可以基於攝影機方向輸入移動
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, ControlRotation.Yaw, 0); 

        // 根據攝影機方向獲取前向和右向向量
        const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(YawRotation);
        const FVector RightDirection = UKismetMathLibrary::GetRightVector(YawRotation);

        // 添加移動輸入
        // 角色會根據這些輸入的方向，自動轉向並向前移動
        AddMovementInput(ForwardDirection, MovementVector.Y); // 前後移動
        AddMovementInput(RightDirection, MovementVector.X); // 左右移動
    }
}

// ====================================================================
// >>> Look 函數保持不變，它仍然控制攝影機 <<<
// ====================================================================
void APlayerCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 這會轉動控制器，進而轉動相機。
        // 由於 bUseControllerRotationYaw = false，角色身體的 Yaw 不會跟著轉
        AddControllerYawInput(LookAxisVector.X); 
        AddControllerPitchInput(LookAxisVector.Y);
    }
}
