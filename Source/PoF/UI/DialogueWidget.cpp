#include "UI/DialogueWidget.h"
#include "Dialogue/ARPGDialogueComponent.h"
#include "Dialogue/ARPGDialogueTree.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetTree.h"

void UDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDialogueWidget::BuildLayout()
{
	RootBorder = NewObject<UBorder>(this);
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 0.9f));
	RootBorder->SetPadding(FMargin(20.f));

	UVerticalBox* MainVBox = NewObject<UVerticalBox>(this);
	RootBorder->SetContent(MainVBox);

	// Speaker row (portrait + name)
	UHorizontalBox* SpeakerRow = NewObject<UHorizontalBox>(this);
	auto* SpeakerSlot = MainVBox->AddChildToVerticalBox(SpeakerRow);
	SpeakerSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	SpeakerPortraitImage = NewObject<UImage>(this);
	SpeakerPortraitImage->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
	auto* PortraitSlot = SpeakerRow->AddChildToHorizontalBox(SpeakerPortraitImage);
	PortraitSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

	SpeakerNameText = NewObject<UTextBlock>(this);
	SpeakerNameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f)));
	auto* NameSlot = SpeakerRow->AddChildToHorizontalBox(SpeakerNameText);
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// Dialogue body
	DialogueBodyText = NewObject<UTextBlock>(this);
	DialogueBodyText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
	DialogueBodyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DialogueBodyText->SetAutoWrapText(true);
	auto* BodySlot = MainVBox->AddChildToVerticalBox(DialogueBodyText);
	BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Choice container
	ChoiceContainer = NewObject<UVerticalBox>(this);
	MainVBox->AddChildToVerticalBox(ChoiceContainer);

	// Continue button (for nodes without player choices)
	ContinueButton = NewObject<UButton>(this);
	ContinueButton->OnClicked.AddDynamic(this, &UDialogueWidget::OnContinueClicked);

	ContinueButtonText = NewObject<UTextBlock>(this);
	ContinueButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	ContinueButtonText->SetText(FText::FromString(TEXT("[Click to continue]")));
	ContinueButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
	ContinueButton->AddChild(ContinueButtonText);

	auto* ContSlot = MainVBox->AddChildToVerticalBox(ContinueButton);
	ContSlot->SetHorizontalAlignment(HAlign_Center);

	// Set as root widget via WidgetTree
	if (WidgetTree)
	{
		WidgetTree->RootWidget = RootBorder;
	}
}

void UDialogueWidget::BindToDialogue(UARPGDialogueComponent* DialogueComp)
{
	if (!DialogueComp) return;

	UnbindFromDialogue();

	BoundDialogue = DialogueComp;
	DialogueComp->OnDialogueNodeEntered.AddDynamic(this, &UDialogueWidget::OnDialogueNodeEntered);
	DialogueComp->OnDialogueEnded.AddDynamic(this, &UDialogueWidget::OnDialogueEnded);

	SetVisibility(ESlateVisibility::Visible);
}

void UDialogueWidget::UnbindFromDialogue()
{
	if (UARPGDialogueComponent* Comp = BoundDialogue.Get())
	{
		Comp->OnDialogueNodeEntered.RemoveDynamic(this, &UDialogueWidget::OnDialogueNodeEntered);
		Comp->OnDialogueEnded.RemoveDynamic(this, &UDialogueWidget::OnDialogueEnded);
	}
	BoundDialogue.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
	ClearChoices();
}

void UDialogueWidget::OnDialogueNodeEntered(int32 NodeIndex, const FARPGDialogueNode& Node)
{
	RefreshNode(NodeIndex, Node);
}

void UDialogueWidget::OnDialogueEnded()
{
	UnbindFromDialogue();
}

void UDialogueWidget::RefreshNode(int32 NodeIndex, const FARPGDialogueNode& Node)
{
	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(Node.SpeakerDisplayName);
	}

	if (SpeakerPortraitImage)
	{
		if (!Node.SpeakerPortrait.IsNull())
		{
			UTexture2D* Tex = Node.SpeakerPortrait.LoadSynchronous();
			if (Tex)
			{
				SpeakerPortraitImage->SetBrushFromTexture(Tex);
				SpeakerPortraitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			SpeakerPortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (DialogueBodyText)
	{
		DialogueBodyText->SetText(Node.DialogueText);
	}

	ClearChoices();

	UARPGDialogueComponent* Comp = BoundDialogue.Get();
	if (!Comp) return;

	TArray<FARPGDialogueChoice> Available = Comp->GetAvailableChoices();

	if (Available.Num() > 0)
	{
		if (ContinueButton) ContinueButton->SetVisibility(ESlateVisibility::Collapsed);

		for (int32 i = 0; i < Available.Num(); ++i)
		{
			CreateChoiceButton(i, Available[i].ChoiceText);
		}
	}
	else
	{
		if (ContinueButton) ContinueButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UDialogueWidget::ClearChoices()
{
	if (ChoiceContainer)
	{
		ChoiceContainer->ClearChildren();
	}
	ChoiceButtons.Empty();
}

void UDialogueWidget::CreateChoiceButton(int32 Index, const FText& Text)
{
	if (!ChoiceContainer) return;

	UButton* Btn = NewObject<UButton>(this);

	UTextBlock* BtnText = NewObject<UTextBlock>(this);
	BtnText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 15));
	BtnText->SetText(FText::Format(
		NSLOCTEXT("Dialogue", "ChoiceFmt", "{0}. {1}"),
		FText::AsNumber(Index + 1),
		Text));
	BtnText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)));
	BtnText->SetAutoWrapText(true);
	Btn->AddChild(BtnText);

	Btn->OnClicked.AddDynamic(this, &UDialogueWidget::OnChoiceButtonClicked);

	auto* BtnSlot = ChoiceContainer->AddChildToVerticalBox(Btn);
	BtnSlot->SetPadding(FMargin(0.f, 2.f));

	ChoiceButtons.Add(Btn);
}

void UDialogueWidget::OnContinueClicked()
{
	if (UARPGDialogueComponent* Comp = BoundDialogue.Get())
	{
		Comp->AdvanceDialogue();
	}
}

void UDialogueWidget::OnChoiceButtonClicked()
{
	for (int32 i = 0; i < ChoiceButtons.Num(); ++i)
	{
		if (ChoiceButtons[i] && ChoiceButtons[i]->IsHovered())
		{
			OnChoiceClicked(i);
			return;
		}
	}
}

void UDialogueWidget::OnChoiceClicked(int32 ChoiceIndex)
{
	if (UARPGDialogueComponent* Comp = BoundDialogue.Get())
	{
		Comp->SelectChoice(ChoiceIndex);
	}
}
