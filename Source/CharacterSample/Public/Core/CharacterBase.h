// CharacterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h" // 繼承自 Unreal Engine 的 ACharacter
#include "CharacterBase.generated.h" // 務必放在最後一行

// 宣告一個委託 (Delegate) 用於通知生命值變更 (可選，但很有用)
// 這樣 UI 或其他系統可以訂閱這個事件來更新血條
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

UCLASS()
class CHARACTERSAMPLE_API ACharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    // 構造函數
    ACharacterBase();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // --- 傷害與生命值系統 ---
	
	// 當有恢復血量應用到這個Actor時，引擎會呼叫這個函數
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount);

	// 可以添加一個藍圖可實現事件，用於治療時的特效和UI反饋
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealedBlueprintEvent(float HealAmount);

    // 當有傷害應用到這個Actor時，引擎會呼叫這個函數
    // 覆寫它來處理生命值邏輯
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    // 藍圖可呼叫的通用傷害應用函數
    // 從攻擊者那邊呼叫來對此角色造成傷害的主要入口
    UFUNCTION(BlueprintCallable, Category = "Health")
    void ApplyDamageToHealth(float DamageAmount);

    // 藍圖可讀寫的生命值屬性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float MaxHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health") // VisibleAnywhere 讓藍圖只讀
    float CurrentHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    bool bIsDead; // 角色是否已死亡

    // 無敵時間相關屬性 (可選，但很實用)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Invincibility")
    float InvincibilityDuration; // 無敵持續時間

protected:
    // 私有變數，用於追蹤無敵計時器
    FTimerHandle InvincibilityTimerHandle;
    bool bIsInInvincibility; // 是否處於無敵狀態

    // 呼叫以啟動無敵時間
    void StartInvincibility();
    // 呼叫以結束無敵時間
    void EndInvincibility();

    // --- 藍圖可實現事件 (BlueprintImplementableEvent) ---
    // 這些事件將在藍圖子類中被實現，用於處理視覺和音效反饋

    // 當生命值改變時觸發 (例如 UI 更新血條)
    UPROPERTY(BlueprintAssignable, Category = "Health") // Assignable 讓藍圖可以綁定事件
    FOnHealthChangedSignature OnHealthChanged;

    // 當角色受到傷害時觸發 (例如播放受擊動畫、顯示受擊特效)
    UFUNCTION(BlueprintImplementableEvent, Category = "Health")
    void OnDamagedBlueprintEvent(float DamageAmount, AController* EventInstigator, AActor* DamageCauser);

    // 當角色死亡時觸發 (例如播放死亡動畫、銷毀角色)
    UFUNCTION(BlueprintImplementableEvent, Category = "Health")
    void OnDeathBlueprintEvent();
};
