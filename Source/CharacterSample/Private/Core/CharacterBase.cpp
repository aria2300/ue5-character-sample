// CharacterBase.cpp

#include "Core/CharacterBase.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h" // 引用 DamageType 相關頭檔，雖然本範例未使用具體類型判斷，但標準函數需要

// Sets default values
ACharacterBase::ACharacterBase()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // 初始化生命值
    MaxHealth = 100.0f; // 預設最大生命值
    CurrentHealth = MaxHealth; // 初始化時生命值等於最大生命值
    bIsDead = false; // 初始化時未死亡

    // 初始化無敵時間
    InvincibilityDuration = 0.5f; // 預設無敵時間 0.5 秒
    bIsInInvincibility = false; // 預設不在無敵狀態
}

// Called when the game starts or when spawned
void ACharacterBase::BeginPlay()
{
    Super::BeginPlay();

    // 在遊戲開始時，如果生命值沒有從藍圖設定，則確保 CurrentHealth 等於 MaxHealth
    if (CurrentHealth == 0.0f) // 簡單檢查，避免藍圖未賦值導致初始為0
    {
        CurrentHealth = MaxHealth;
    }

    // 可以在這裡廣播初始生命值，用於 UI 初始化
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

// Called every frame
void ACharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// --- 傷害與生命值系統實作 ---

void ACharacterBase::Heal(float HealAmount)
{
    if (bIsDead) // 已死亡則無法恢復
    {
        return;
    }

    // 確保恢復量為正值
    HealAmount = FMath::Abs(HealAmount); // 確保是正數，以防意外傳入負值

    // 增加生命值，並限制在 MaxHealth 範圍內
    CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);

    // 廣播生命值改變事件 (UI 更新)
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    // 觸發藍圖治療事件
    OnHealedBlueprintEvent(HealAmount);

    // 治療通常不需要無敵時間，但可以根據遊戲設計決定
}

float ACharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 如果已經死亡，或者正處於無敵狀態，則不處理傷害
    if (bIsDead || bIsInInvincibility)
    {
        return 0.0f; // 返回 0 表示沒有實際造成傷害
    }

    // 調用父類的 TakeDamage 確保其他基礎邏輯被執行
    // 重要的是，這個函數在引擎內部處理各種傷害類型，最終會呼叫到這裏
    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 減去生命值，並限制在 0 到 MaxHealth 之間
    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

    // 廣播生命值改變事件
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    // 觸發藍圖受傷事件
    OnDamagedBlueprintEvent(ActualDamage, EventInstigator, DamageCauser);

    // 檢查是否死亡
    if (CurrentHealth <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        // 觸發藍圖死亡事件
        OnDeathBlueprintEvent();
        // 可以在這裡添加角色的禁用輸入、禁用碰撞等邏輯
        // 例如：SetActorEnableCollision(ECollisionEnabled::NoCollision);
        // GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // GetCharacterMovement()->DisableMovement();
        // 如果是玩家角色，可能需要禁用控制器輸入
        // if (AController* PC = GetController()) { PC->DisableInput(nullptr); }
    }
    else // 如果沒有死亡，則啟動無敵時間
    {
        StartInvincibility();
    }

    return ActualDamage; // 返回實際造成的傷害量
}

void ACharacterBase::ApplyDamageToHealth(float DamageAmount)
{
    // 這個函數可以用於藍圖內部或其他地方簡單地施加傷害，
    // 它會內部呼叫 TakeDamage，方便統一處理邏輯
    // 注意：這裡只是一個簡化版本，實際遊戲中通常會用 UGameplayStatics::ApplyDamage
    // 來觸發 TakeDamage 事件，這樣 DamageEvent, EventInstigator, DamageCauser 這些參數會有值
    
    // 這裡我們直接調用 TakeDamage，但注意參數會是空的，
    // 如果需要這些詳細資訊，應透過 UGameplayStatics::ApplyDamage 呼叫
    TakeDamage(DamageAmount, FDamageEvent(), nullptr, nullptr); 
}

void ACharacterBase::StartInvincibility()
{
    if (!bIsInInvincibility)
    {
        bIsInInvincibility = true;
        // 設定定時器，在 InvincibilityDuration 後呼叫 EndInvincibility
        GetWorldTimerManager().SetTimer(InvincibilityTimerHandle, this, &ACharacterBase::EndInvincibility, InvincibilityDuration, false);
    }
}

void ACharacterBase::EndInvincibility()
{
    bIsInInvincibility = false;
    GetWorldTimerManager().ClearTimer(InvincibilityTimerHandle); // 清除定時器，避免重複呼叫
}
