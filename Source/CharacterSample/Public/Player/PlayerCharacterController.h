// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerCharacterController.generated.h"

class UHealthBarBaseWidget;

/**
 * 
 */
UCLASS()
class CHARACTERSAMPLE_API APlayerCharacterController : public APlayerController
{
	GENERATED_BODY()

private:
	// ====================================================================
    // >>> UI 相關屬性 <<<
    // ====================================================================
    // 這個屬性允許你在藍圖編輯器中指定要使用的血條 Widget Blueprint 類別。
    // EditDefaultsOnly: 只能在藍圖的 Default 屬性中編輯，不能在實例上編輯。
    // Category = "UI": 讓它在藍圖 Details 面板中歸類到 "UI" 分類下。
    // TSubclassOf<UHealthBarBaseWidget>: 確保只能選擇繼承自 UHealthBarBaseWidget 的藍圖 Widget 類別。
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UHealthBarBaseWidget> HealthBarWidgetClass;

	// 確保這裡有 HealthBarWidgetInstance 的宣告！
    UPROPERTY() // 不需要 EditAnywhere，因為它是在 C++ 中創建的實例
    UHealthBarBaseWidget* HealthBarWidgetInstance;

	// 負責創建和設定血條 Widget 的輔助函式
    void CreateAndSetupHealthBarWidget();

public:
	// 構造函數：設定控制器的預設值
	APlayerCharacterController();

protected:
    // BeginPlay：在遊戲開始時或角色被生成時呼叫
    virtual void BeginPlay() override;
	// 當 PlayerController 擁有（Possess）一個新的 Pawn 時會被呼叫
    virtual void OnPossess(APawn* InPawn) override;
    // 當 PlayerController 失去（UnPossess）一個 Pawn 時會被呼叫
    virtual void OnUnPossess() override;
};
