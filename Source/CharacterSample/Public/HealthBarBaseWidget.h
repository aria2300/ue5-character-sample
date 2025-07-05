// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/CharacterBase.h"
#include "HealthBarBaseWidget.generated.h"

/**
 * 
 */
UCLASS()
class CHARACTERSAMPLE_API UHealthBarBaseWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // 初始化血條的擁有者，Actor物件專用
    // 將其公開為 BlueprintReadWrite 並加入 Category 便於在編輯器中訪問和組織。
    UPROPERTY(BlueprintReadWrite, Category = "HealthBar") // <-- Added Category
    ACharacterBase* OwnerCharacter;

    // 建議添加一個設定 OwnerCharacter 並觸發 UI 更新的函數
    // 這樣在 C++ 和藍圖中都能以統一的方式初始化血條
    UFUNCTION(BlueprintCallable, Category = "HealthBar")
    void SetOwnerCharacterAndInitialize(ACharacterBase* NewOwnerCharacter);

protected:
    // 如果你需要在 C++ 中處理 Construct 邏輯，可以覆寫這個
    virtual void NativeConstruct() override; 

    // 聲明一個 Blueprint Implementable Event，讓藍圖去實現實際的 UI 更新邏輯
    // 這樣當生命值變化時，C++ 可以呼叫這個事件，藍圖來更新 Progress Bar 和 Text
    UFUNCTION(BlueprintImplementableEvent, Category = "HealthBar")
    void K2_UpdateHealthBarUI(float CurrentHealth, float MaxHealth);
};
