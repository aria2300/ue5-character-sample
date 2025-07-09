// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


// 前向聲明，避免循環引用，因為 CombatComponent 會引用 Character
class APlayerCharacter;
class UAnimMontage;
class UInputMappingContext; // 雖然輸入綁定會拆出去，但為了完整性，先聲明

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHARACTERSAMPLE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // 暫時不需要 Tick，可以先註釋掉

	// ====================================================================
	// >>> 攻擊相關函式 <<<
	// ====================================================================

	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void Attack(); // 處理攻擊輸入 (由輸入系統綁定)

	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void PlayAttackComboSegment(); // 播放當前連擊段動畫

	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void TryEnterNextCombo(); // 嘗試進入下一段連擊

	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void ResetCombo(); // 重置連擊狀態

	UFUNCTION() // 動態委託需要 UFUNCTION 標記
	void OnComboWindowEnd(); // 連擊窗口定時器結束時呼叫

	// 由 Anim Notify 呼叫
	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void SetCanEnterNextCombo(bool bCan);

	// 由 Anim Notify 呼叫
	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void SetPendingNextComboInput(bool bPending);

	// 由 Anim Notify 呼叫 - 執行實際的攻擊命中檢測
	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void PerformNormalAttackHitCheck();

	UFUNCTION() // 動態委託需要 UFUNCTION 標記
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted); // 攻擊動畫結束時呼叫

protected:
	// ====================================================================
	// >>> 參考：擁有的角色 <<<
	// 讓 CombatComponent 能夠訪問到它所附加的 APlayerCharacter
	// ====================================================================
	UPROPERTY()
	APlayerCharacter* OwnerCharacter; // 將是附加此組件的 PlayerCharacter

	// ====================================================================
	// >>> 攻擊連擊相關屬性 <<<
	// ====================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	TArray<UAnimMontage*> AttackMontages; // 攻擊動畫蒙太奇陣列，藍圖中設置

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack")
	int32 CurrentAttackComboIndex; // 當前連擊段數

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack")
	bool bIsAttacking; // 是否正在攻擊中

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack")
	bool bCanEnterNextCombo; // 是否可以進入下一段連擊 (由 Anim Notify 設置)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack")
	bool bPendingNextComboInput; // 是否有連段輸入緩衝

	FTimerHandle ComboWindowTimerHandle; // 連擊窗口定時器句柄

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsDead; // 角色是否死亡 (未來可能移到 HealthComponent)
};
