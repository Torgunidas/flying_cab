#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "FlyingCabDialogueSession.h"
#include "FlyingCabDialogueWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UFlyingCabDialogueWidget;

UCLASS()
class UFlyingCabDialogueOptionButton : public UButton
{
	GENERATED_BODY()
public:
	UFlyingCabDialogueOptionButton();
	void InitializeOption(UFlyingCabDialogueWidget* InOwner, int32 InIndex, int32 InRevision);
private:
	UFUNCTION() void Choose();
	TWeakObjectPtr<UFlyingCabDialogueWidget> OwnerWidget;
	int32 Index = 0;
	int32 Revision = 0;
};

/** Native fallback view; a Widget Blueprint may implement PresentDialogue and call ChooseOption/CloseDialogue. */
UCLASS(Blueprintable)
class FLYINGCABFLIGHTLAB_API UFlyingCabDialogueWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetSession(UFlyingCabDialogueSession* InSession);
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ChooseOption(int32 Index, int32 Revision);
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void CloseDialogue();
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void PresentDialogue(const FFlyingCabDialogueView& View);
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UFlyingCabDialogueSession* GetSession() const { return Session; }
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
	void BuildLayout();
	UFUNCTION() void RefreshView();
	UPROPERTY(Transient) TObjectPtr<UFlyingCabDialogueSession> Session;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Speaker;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Line;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Options;
	UPROPERTY(Transient) TArray<TObjectPtr<UFlyingCabDialogueOptionButton>> OptionButtons;
	FFlyingCabDialogueView CurrentView;
	int32 SelectedOption = 0;
};
