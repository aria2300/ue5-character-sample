// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthBarBaseWidget.h"
// #include "Kismet/GameplayStatics.h" // 如果你在這裡需要用到 GameplayStatics，才需要包含
// #include "Components/ProgressBar.h" // 如果你直接在 C++ 中訪問 ProgressBar

void UHealthBarBaseWidget::SetOwnerCharacterAndInitialize(ACharacterBase* NewOwnerCharacter)
{
    OwnerCharacter = NewOwnerCharacter;

    if (OwnerCharacter)
    {
        // 綁定到 OwnerCharacter 的 OnHealthChanged 委託
        // 注意：這裡如果 OwnerCharacter 是 nullptr，這個綁定會失敗，所以要檢查
        OwnerCharacter->OnHealthChanged.AddDynamic(this, &UHealthBarBaseWidget::K2_UpdateHealthBarUI);

        // 立即呼叫一次藍圖事件以更新初始顯示
        K2_UpdateHealthBarUI(OwnerCharacter->GetCurrentHealth(), OwnerCharacter->GetMaxHealth());
    }
}

void UHealthBarBaseWidget::NativeConstruct()
{
    Super::NativeConstruct(); // 務必呼叫父類的 NativeConstruct

    // 如果你在 C++ 創建時就設置了 OwnerCharacter，這裡可以進行綁定
    // 但更推薦在 SetOwnerCharacterAndInitialize 中進行綁定，因為它確保 OwnerCharacter 已被設置
    // 如果這裡的 OwnerCharacter 已經被設置 (例如通過 SetOwnerCharacterAndInitialize 或暴露在 Spawn 時設置)
    // 則可以在這裡綁定。否則會綁定到一個空指針。
}

// NativeTick 和其他 C++ 邏輯如果你需要的話...
// void UHealthBarBaseWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
// {
//     Super::NativeTick(MyGeometry, InDeltaTime);
//     // 這裡可以添加每幀更新的邏輯，但對於血條，委託更新更高效
// }